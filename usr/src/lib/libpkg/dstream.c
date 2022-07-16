/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)libpkg:dstream.c	1.9.2.2"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/statfs.h>

extern int	errno;

extern FILE	*epopen();
extern int	pkgnmchk(),
		eptclose(),
		esystem();
extern void	*calloc(),
		ecleanup(),
		cleanup(),
		rpterr(),
		progerr(), 
		logerr(),
		free();

#define CMDSIZ	512
#define LSIZE	128
#define DDPROC		"/usr/bin/dd"
#define CPIOPROC	"/usr/bin/cpio"

#define ERR_UNPACK	"attempt to process datastream failed"
#define ERR_DSTREAMSEQ	"datastream sequence corruption"
#define MSG_MEM		"no memory"
#define MSG_CMDFAIL	"- process <%s> failed, exit code %d"
#define MSG_TOC		"- bad format in datastream table-of-contents"
#define MSG_EMPTY	"- datastream table-of-contents appears to be empty"
#define MSG_POPEN	"- popen of <%s> failed, errno=%d"
#define MSG_OPEN	"- open of <%s> failed, errno=%d"
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
	char    volnos[128];
	struct dstoc *next;
} *ds_head, *ds_toc;

#define	ds_nparts	ds_toc->nparts
#define	ds_maxsiz	ds_toc->maxsiz
	
int	ds_totread;
int	ds_fd = -1;
int	ds_curpartcnt;
int	ds_next();

static int	ds_read;
static int	ds_volno;	/* volume number of current volume */
static int	ds_volcnt;	/* total number of volumes */
static char	ds_volnos[128];	/* parts/volume info */
static int	ds_volpart;	/* number of parts read in current volume, including skipped parts */
static int	ds_skippart;	/* number of parts skipped in current volume */
static int	ds_getnextvol(), ds_skip();

