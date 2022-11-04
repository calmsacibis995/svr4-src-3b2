/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getpwuid.c	1.13"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
#ifdef __STDC__
	#pragma weak getpwuid = _getpwuid
#endif
#include "synonyms.h"
#include <pwd.h>
#include <stddef.h>
#include <sys/types.h>

extern struct passwd *getpwent();
extern void setpwent(), endpwent();

struct passwd *
getpwuid(uid)
register uid_t uid;
{
	register struct passwd *p;

	setpwent();
	while((p = getpwent()) != NULL && p->pw_uid != uid)
		;
	endpwent();
	return(p);
}
