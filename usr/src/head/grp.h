/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _GRP_H
#define _GRP_H 

#ident	"@(#)head:grp.h	1.3.1.8"

#include <sys/types.h>

struct	group {	/* see getgrent(3) */
	char	*gr_name;
	char	*gr_passwd;
	gid_t	gr_gid;
	char	**gr_mem;
};

#include <stdio.h>

#if defined(__STDC__)

extern void endgrent(void);
extern struct group *fgetgrent(FILE *);
extern struct group *getgrent(void);
extern struct group *getgrgid(uid_t);
extern struct group *getgrnam(const char *);
extern void setgrent(void);
extern int initgroups(const char *, gid_t);

#else

extern void endgrent();
extern struct group *fgetgrent();
extern struct group *getgrent();
extern struct group *getgrgid();
extern struct group *getgrnam();
extern void setgrent();
extern int initgroups();

#endif	/* __STDC__ */

#endif 	/* _GRP_H */
