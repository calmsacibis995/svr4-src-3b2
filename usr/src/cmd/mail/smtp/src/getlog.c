/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:smtp/src/getlog.c	1.3"
#ident "@(#)getlog.c	1.2 'attmail mail(1) command'"
/*
 *	getlog() -- paranoid version of getlogin.
 *	Unnecessary but harmless in V9; may be essential in Sys V
 *		with botched layers implementations
 *
 *	Try getlogin().
 *	If that fails, look in the password file.
 *	if that fails, give up.
 */

#include <pwd.h>

extern struct passwd *getpwuid(), *getpwnam();
extern int getuid();
extern char *getlogin(), *getenv();

#define NULL 0

char *
getlog()
{
	char *p;
	struct passwd *pw;

	if ((p = getlogin()) != NULL)
		return p;
	
	/* If LOGNAME is set, and it matches getuid(), use it */
	p = getenv ("LOGNAME");
	if (p != NULL && *p != '\0') {
		pw = getpwnam(p);
		if (pw != NULL && pw->pw_uid == getuid())
			return p;
	}

	/* Try to get the password file entry for getuid() */
	if ((pw = getpwuid(getuid())) != NULL)
		return pw->pw_name;
	
	/* Give up */
	return NULL;
}
