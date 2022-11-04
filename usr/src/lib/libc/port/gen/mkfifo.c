/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/mkfifo.c	1.1"
/*
 * mkfifo(3c) - create a named pipe (FIFO). This code provides
 * a POSIX mkfifo function.
 *
 */

#ifdef __STDC__
	#pragma weak mkfifo = _mkfifo
#endif
#include "synonyms.h"
mkfifo(path,mode)
char *path;
int mode;
{
	mode &= 0777;		/* only allow file access permissions */
	mode |= 0010000;	/* creating a FIFO 		      */
	return(mknod(path,mode,0));
}
