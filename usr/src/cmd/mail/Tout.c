/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:mail/Tout.c	1.5"
#ident "@(#)Tout.c	2.5 'attmail mail(1) command'"
/*
    NAME
	Tout - Print surrogate debug output

    SYNOPSIS
	void Tout(char *subname, char *msg, ...)

    DESCRIPTION
	Tout prints debugging output if surrogate tracing
	has been turned on (-T specified). The message will
	also go to the debug output if debugging is turned
	on (-x specified). The subroutine name is printed
	if it is not a null string.
*/
#include "mail.h"
#ifdef __STDC__
# include <stdarg.h>
#else
# include <varargs.h>
#endif

/* VARARGS2 PRINTFLIKE2 */
void
#ifdef __STDC__
Tout(char *subname, char *fmt, ...)
#else
# ifdef lint
Tout(Xsubname, Xfmt, va_alist)
char *Xsubname, *Xfmt;
va_dcl
# else
Tout(va_alist)
va_dcl
# endif
#endif
{
#ifndef __STDC__
        char    *subname;
        char    *fmt;
#endif
        va_list args;

#if !defined(__STDC__) && defined(lint)
	subname = Xsubname;
	fmt = Xfmt;
#endif

        if (debug > 0) {
#ifdef __STDC__
                va_start(args, fmt);
#else
                va_start(args);
                subname = va_arg(args, char *);
                fmt = va_arg(args, char *);
#endif
                if (subname && *subname) {
                        fprintf(dbgfp,"%s(): ", subname);
                }
                vfprintf(dbgfp, fmt, args);
                va_end(args);
        }

        if (flgT) {
#ifdef __STDC__
                va_start(args, fmt);
#else
                va_start(args);
                subname = va_arg(args, char *);
                fmt = va_arg(args, char *);
#endif
                vfprintf(stdout, fmt, args);
                va_end(args);
        }
}
