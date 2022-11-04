/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:gen/sigsetjmp.c	1.2"

#ifdef __STDC__
	#pragma weak sigsetjmp = _sigsetjmp
	#pragma weak siglongjmp = _siglongjmp
#endif
#include "synonyms.h"
#include <ucontext.h>
#include <setjmp.h>

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
sigsetjmp(env, savemask)
sigjmp_buf env;
int savemask;
{
	register ucontext_t *ucp;
	register greg_t *reg;

	ucp = (ucontext_t *)env;

	ucp->uc_flags = UC_ALL;

	__getcontext(ucp);

	if (!savemask)
		ucp->uc_flags &= ~UC_SIGMASK;

	reg = ucp->uc_mcontext.gregs;
	reg[R_FP] = getfp()-(7*sizeof(greg_t));	/* get old fp off stack */
	reg[R_AP] = getfp()-(8*sizeof(greg_t));	/* get old ap off stack */
	reg[R_PC] = getfp()-(9*sizeof(greg_t));	/* get old pc off stack */
	reg[R_SP] = getap();			/* reset sp */

	return 0;
}

void 
siglongjmp(env,val)
sigjmp_buf env;
int val;
{
	register ucontext_t *ucp = (ucontext_t *)env;
	if (val)
		ucp->uc_mcontext.gregs[R_R0] = (greg_t)val;
	else
		ucp->uc_mcontext.gregs[R_R0] = (greg_t)1;
	setcontext(ucp);
}
