/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:fp/fp_data.c	1.8"
/*
 * contains the definitions
 * of the global symbols used
 * by the floating point environment
 */
#include "synonyms.h"

#ifdef __STDC__
#include <math.h>
const _h_val __huge_val =
#else
unsigned long __huge_val[sizeof(double)/sizeof(unsigned long)] =
#endif /* __STDC__ */
/* IEEE infinity */
#if u3b || u3b2 | u3b5 | u3b15
	{ 0x7ff00000,0x0 };
#elif i386
	{ 0x0,0x7ff00000 };
#endif
