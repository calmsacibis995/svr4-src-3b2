/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:islocal.c	1.4"
#ident "@(#)islocal.c	1.5 'attmail mail(1) command'"
#include "mail.h"

/*
 * islocal (char *user, uid_t *puid) - see if user exists on this system
 */
islocal(user, puid)
char *user;
uid_t *puid;
{
	char	fname[80];
	struct stat statb;
	struct passwd *pwd_ptr;

	/* Check for existing mailfile first */
	sprintf(fname,"%s%s", maildir, user);
	if (stat(fname,&statb) == 0) {
		*puid = statb.st_uid;
		return (TRUE);
	}

	/* If no existing mailfile, check passwd file */
	setpwent();	
	if ((pwd_ptr = getpwnam(user)) == NULL) {
		return(FALSE);
	}
	*puid = pwd_ptr->pw_uid;
	return (TRUE);
}
