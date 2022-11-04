/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:print/fprintf.c	1.13"
/*LINTLIBRARY*/
#include "synonyms.h"
#include "shlib.h"
#include <stdio.h>
#include <stdarg.h>

extern int _doprnt();

/*VARARGS2*/
int
#ifdef __STDC__
fprintf(FILE *iop, const char *format, ...)
#else
fprintf(iop, format, va_alist) FILE *iop; char *format; va_dcl
#endif
{
	register int count;
	va_list ap;

#ifdef __STDC__
	va_start(ap,);
#else
	va_start(ap);
#endif
	if (!(iop->_flag & _IOWRT)) {
		/* if no write flag */
		if (iop->_flag & _IORW) {
			/* if ok, cause read-write */
			iop->_flag |= _IOWRT;
		} else {
			/* else error */
			return EOF;
		}
	}
	count = _doprnt(format, ap, iop);
	va_end(ap);
	return(ferror(iop)? EOF: count);
}
