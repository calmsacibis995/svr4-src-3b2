/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/strumount.c	1.1"
/*
 * Unmount a file descriptor from a file in the file system.
 */
#ifdef __STDC__
	#pragma weak strumount = _strumount
#endif
#include "synonyms.h"
#include <sys/errno.h>
#include <stdio.h>

int
strumount(path)
	char *path;
{
	return (funmount(path));
}
	