void
ds_order(list)
char *list[];
{
	struct dstoc *toc_pt;
	register int j, n;
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

static char *pds_header;
static char *ds_header;
static int ds_headsize;

static char *
ds_gets(buf, size)
char *buf;
int size;
{
	int length;
	char *nextp;

	nextp = strchr(pds_header, '\n');
	if(nextp == NULL) {
		length = strlen(pds_header);
		if(length > size)
			return 0;
		if((ds_header = (char *)realloc(ds_header, ds_headsize + 512)) == NULL) 
			return 0;
		if(read(ds_fd, ds_header + ds_headsize, 512) < 512)
			return 0;
		ds_headsize += 512;
		nextp = strchr(pds_header, '\n');
		if(nextp == NULL)
			return 0;
		*nextp = '\0';
		if(length + (int)strlen(pds_header) > size)
			return 0;
		(void)strncpy(buf + length, pds_header, strlen(pds_header));
		buf[length + strlen(pds_header)] = '\0';
		pds_header = nextp + 1;
		return buf;
	}
	*nextp = '\0';
	if((int)strlen(pds_header) > size)
		return 0;
	(void)strncpy(buf, pds_header, strlen(pds_header));
	buf[strlen(pds_header)] = '\0';
	pds_header = nextp + 1;
	return buf;
}

/*
 * function to determine if media is datastream or mounted 
 * floppy
 */
int
ds_readbuf(device)
char *device;
{
	char buf[512];

	if(ds_fd > 0)
		(void)close(ds_fd);
	if((ds_fd = open(device, 0)) >= 0 
	  	&& read(ds_fd, buf, 512) == 512
	  	&& strncmp(buf, "# PaCkAgE DaTaStReAm", 20) == 0) {
		if((ds_header = (char *)malloc(512)) == NULL) {
			(void)close(ds_fd);
			ds_fd = -1;
			return 0;
		}
		memcpy(ds_header, buf, 512);
		ds_headsize = 512;
		return 1;
	} else if(ds_fd >= 0) {
		(void)close(ds_fd);
		ds_fd = -1;
	}
	return 0;
}

/*
 * Determine how many additional volumes are needed for current package.
 * Note: a 0 will occur as first volume number when the package begins
 * on the next volume.
 */
static int
ds_volsum(toc)
struct dstoc *toc;
{
	int curpartcnt, volcnt;
	char volnos[128], tmpvol[128];
	if(toc->volnos[0]) {
		int index, sum;
		sscanf(toc->volnos, "%d %[ 0-9]", &curpartcnt, volnos);
		volcnt = 0;
		sum = curpartcnt;
		while(sum < toc->nparts && sscanf(volnos, "%d %[ 0-9]", &index, tmpvol) >= 1) {
			(void)strcpy(volnos, tmpvol);
			volcnt++;
			sum += index;
		}
		/* side effect - set number of parts read on current volume */
		ds_volpart = index;
		return volcnt;
	}
	ds_volpart += toc->nparts;
	return 0;
}

/* initialize ds_curpartcnt and ds_volnos */
static void
ds_pkginit()
{
	if(ds_toc->volnos[0])
		sscanf(ds_toc->volnos, "%d %[ 0-9]", &ds_curpartcnt, ds_volnos);
	else
		ds_curpartcnt = -1;
}

/* functions to pass current package info to exec'ed program */
void
ds_putinfo(buf)
char *buf;
{
	(void)sprintf(buf, "%d %d %d %d %d %d %d %d %s", ds_fd, ds_volcnt, ds_volno, ds_totread, ds_volpart, ds_skippart, ds_toc->nparts, ds_toc->maxsiz, ds_toc->volnos);
}

int
ds_getinfo(string)
char *string;
{
	ds_toc = (struct dstoc *)calloc(1, sizeof(struct dstoc));
	(void)sscanf(string, "%d %d %d %d %d %d %d %d %[ 0-9]", &ds_fd, &ds_volcnt, &ds_volno, &ds_totread, &ds_volpart, &ds_skippart, &ds_toc->nparts, &ds_toc->maxsiz, ds_toc->volnos);
	ds_pkginit();
	return ds_toc->nparts;
}

int
ds_init(device, pkg)
char	*device;
char	**pkg;
{
	struct dstoc *tail, *toc_pt; 
	char	*ret;
	char	cmd[CMDSIZ];
	char	line[LSIZE+1];
	int	i, n;

	if(!ds_header) {
		if(ds_fd >= 0)
			(void)close(ds_fd);
		if((ds_fd = open(device, 0)) < 0) {
			rpterr();
			progerr(ERR_UNPACK);
			logerr(MSG_OPEN, device, errno);
			return -1;
		}
		if((ds_header = (char *)malloc(512)) == NULL) {
			rpterr();
			progerr(ERR_UNPACK);
			logerr(MSG_MEM);
			return -1;
		}
			
		if(read(ds_fd, ds_header, 512) != 512) {
			rpterr();
			progerr(ERR_UNPACK);
			logerr(MSG_TOC);
			(void)close(ds_fd);
			ds_fd = -1;
			return -1;
		}
		ds_headsize = 512;
	}
	pds_header = ds_header;
	/* read datastream table of contents */
	ds_head = tail = (struct dstoc *)0;
	ds_volcnt = 1;
	while(ret=ds_gets(line, LSIZE)) {
		int length;
		if(strcmp(line, "# end of header") == 0)
			break;
		if(!line[0] || (line[0] == '#'))
			continue;
		toc_pt = (struct dstoc *) calloc(1, sizeof(struct dstoc));
		if(!toc_pt) {
			progerr(ERR_UNPACK);
			logerr(MSG_MEM);
			ecleanup();
			return(-1);
		}
		if((length = sscanf(line, "%14s %d %d %[ 0-9]", toc_pt->pkg, &toc_pt->nparts, 
		&toc_pt->maxsiz, toc_pt->volnos)) < 3) {
			progerr(ERR_UNPACK);
			logerr(MSG_TOC);
			free(toc_pt);
			ecleanup();
			return(-1);
		}
		if(tail) {
			tail->next = toc_pt;
			tail = toc_pt;
		} else
			ds_head = tail = toc_pt;
		ds_volcnt += ds_volsum(toc_pt);
	}
	if(!ret) {
		progerr(ERR_UNPACK);
		logerr(MSG_TOC);
		return -1;
	}
	sighold(SIGINT);
	sigrelse(SIGINT);
	if(!ds_head) {
		progerr(ERR_UNPACK);
		logerr(MSG_EMPTY);
		return(-1);
	}
	/* this could break, thanks to cpio command limit */
	(void) sprintf(cmd, "%s -icdumD -C 512 ", CPIOPROC);
	for(i=0; pkg[i]; i++) {
		if(!strcmp(pkg[i], "all"))
			continue;
		strcat(cmd, pkg[i]);
		strcat(cmd, "'/*' ");
	}
	if(n = esystem(cmd, ds_fd, -1)) {
		rpterr();
		progerr(ERR_UNPACK);
		logerr(MSG_CMDFAIL, cmd, n);
		return(-1);
	}

	ds_toc = ds_head;
	ds_totread = 0;
	ds_volno = 1;
	return(0);
}

ds_findpkg(device, pkg)
char	*device, *pkg;
{
	char	*pkglist[2];
	int	nskip, ods_volpart;

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
		
	nskip = 0;
	ds_volno = 1;
	ds_volpart = 0;
	while(ds_toc) {
		if(!strcmp(ds_toc->pkg, pkg))
			break;
		nskip += ds_toc->nparts; 
		ds_volno += ds_volsum(ds_toc);
		ds_toc = ds_toc->next;
	}
	if(!ds_toc) {
		progerr(ERR_UNPACK);
		logerr(MSG_NOPKG, pkg);
		return(-1);
	}

	ds_pkginit();
	ds_skippart = 0;
	if(ds_curpartcnt > 0) {
		ods_volpart = ds_volpart;
		/* skip past archives belonging to last package on current volume */
		if(ds_volpart > 0 && ds_getnextvol(device))
			return -1;
		ds_totread = nskip - ods_volpart;
		if(ds_skip(device, ods_volpart))
			return -1;
	} else if(ds_curpartcnt < 0) {
		if(ds_skip(device, nskip - ds_totread))
			return -1;
	} else
		ds_totread = nskip;
	ds_read = 0;
	return(ds_nparts);
}

/*
 * Get datastream part
 * Call for first part should be preceded by
 * call to ds_findpkg
 */

ds_getpkg(device, n, dstdir) 
char	*device;
int	n;
char	*dstdir;
{
	struct statfs buf;

	if(ds_read >= ds_nparts)
		return(2);

	if(ds_read == n)
		return(0);
	else if((ds_read > n) || (n > ds_nparts))
		return(2);

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
	return ds_next(device, dstdir);
}

static int
ds_getnextvol(device)
char *device;
{
	char prompt[128];
	int n;

	(void)sprintf(prompt, "Insert %%v %d of %d into %%p", ds_volno, ds_volcnt);
	if(ds_fd >= 0)
		(void)close(ds_fd);
	if(n = getvol(device, NULL, NULL, prompt))
		return n;
	if((ds_fd = open(device, 0)) < 0)
		return -1;
	ds_volpart = 0;
	return 0;
}

/*
 * called by ds_findpkg to skip past archives for unwanted packages 
 * in current volume 
 */
static int
ds_skip(device, nskip)
char *device;
int	nskip;
{
	char	cmd[CMDSIZ];
	int	n, onskip = nskip;
	
	while(nskip--) {
		/* skip this one */
		(void) sprintf(cmd, "%s -ictD -C 512 > /dev/null", CPIOPROC);
		if(n = esystem(cmd, ds_fd, -1)) {
			rpterr();
			progerr(ERR_UNPACK);
			logerr(MSG_CMDFAIL, cmd, n);
			nskip = onskip;
			if(ds_volno == 1 || ds_volpart > 0)
				return n;
			if(n = ds_getnextvol(device))
				return n;
		}
	}
	ds_totread += onskip;
	ds_volpart = onskip;
	ds_skippart = onskip;
	return(0);
}

/* skip to end of package if necessary */
void ds_skiptoend(device)
char *device;
{
	if(ds_read < ds_nparts && ds_curpartcnt < 0)
		ds_skip(device, ds_nparts - ds_read);
}


int
ds_next(device, instdir)
char *device;
char *instdir; /* current directory where we are spooling package */
{
	char	cmd[CMDSIZ], tmpvol[128];
	int	nparts, n, index;

	while(1) {
		if(ds_read + 1 > ds_curpartcnt && ds_curpartcnt >= 0) {
			ds_volno++;
			if(n = ds_getnextvol(device))
				return n;
			(void)sscanf(ds_volnos, "%d %[ 0-9]", &index, tmpvol);
			(void)strcpy(ds_volnos, tmpvol);
			ds_curpartcnt += index;
		}
		(void) sprintf(cmd, "%s -icdumD -C 512", CPIOPROC);
		if(n = esystem(cmd, ds_fd, -1)) {
			rpterr();
			progerr(ERR_UNPACK);
			logerr(MSG_CMDFAIL, cmd, n);
		}
		if(ds_read == 0)
			nparts = 0;
		else
			nparts = ds_toc->nparts;
		if(n || (n = ckvolseq(instdir, ds_read + 1, nparts))) {
			if(ds_volno == 1 || ds_volpart > ds_skippart) 
				return -1;
				
			if(n = ds_getnextvol(device))
				return n;
			continue;
		}
		ds_read++;
		ds_totread++;
		ds_volpart++;
	
		return(0);
	}
}
