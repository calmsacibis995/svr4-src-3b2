/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:gen/makectxt.c	1.4"

#ifdef __STDC__
	#pragma weak makecontext = _makecontext
#endif
#include "synonyms.h"
#include <stdarg.h>
#include <ucontext.h>

void
#ifdef __STDC__
makecontext(ucontext_t *ucp, void (*func)(), int argc, ...)
#else
makecontext(ucp, func, argc, va_alist)
ucontext_t *ucp;
void (*func)();
int argc;
va_dcl
#endif
{

	register greg_t *reg;
	int *sp;
	va_list ap;

	reg = ucp->uc_mcontext.gregs;
	reg[R_PC] = (greg_t)func;

	sp = (int *)ucp->uc_stack.ss_sp;
	*sp++ = (int)ucp->uc_link;

	reg[R_AP] = (greg_t)sp;

#ifdef __STDC__
	va_start(ap, ...);
#else
	va_start(ap, va_alist);
#endif
	while (argc--)
		*sp++ = va_arg(ap, int);

	*sp++ = (int)setcontext;		/* return pc */
	*sp++ = (int)(ucp->uc_stack.ss_sp);	/* return ap */

	reg[R_SP] = (greg_t)sp;
}
