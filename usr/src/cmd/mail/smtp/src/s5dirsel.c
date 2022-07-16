/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:smtp/src/s5dirsel.c	1.3"
#ident "@(#)s5dirsel.c	1.2 'attmail mail(1) command'"
#include <sys/types.h>
#define	dirent	direct	/* S5 dirent's are V9/BSD direct's -- yecch! */
#include <dirent.h>
#undef	dirent
#include <sys/stat.h>
#include "dirsel.h"

/* almost like 4.2 scandir(), but the returned structs are different */
#define MAXENTS 2000	/* hope we don't find a dir with > MAXENTS entries */

int
dirsel(dirname, pents, select, compare)
	char *dirname;
	dirent **pents;
	int (*select)();
	int (*compare)();
{
	dirent *ep;
	DIR *ddp;
	struct direct *dp;
	struct stat statbuf;
	unsigned long starttime;
	int nents=0;
	char fullname[MAXNAMLEN+1];
	char *p;

	ddp=opendir(dirname);
	strcpy(fullname, dirname);
	p= &fullname[strlen(dirname)];
	*p++ = '/';
	*p='\0';
	if (ddp==0)
		return 0;
	starttime=(unsigned long)time((long *)0);
	*pents=ep=(dirent *)malloc(MAXENTS * sizeof(dirent));
	while ((dp=readdir(ddp))!=0){
		strcpy(p, dp->d_name);
		if (stat(fullname, &statbuf)!=0)
			continue;	/* shouldn't happen */
		ep->name=(char *)malloc(dp->d_reclen+1);
		strcpy(ep->name, dp->d_name);
		ep->namelen=dp->d_reclen;
		ep->isdir=((statbuf.st_mode&S_IFDIR) != 0);
		ep->age=(long)(starttime-(unsigned long)statbuf.st_mtime);
		if (select && !(*select)(ep))
			continue;	/* don't count this entry */
		ep++, nents++;
		if (nents==MAXENTS)
			break;	/* could realloc */
	}
	closedir(ddp);
	if (nents>1 && compare)
		qsort(*pents, nents, sizeof(dirent), compare);
	return nents;
}
