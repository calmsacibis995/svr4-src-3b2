/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/tell.c	1.9"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
/*
 * return offset in file.
 */
#ifdef __STDC__
	#pragma weak tell = _tell
#endif
#include "synonyms.h"

extern long lseek();

long
tell(f)
int	f;
{
	return(lseek(f, 0L, 1));
}
