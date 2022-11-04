/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getpwnam.c	1.16"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
#ifdef __STDC__
	#pragma weak getpwnam = _getpwnam
#endif
#include "synonyms.h"
#include <pwd.h>
#include <stddef.h>
#include <string.h>

extern struct passwd *getpwent();
extern void setpwent(), endpwent();

struct passwd *
getpwnam(name)
const char	*name;
{
	register struct passwd *p;

	setpwent();
	while ((p = getpwent()) != NULL && strcmp(name, p->pw_name))
		;
	endpwent();
	return (p);
}
