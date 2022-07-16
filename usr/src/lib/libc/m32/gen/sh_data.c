/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:gen/sh_data.c	1.3"

#ifdef DSHLIB
int	_environ;
#else
#ifdef __STDC__
	#pragma weak environ = _environ
int	_environ = 0;
#else
int	environ = 0;
#endif
#endif
