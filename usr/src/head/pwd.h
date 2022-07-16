/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PWD_H
#define _PWD_H

#ident	"@(#)head:pwd.h	1.3.1.8"

#include <sys/types.h>

struct passwd {
	char	*pw_name;
	char	*pw_passwd;
	uid_t	pw_uid;
	gid_t	pw_gid;
	char	*pw_age;
	char	*pw_comment;
	char	*pw_gecos;
	char	*pw_dir;
	char	*pw_shell;
};

struct comment {
	char	*c_dept;
	char	*c_name;
	char	*c_acct;
	char	*c_bin;
};

#if defined(__STDC__)

#include <stdio.h>

extern struct passwd *getpwent(void);
extern struct passwd *getpwuid(uid_t);
extern struct passwd *getpwnam(const char *);
extern void setpwent(void);
extern void endpwent(void);
extern struct passwd *fgetpwent(FILE *);
extern int putpwent(const struct passwd *, FILE *);

#else
extern struct passwd *getpwent();
extern struct passwd *getpwuid();
extern struct passwd *getpwnam();
extern void setpwent();
extern void endpwent();
extern struct passwd *fgetpwent();
extern int putpwent();

#endif

#endif /* _PWD_H */
