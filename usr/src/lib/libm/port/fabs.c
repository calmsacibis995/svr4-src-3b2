/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/fabs.c	1.3"
/*LINTLIBRARY*/
/*
 *	fabs returns the absolute value of its double-precision argument.
 */

#include <values.h>
#include "fpparts.h"

double
fabs(x)
double x;
{

#if _IEEE
	SIGNBIT(x) = 0;
	return x;
#else
	return (x < 0 ? -x : x);
#endif
}

/* COPYSIGN(X,Y)
 * Return x with the sign of y  - no exceptions are raised
 */

#ifdef __STDC__
#pragma weak copysign = _copysign
#endif

double _copysign(x,y)
double	x, y;
{
#if _IEEE
	SIGNBIT(x) = SIGNBIT(y);
	return x ;
#else
	if (y >= 0.0)
		return(x >= 0.0 ? x : -x);
	else 
		return(x >= 0.0 ? -x : x);
#endif
}
