/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:fp/scalb.c	1.5"
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
#include <limits.h>

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
	if (x == 0.0)
		return x;
	if ((n >= (double)INT_MAX) || (n <= (double)INT_MIN)) {
	/* over or underflow */
		double d;
		errno = ERANGE;
		if (x < 0.0)
			d = -x;
		else
			d = x;
		if (((d < 1.0)  && (n > 0)) ||
			((d >= 1.0) && (n < 0)))
			return 0.0;  /* underflow */
		else {
			if (_lib_version == c_issue_4)
				return(x > 0.0 ? HUGE : -HUGE);
			else
				return(x > 0.0 ? HUGE_VAL : -HUGE_VAL);
		}
	}
	return(ldexp(x, (int)n));
}
