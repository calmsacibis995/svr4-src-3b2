/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:smtp/src/s5mboxown.c	1.3"
#ident "@(#)s5mboxown.c	1.3 'attmail mail(1) command'"
/*
 *	store mbox owner through pointer.  return 0 (success), -1 (failure)
 *	hard version for systems that let you give files away;
 *	the owner of a mbox is determined from its name -- unless
 *	the set-uid bit is on (!) .  In that case, the file must have
 *	been created by its present owner, else anyone could become root.
 *	If the set-uid bit is on, return the file owner and group.
 */

#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "s_string.h"
#include "aux.h"
#include "ugid.h"
#include "xmail.h"

mboxowner (mbox, ugidp)
	char *mbox;
	struct ugid *ugidp;
{
	char *u;
	struct passwd *pw, *getpwnam();
	struct stat statb;

	if (stat (mbox, &statb) >= 0 && (statb.st_mode & S_ISUID)) {
		ugidp->uid = statb.st_uid;
		ugidp->gid = statb.st_gid;
		return 0;
	}

	u = basename(mbox);
	if (u == NULL)
		return -1;

	pw = getpwnam(u);
	if (pw == NULL)
		return -1;

	ugidp->uid = pw->pw_uid;
	ugidp->gid = pw->pw_gid;
	return 0;
}
