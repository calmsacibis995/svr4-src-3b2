/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getgrgid.c	1.12"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
#ifdef __STDC__
	#pragma weak getgrgid = _getgrgid
#endif
#include "synonyms.h"
#include "shlib.h"
#include <grp.h>
#include <stddef.h>
#include <sys/types.h>

extern struct group *getgrent();
extern void setgrent(), endgrent();

struct group *
getgrgid(gid)
register uid_t gid;
{
	register struct group *p;

	setgrent();
	while((p = getgrent()) != NULL && p->gr_gid != gid)
		;
	endgrent();
	return(p);
}
