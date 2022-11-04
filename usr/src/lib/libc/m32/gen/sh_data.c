/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:gen/sh_data.c	1.1"

/* this file includes definitions of data so that the
 * dynamic shared library will build. The actual
 * definitions are contained in other source files.
 */
#ifdef DSHLIB
int	_environ;
#endif
