/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:gen/getctxt.c	1.2"

#ifdef __STDC__
	#pragma weak getcontext = _getcontext
#endif
#include "synonyms.h"
#include <ucontext.h>

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
getcontext(ucp)
ucontext_t *ucp;
{
	register greg_t *reg;
	int error;

	ucp->uc_flags = UC_ALL;
	if (error = __getcontext(ucp))
		return error;

	reg = ucp->uc_mcontext.gregs;
	reg[R_FP] = getfp()-(7*sizeof(greg_t));
	reg[R_AP] = getfp()-(8*sizeof(greg_t));
	reg[R_PC] = getfp()-(9*sizeof(greg_t));
	reg[R_SP] = getap();

	return 0;
}
