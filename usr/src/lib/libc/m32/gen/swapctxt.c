/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:gen/swapctxt.c	1.2"

#ifdef __STDC__
	#pragma weak swapcontext = _swapcontext
#endif
#include "synonyms.h"
#include <ucontext.h>
#include <stdlib.h>

asm greg_t
getfp()
{
	MOVW	%fp,%r0
}

asm greg_t
getap()
{
	MOVW	%ap,%r0
}

int
swapcontext(oucp, nucp)
ucontext_t *oucp, *nucp;
{
	register greg_t *reg;
	int rval;

	if (rval = __getcontext(oucp))
		return rval;

	reg = oucp->uc_mcontext.gregs;
	reg[R_FP] = getfp()-(sizeof(greg_t)*7);	/* get old fp off stack */
	reg[R_AP] = getfp()-(sizeof(greg_t)*8);	/* get old ap off stack */
	reg[R_PC] = getfp()-(sizeof(greg_t)*9);	/* get old pc off stack */
	reg[R_SP] = getap();			/* reset sp */

	return setcontext(nucp);
}
