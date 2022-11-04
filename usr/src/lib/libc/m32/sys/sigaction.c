/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-m32:sys/sigaction.c	1.4"

#ifdef __STDC__ 
	#pragma weak sigaction = _sigaction
#endif
#include "synonyms.h"
#include <signal.h>
#include <errno.h>
#include <siginfo.h>
#include <ucontext.h>

extern void (*_siguhandler[])();

static void
sigacthandler(sig, sip, uap)
	int sig;
	siginfo_t *sip;
	ucontext_t *uap;
{
	(*_siguhandler[sig])(sig, sip, uap);
	setcontext(uap);
}
	
sigaction(sig, nact, oact)
	int sig;
	struct sigaction *nact, *oact;
{
	struct sigaction tact;
	void (*ohandler)();

	if (sig <= 0 || sig >= NSIG) {
		errno = EINVAL;
		return -1;
	}

	ohandler = _siguhandler[sig];

	if (nact) {
		tact = *nact;
		nact = &tact;
		_siguhandler[sig] = nact->sa_handler;
		if (nact->sa_handler != SIG_DFL && nact->sa_handler != SIG_IGN)
			nact->sa_handler = sigacthandler;
	}

	if (__sigaction(sig, nact, oact) == -1) {
		_siguhandler[sig] = ohandler;
		return -1;
	}

	if (oact && oact->sa_handler != SIG_DFL && oact->sa_handler != SIG_IGN)
		oact->sa_handler = ohandler;
	
	return 0;
}
