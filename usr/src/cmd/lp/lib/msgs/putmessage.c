/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/msgs/putmessage.c	1.7"
/* LINTLIBRARY */

#if	defined(__STDC__)
# include	<stdarg.h>
#else
# include	<varargs.h>
#endif

/* VARARGS */
#if	defined(__STDC__)
int putmessage(char * buf, short type, ... )
#else
int putmessage(buf, type, va_alist)
    char	*buf;
    short	type;
    va_dcl
#endif
{
    int		size;
    va_list	arg;
    int		_putmessage();

#if	defined(__STDC__)
    va_start(arg, type);
#else
    va_start(arg);
#endif

    size = _putmessage(buf, type, arg);
    va_end(arg);
    return(size);
}
