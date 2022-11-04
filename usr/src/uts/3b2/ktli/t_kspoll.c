/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktli:ktli/t_kspoll.c	1.6"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)t_kspoll.c 1.3 89/01/27 SMI"
#endif

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

/*
 *	This function waits for timo clock ticks for something
 *	to arrive on the specified stream. If more than one client is
 *	hanging off of a single endpoint, and at least one has specified
 *	a non-zero timeout, then all will be woken.
 *
 *	Returns:
 *		-1	on failure
 *		 0	on timeout or no events(timout = 0),
 *		 1	if desired event has occurred
 *
 *	Most of the code is from strwaitq().
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/stream.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/strsubr.h>
#include <sys/tihdr.h>
#include <sys/timod.h>
#include <sys/tiuser.h>
#include <sys/t_kuser.h>
#include <sys/signal.h>
#include <sys/siginfo.h>
#include <sys/debug.h>

STATIC void ktli_poll();

int 
t_kspoll(tiptr, timo, waitflg)
register TIUSER *tiptr;
register int  waitflg;
register int timo;

{
	register int s;
	register int errs;
	register struct file *fp;
	register struct vnode *vp;
	register struct stdata *stp;
	register int timeid;
	k_sigset_t	sig;
	sigqueue_t	*sigqueue = NULL;
	char		cursig = 0;
	sigqueue_t	*curinfo = NULL;
	proc_t		*p = u.u_procp;

	fp = tiptr->fp;
	vp = fp->f_vnode;
	stp = vp->v_stream;

again:

	if (waitflg != READWAIT) {
		u.u_error = EINVAL;
		return -1;
	}

	errs = STRDERR|STPLEX;
	if (stp->sd_flag & errs) {
		u.u_error = (stp->sd_flag & STPLEX) ? EINVAL : stp->sd_rerror;
		return -1;
	}

	if ((RD(stp->sd_wrq))->q_first != NULL)
		return 1;

	if (timo == 0)
		return 0;

	/* set timer and sleep.
	 */
	if (timo > 0) {
		if ((timeid = timeout(ktli_poll, tiptr, timo)) < 0) {
			printf("t_kspoll: Can't set timer\n");
			return -1;
		}
	}
	s = splstr();
	stp->sd_flag |= RSLEEP;
	/* remove any pending signals before sleep */
	if (p->p_cursig) {
		sigemptyset (&sig);
		cursig = p->p_cursig;
		curinfo = p->p_curinfo;
		p->p_cursig = 0;
		p->p_curinfo = NULL;
		if (!sigisempty(&p->p_sig)) {
			sigqueue = sigappend (&sig, sigqueue, &p->p_sig, p->p_sigqueue);
			sigemptyset (&p->p_sig);
			p->p_sigqueue = NULL;
		}
	}
	if (sleep((caddr_t)RD(stp->sd_wrq), STIPRI|PCATCH)) {
		/* don't need to restore saved signal if there wasn't one */
		if (cursig) {
			/*
			 * restore any saved signals before the new signal:
			 * 1) put the new sig at the head of any new queue
			 * 2) restore the old signal info before any new ones
			 */
			ASSERT(p->p_curinfo);
			ASSERT(p->p_cursig);
			sigaddset (&p->p_sig, p->p_cursig);
			if (p->p_sigqueue) {
				p->p_curinfo->sq_next = p->p_sigqueue;
			}
			p->p_sigqueue = p->p_curinfo;
			p->p_cursig = cursig;
			p->p_curinfo = curinfo;
			if (!sigisempty(&sig)) {
				p->p_sigqueue = sigprepend (&p->p_sig,
					p->p_sigqueue, &sig, sigqueue);
			}
		}

		if (timo > 0)
			untimeout(timeid);
		/* only unset RSLEEP if no other procs are sleeping on this stream */
		if (fp->f_count <= 1)		/* shouldn't ever be < 1 */
			stp->sd_flag &= ~RSLEEP;
		(void) splx(s);
		u.u_error = EINTR;
		return -1;
	}
	/* restore any saved signals */
	if (cursig) {
		/*
		 * We should not have any new signals, but there may be a race
		 * here - a signal may come in after the sleep return but
		 * before we get here. So the assertions are not safe.
		 * XXX Maybe not, since we are splstr'd. Is splstr high enough?
		 */
		ASSERT(!p->p_cursig);
		ASSERT(p->p_curinfo == NULL);
		p->p_cursig = cursig;
		p->p_curinfo = curinfo;
		if (!sigisempty(&sig)) {
			p->p_sigqueue = sigprepend (&p->p_sig, p->p_sigqueue,
				&sig, sigqueue);
		}
	}
#ifdef KTLIDEBUG
printf("t_kspoll: pid %d, woken from sleep\n", u.u_procp->p_pid);
#endif
	if (timo > 0)
		untimeout(timeid);
	stp->sd_flag &= ~RSLEEP;
	(void)splx(s);

	/* see if the timer expired
	 */
	if (tiptr->flags & TIME_UP) {
		tiptr->flags &= ~ TIME_UP;
		return 0;
	}
	
	goto again;
}



STATIC void
ktli_poll(tiptr)
register TIUSER *tiptr;

{
	register struct vnode *vp;
	register struct file *fp;
	register struct stdata *stp;

	fp = tiptr->fp;
	vp = fp->f_vnode;
	stp = vp->v_stream;

	tiptr->flags |= TIME_UP;
	wakeup((caddr_t)RD(stp->sd_wrq));
}

/******************************************************************************/




