/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:sys/execl.c	1.5.1.5"
/*
 *	execl(name, arg0, arg1, ..., argn, 0)
 *	environment automatically passed.
 */

#ifdef __STDC__
	#pragma weak execl = _execl
#endif
#include "synonyms.h"

extern int execve();

execl(name, args)
char *name, *args;
{
	extern char **environ;

	return (execve(name, &args, environ));
}
