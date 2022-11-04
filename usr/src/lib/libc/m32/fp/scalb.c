/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:fp/scalb.c	1.4"
/*LINTLIBRARY

/* SCALB(X,N)
 * return x * 2**N without computing 2**N - this is the standard
 * C library ldexp() routine except that signaling NANs generate
 * invalid op exception - errno = EDOM
 */

#ifdef __STDC__
	#pragma weak scalb = _scalb
#endif
#include "synonyms.h"
#include <values.h>
#include <math.h>
#include <errno.h>
#include "fpparts.h"

double scalb(x,n)
double	x, n;
{
#if _IEEE
	if ((EXPONENT(x) == MAXEXP) && !QNANBIT(x) && (HIFRACTION(x)
		|| LOFRACTION(x)) ) {
		errno = EDOM;
		return (x + 1.0); /* signaling NaN - raise exception */
	}
#endif
	return(ldexp(x, (int)n));
}
