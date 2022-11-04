/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:sys/execle.c	1.6.1.4"
/*
 *	execle(file, arg1, arg2, ..., 0, envp)
 */
#ifdef __STDC__
	#pragma weak execle = _execle
#endif
#include "synonyms.h"

extern int execve();

execle(file, args)
	char	*file;
	char	*args;			/* first arg */
{
	register  char  **p;

	p = &args;
	while(*p++);
	return(execve(file, &args, *p));
}
