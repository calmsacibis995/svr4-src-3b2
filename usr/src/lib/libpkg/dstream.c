/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)libpkg:dstream.c	1.9.1.1"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/statfs.h>

extern int	errno;

extern FILE	*epopen();
extern int	pkgnmchk(),
		esystem();
extern void	*calloc(),
		rpterr(),
		progerr(), 
		logerr(),
		free();

#define CMDSIZ	512
#define LSIZE	128
#define DDPROC		"/bin/dd"
#define CPIOPROC	"/bin/cpio"

#define ERR_UNPACK	"attempt to process datastream failed"
#define MSG_MEM		"no memory"
#define MSG_CMDFAIL	"- process <%s> failed, exit code %d"
#define MSG_TOC		"- bad format in datastream table-of-contents"
#define MSG_EMPTY	"- datastream table-of-contents appears to be empty"
#define MSG_POPEN	"- popen of <%s> failed, errno=%d"
#define MSG_PCLOSE	"- pclose of <%s> failed, errno=%d"
#define MSG_PKGNAME	"- invalid package name in datastream table-of-contents"
#define MSG_NOPKG	"- package <%s> not in datastream"
#define MSG_STATFS	"- unable to stat filesystem, errno=%d"
#define MSG_NOSPACE	"- not enough tmp space, %d free blocks required"

struct dstoc {
	int	cnt;
	char	pkg[16];
	long	nparts;
	long	maxsiz;
	struct dstoc *next;
} *ds_head, *ds_toc;

#define	ds_nparts	ds_toc->nparts
#define	ds_maxsiz	ds_toc->maxsiz
	
long	ds_read;
int	ds_next(), ds_skip();

void
ds_order(list)
char *list[];
{
	struct dstoc *toc_pt;
	register int i, j, n;
	char	*pt;

	toc_pt = ds_head;
	n = 0;
	while(toc_pt) {
		for(j=n; list[j]; j++) {
			if(!strcmp(list[j], toc_pt->pkg)) {
				/* just swap places in the array */
				pt = list[n];
				list[n++] = list[j];
				list[j] = pt;
			}
		}
		toc_pt = toc_pt->next;
	}
}

int
ds_init(device, pkg)
char	*device;
char	**pkg;
{
	struct dstoc *tail, *toc_pt;
	FILE	*pp;
	char	cmd[CMDSIZ];
	char	line[LSIZE];
	int	i, n;

	(void) sprintf(cmd, "%s if=%s", DDPROC, device);
	if((pp = epopen(cmd, "r")) == NULL) {
		rpterr();
		progerr(ERR_UNPACK);
		logerr(MSG_POPEN, cmd, errno);
		return(-1);
	}

	/* read datastream table of contents */
	ds_head = tail = (struct dstoc *)0;
	while(fgets(line, LSIZE, pp)) {
		if(!line[0] || (line[0] == '#'))
			continue;
		toc_pt = (struct dstoc *) calloc(1, sizeof(struct dstoc));
		if(!toc_pt) {
			progerr(ERR_UNPACK);
			logerr(MSG_MEM);
			return(-1);
		}
		if(sscanf(line, "%14s %d %d", toc_pt->pkg, &toc_pt->nparts, 
		&toc_pt->maxsiz) != 3) {
			progerr(ERR_UNPACK);
			logerr(MSG_TOC);
			free(toc_pt);
			return(-1);
		}
		if(tail) {
			tail->next = toc_pt;
			tail = toc_pt;
		} else
			ds_head = tail = toc_pt;
	}
	sighold(SIGINT);
	if(pclose(pp)) {
		sigrelse(SIGINT);
		rpterr();
		progerr(ERR_UNPACK);
		logerr(MSG_PCLOSE, cmd, errno);
		return(-1);
	}
	sigrelse(SIGINT);
	if(!ds_head) {
		progerr(ERR_UNPACK);
		logerr(MSG_EMPTY);
		return(-1);
	}

	/* this could break, thanks to cpio command limit */
	(void) sprintf(cmd, "%s -icdum ", CPIOPROC);
	for(i=0; pkg[i]; i++) {
		if(!strcmp(pkg[i], "all"))
			continue;
		strcat(cmd, pkg[i]);
		strcat(cmd, "'/*' ");
	}
	(void) strcat(cmd, "<");
	(void) strcat(cmd, device);
	if(n = esystem(cmd)) {
		rpterr();
		progerr(ERR_UNPACK);
		logerr(MSG_CMDFAIL, cmd, n);
		return(-1);
	}

	ds_toc = ds_head;
	ds_read = 0;
	return(0);
}

ds_findpkg(device, pkg)
char	*device;
char	*pkg;
{
	int	nskip;
	char	*pkglist[2];

	if(ds_head == NULL) {
		pkglist[0] = pkg;
		pkglist[1] = NULL;
		if(ds_init(device, pkglist))
			return(-1);
	}

	if(!pkg || pkgnmchk(pkg, "all")) {
		progerr(ERR_UNPACK);
		logerr(MSG_PKGNAME);
		return(-1);
	}
		
	/* nskip should eventually contain the number of cpio archives 
	 * to skip before we reach the package we're looking for
	 */
	nskip = 0;

	while(ds_toc) {
		if(!strcmp(ds_toc->pkg, pkg))
			break;
		nskip += (ds_toc->nparts - ds_read);
		ds_toc = ds_toc->next;
		ds_read = 0;
	}
	if(!ds_toc) {
		progerr(ERR_UNPACK);
		logerr(MSG_NOPKG, pkg);
		return(-1);
	}
	ds_skip(device, nskip);
	ds_read = 0;
	return(ds_nparts);
}

ds_getpkg(device, pkg, n) 
char	*device;
char	*pkg;
int	n;
{
	struct statfs buf;
	int	nskip;

	if(ds_findpkg(device, pkg) < 0)
		return(1);

	if(ds_read >= ds_nparts)
		return(2);

	if(n) {
		if(ds_read == n)
			return(0);
		else if((ds_read > n) || (n > ds_nparts))
			return(2);
		nskip = n - ds_read - 1;
	} else {
		n = ds_read+1;
		nskip = 0;
	}

	if(ds_maxsiz > 0) {
		if(statfs(".", &buf, sizeof(buf), 0)) {
			progerr(ERR_UNPACK);
			logerr(MSG_STATFS, errno);
			return(-1);
		}
		if((ds_maxsiz + 50) > ((buf.f_bfree * buf.f_bsize)/512)) {
			progerr(ERR_UNPACK);
			logerr(MSG_NOSPACE, ds_maxsiz+50);
			return(-1);
		}
	}
	if(ds_skip(device, nskip))
		return(-1);
	return(ds_next(device));
}

int
ds_skip(device, nskip)
char	*device;
int	nskip;
{
	char	cmd[CMDSIZ];
	int	n;
	
	while(nskip--) {
		/* skip this one */
		(void) sprintf(cmd, "%s if=%s of=/dev/null", DDPROC,
			device);
		ds_read++;
		if(n = esystem(cmd)) {
			rpterr();
			progerr(ERR_UNPACK);
			logerr(MSG_CMDFAIL, cmd, n);
			return(-1);
		}
	}
	return(0);
}

int
ds_next(device)
char *device;
{
	char	cmd[CMDSIZ];
	int	n;

	(void) sprintf(cmd, "%s -icdum <%s", CPIOPROC, device);
	ds_read++;
	if(n = esystem(cmd)) {
		rpterr();
		progerr(ERR_UNPACK);
		logerr(MSG_CMDFAIL, cmd, n);
		return(-1);
	}
	return(0);
}
