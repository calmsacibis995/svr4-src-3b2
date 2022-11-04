/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/sig.c	1.49.2.1"
#include "sys/param.h"
#include "sys/types.h"
#include "sys/psw.h"
#include "sys/sysmacros.h"
#include "sys/immu.h"
#include "sys/pcb.h"
#include "sys/systm.h"
#include "sys/sbd.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/proc.h"
#include "sys/signal.h"
#include "sys/siginfo.h"
#include "sys/fault.h"
#include "sys/syscall.h"
#include "sys/ucontext.h"
#include "sys/procfs.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/reg.h"
#include "sys/var.h"
#include "sys/debug.h"
#include "sys/rf_messg.h"
#include "sys/inline.h"
#include "sys/uio.h"
#include "sys/wait.h"
#include "sys/class.h"
#include "sys/disp.h"
#include "sys/mman.h"
#include "sys/procset.h"
#include "sys/kmem.h"
#include "sys/evecb.h"
#include "sys/hrtcntl.h"
#include "sys/events.h"
#include "sys/evsys.h"

#include "vm/as.h"

#ifdef KPERF
#include "sys/disp.h" 		/* used when compiled with -DKPER */
#endif

k_sigset_t fillset = 0x7fffffff;	/* MUST be contiguous */

k_sigset_t cantmask = (sigmask(SIGKILL)|sigmask(SIGSTOP));

k_sigset_t cantreset = (sigmask(SIGILL)|sigmask(SIGTRAP)|sigmask(SIGPWR));

k_sigset_t ignoredefault = (sigmask(SIGCONT)|sigmask(SIGCLD)|sigmask(SIGPWR)
			|sigmask(SIGWINCH)|sigmask(SIGURG));

k_sigset_t stopdefault = (sigmask(SIGSTOP)|sigmask(SIGTSTP)
			|sigmask(SIGTTOU)|sigmask(SIGTTIN));

k_sigset_t coredefault = (sigmask(SIGQUIT)|sigmask(SIGILL)|sigmask(SIGTRAP)
			|sigmask(SIGIOT)|sigmask(SIGEMT)|sigmask(SIGFPE)
			|sigmask(SIGBUS)|sigmask(SIGSEGV)|sigmask(SIGSYS)
			|sigmask(SIGXCPU)|sigmask(SIGXFSZ));

k_sigset_t holdvfork = (sigmask(SIGTTOU)|sigmask(SIGTTIN)|sigmask(SIGTSTP));

#define tracing(p, sig) \
	(((p)->p_flag & STRC) || sigismember(&(p)->p_sigmask, (sig)))

STATIC int isjobstop();
int fsig();
STATIC int procxmt();

/*
 * Send the specified signal to the specified process.
 */

void
psignal(p, sig)
	register proc_t *p;
	register int sig;
{
	sigtoproc(p, sig, 0);
}

void
sigtoproc(p, sig, fromuser)
	register proc_t *p;
	register int sig;
	register int fromuser;
{
	if (sig <= 0 || sig >= NSIG)
		return;

	ev_gotsig(p, sig, fromuser);

	if (sig == SIGCONT) {
		sigdiffset(&p->p_sig, &stopdefault);
		if (p->p_stat == SSTOP && p->p_whystop == PR_JOBCONTROL) {
			p->p_flag |= SXSTART;
			setrun(p);
		} 
	} else if (sigismember(&stopdefault, sig))
		sigdelset(&p->p_sig, SIGCONT);

	if (!tracing(p, sig) && sigismember(&p->p_ignore, sig))
		return;

	sigaddset(&p->p_sig, sig);

	if (p->p_stat == SSLEEP) {
		if ((p->p_flag & SNWAKE) 
		  || (sigismember(&p->p_hold, sig) && !EV_ISTRAP(p)))
			return;
		setrun(p);
	} else if (p->p_stat == SSTOP) {
		if (sig == SIGKILL) {
			p->p_flag |= SXSTART|SPSTART;
			setrun(p);
		} else if (p->p_wchan && ((p->p_flag & SNWAKE) == 0))
			/*
			 * If process is in the sleep queue at an
			 * interruptible priority but is stopped,
			 * remove it from the sleep queue but don't
			 * set it running yet. The signal will be
			 * noticed when the process is continued.
			 */
			unsleep(p);
	} else if (p == curproc) {

		/*
		 * If the process is the current process we set the
		 * u_sigevpend flag to ensure that signals posted to the
		 * current process from interrupt level are received by
		 * the process before it returns to user mode.
		 */
		u.u_sigevpend = 1;
	}
}

STATIC int
isjobstop(sig)
	register int sig;
{
	register proc_t *p = u.u_procp;

	if (u.u_signal[sig-1] == SIG_DFL && sigismember(&stopdefault, sig)) {
		/*
		 * If SIGCONT has been posted since we promoted this signal
		 * from pending to current, then don't do a jobcontrol stop.
		 * Also discard SIGCONT, since it would not have been sent
		 * if a debugger had not been holding the process stopped.
		 */
		if (sigismember(&p->p_sig, SIGCONT)) {
			sigdelset(&p->p_sig, SIGCONT);
			sigdelq(p, SIGCONT);
		} else if (sig == SIGSTOP || !(p->p_flag & SDETACHED)) {
			if (stop(p, PR_JOBCONTROL, sig, 0))
				swtch();
			p->p_wcode = CLD_CONTINUED;
			p->p_wdata = SIGCONT;
			sigcld(p);
		}
		return 1;
	}
	return 0;
}


/*
 * Returns true if the current process has a signal to process, and
 * the signal is not held.  The signal to process is put in p_cursig.
 * This is asked at least once each time a process enters the system
 * (though this can usually be done without actually calling issig by
 * checking the pending signal masks).  A signal does not do anything
 * directly to a process; it sets a flag that asks the process to do
 * something to itself.
 *
 * The "why" argument indicates the allowable side-effects of the call:
 *
 * FORREAL:  Extract the next pending signal from p_sig into p_cursig;
 * stop the process if a stop has been requested or if a traced signal
 * is pending.
 *
 * JUSTLOOKING:  Don't stop the process, just indicate whether or not
 * a signal is pending.
 */
int
issig(why)
	int why;
{
	register int sig;
	register proc_t *p = u.u_procp;

	u.u_sigevpend = 0;

	for (;;) {
		/*
		 * Honor requested stop before dealing with the
		 * current signal; a debugger may change it.
		 */
		if (why == FORREAL
		  && (p->p_flag & SPRSTOP)
		  && stop(p, PR_REQUESTED, 0, 0))
			swtch();

		/*
		 * If a debugger wants us to take a signal it will
		 * have left it in p->p_cursig.  If p_cursig has been
		 * cleared or if it's being ignored, we continue on
		 * looking for another signal.  Otherwise we return
		 * the specified signal, provided it's not a signal
		 * that causes a job control stop.
		 *
		 * When stopped on PR_JOBCONTROL, there is no current
		 * signal; we cancel p->p_cursig temporarily before
		 * calling isjobstop().  The current signal may be reset
		 * by a debugger while we are stopped in isjobstop().
		 */
		if ((sig = p->p_cursig) != 0) {
			p->p_cursig = 0;
			if (why == JUSTLOOKING
			  || (p->p_flag & SPTRX)
			  || (!sigismember(&p->p_ignore, sig)
			    && !isjobstop(sig)))
				return p->p_cursig = (char)sig;
			/*
			 * The signal is being ignored or it caused a
			 * job-control stop.  If another current signal
			 * has not been established, return the current
			 * siginfo, if any, to the memory manager.
			 */
			if (p->p_cursig == 0 && p->p_curinfo != NULL) {
				kmem_free((caddr_t)p->p_curinfo,
				  sizeof(*p->p_curinfo));
				p->p_curinfo = NULL;
			}
			/*
			 * Loop around again in case we were stopped
			 * on a job control signal and a /proc stop
			 * request was posted or another current signal
			 * was established while we were stopped.
			 */
			continue;
		}

		/*
		 * Loop on the pending signals until we find a 
		 * non-held signal that is traced or not ignored.
		 */
		for (;;) {
			if ((sig = fsig(p)) == 0)
				return 0;
			if (tracing(p, sig)
			  || !sigismember(&p->p_ignore, sig)) {
				if (why == JUSTLOOKING)
					return sig;
				break;
			}
			sigdelset(&p->p_sig, sig);
			sigdelq(p, sig);
		}

		/*
		 * Promote the signal from pending to current.
		 *
		 * Note that sigdeq() will set p->p_curinfo to NULL
		 * if no siginfo_t exists for this signal.
		 */
		sigdelset(&p->p_sig, sig);
		p->p_cursig = (char)sig;
		ASSERT(p->p_curinfo == NULL);
		sigdeq(p, sig, &p->p_curinfo);

		/*
		 * If tracing, stop.  If tracing via ptrace(2), call
		 * procxmt() repeatedly until released by debugger.
		 */
		if (tracing(p, sig)) {
			int firststop = 1;

			do  {
				if ((p->p_flag & STRC) && p->p_ppid == 1) {
					p->p_flag |= SPTRX;
					break;
				}
				if (!stop(p, PR_SIGNALLED, sig, firststop))
					break;
				swtch();
				firststop = 0;
			} while ((p->p_flag & STRC) && !procxmt());

			if ((p->p_flag & SPTRX) && p->p_cursig == 0)
				p->p_cursig = SIGKILL;
		}

		/*
		 * Loop around to check for requested stop before
		 * performing the usual current-signal actions.
		 */
	}
}

/*
 * Put the specified process into the stopped state and notify
 * tracers via wakeup().  Returns 0 if process can't be stopped.
 * Returns non-zero in the normal case.
 */
int
stop(p, why, what, firststop)
	register proc_t *p;
	register why, what;
	int firststop;		/* only for SIGNALLED stop */
{

	/*
	 * Don't stop a process with SIGKILL pending and
	 * don't stop one undergoing a ptrace(2) exit.
	 */
	if (p->p_cursig == SIGKILL
	  || sigismember(&p->p_sig, SIGKILL)
	  || (p->p_flag & SPTRX))
		return 0;

	(void)dispdeq(p);
	p->p_stat = SSTOP;
	p->p_flag &= ~(SPSTART|SXSTART);
	p->p_whystop = (short)why;
	p->p_whatstop = (short)what;
	CL_STOP(p, p->p_clproc, why, what);

	switch (why) {
	case PR_JOBCONTROL:
		p->p_flag |= SPSTART;
		p->p_wcode = CLD_STOPPED;
		p->p_wdata = what;
		sigcld(p);
		break;
	case PR_SIGNALLED:
		if (p->p_flag & STRC) {		/* ptrace() */
			p->p_wcode = CLD_TRAPPED;
			p->p_wdata = what;
			wakeup((caddr_t)p->p_parent);
			if (!firststop || !sigismember(&p->p_sigmask, what)) {
				p->p_flag |= SPSTART;
				break;
			}
		} else
			p->p_flag |= SXSTART;
		/* fall through */
	default:
		if (why != PR_SIGNALLED)
			p->p_flag |= SXSTART;
		p->p_flag &= ~SPRSTOP;
		if (p->p_trace)			/* /proc */
			wakeup((caddr_t)p->p_trace);
		break;
	}

	return 1;
}

/*
 * Perform the action specified by the current signal.
 * The usual sequence is:
 * 	if (issig())
 * 		psig();
 * The signal bit has already been cleared by issig(),
 * the current signal number has been stored in p->p_cursig,
 * and the current siginfo is now referenced by p->p_curinfo.
 */

void
psig()
{
	register proc_t *p = u.u_procp;
	register int sig = p->p_cursig;
	void (*func)();
	int rc, code;

	code = CLD_KILLED;

	/*
	 * A pending SIGKILL overrides the current signal.
	 */
	if (sigismember(&p->p_sig, SIGKILL))
		sig = p->p_cursig = SIGKILL;

	/*
	 * Exit immediately on a ptrace exit request.
	 */
	if (p->p_flag & SPTRX) {
		p->p_flag &= ~SPTRX;
		if (sig == 0)
			sig = p->p_cursig = SIGKILL;
	} else {
		ASSERT(sig);
		ASSERT(!sigismember(&p->p_ignore, sig));
		if ((func = u.u_signal[sig-1]) != SIG_DFL) {
			if (u.u_sigflag & SOMASK) 
				u.u_sigflag &= ~SOMASK;
			else 
				u.u_sigoldmask = p->p_hold;
			sigorset(&p->p_hold, &u.u_sigmask[sig-1]);
			if (!sigismember(&u.u_signodefer, sig))
				sigaddset(&p->p_hold, sig);
			if (sigismember(&u.u_sigresethand, sig))
				setsigact(sig, SIG_DFL, 0, 0);
			rc = sendsig(sig, func);
			p->p_cursig = 0;
			if (p->p_curinfo) {
				kmem_free((caddr_t)p->p_curinfo,
				  sizeof(*p->p_curinfo));
				p->p_curinfo = NULL;
			}
			if (rc)
				return;
			sig = p->p_cursig = SIGSEGV;
		}
		if (sigismember(&coredefault, sig)) {
			if (core("core", u.u_procp, u.u_cred,
			   u.u_rlimit[RLIMIT_CORE].rlim_cur, sig) == 0)
				code = CLD_DUMPED;
		}
	}
	p->p_cursig = 0;
	if (p->p_curinfo) {
		kmem_free((caddr_t)p->p_curinfo, sizeof(*p->p_curinfo));
		p->p_curinfo = NULL;
	}
	exit(code, sig);
}

/*
 * Find the next unheld signal in bit-position representation in p_sig.
 */

int
fsig(p)
	register proc_t *p;
{
	register i;
	k_sigset_t temp;

	temp = p->p_sig;
	sigdiffset(&temp, &p->p_hold);
	if (p->p_flag & SVFORK)
		sigdiffset(&temp, &holdvfork);
	if (!sigisempty(&temp)) {
		if (sigismember(&temp, SIGKILL))
			return SIGKILL;
		for (i = 1; i < NSIG; i++) {
			if (sigismember(&temp, i))
				return i;
		}
	}
	return 0;
}

/*
 * sys-trace system call.
 */
struct ptracea {
	int	req;
	int	pid;
	int	*addr;
	int	data;
};

/*
 * Priority for tracing
 */
#define	IPCPRI	PZERO

/*
 * Tracing variables.  Used to pass trace command from parent
 * to child being traced.  This data base cannot be shared and
 * is locked per user.
 */
STATIC struct {
	int	ip_lock;
	int	ip_req;
	int	*ip_addr;
	int	ip_data;
} ipc;

int
ptrace(uap, rvp)
	register struct ptracea *uap;
	rval_t *rvp;
{
	register struct proc *p;
	int error = 0;

	if (uap->req <= 0) {
		u.u_procp->p_flag |= STRC;
		/*
		 * Disable possible fast interface to illegal
		 * op-code handler.
		 */
		u.u_iop = NULL;
		return 0;
	}

    again:
	for (p = u.u_procp->p_child; p; p = p->p_sibling)
		if (p->p_pid == uap->pid
		  && p->p_stat == SSTOP
		  && (p->p_flag & (STRC|SXSTART)) == STRC
		  && p->p_whystop == PR_SIGNALLED)
			goto found;
	return ESRCH;

    found:
	/*
	 * Wait until /proc has started the process, otherwise we would
	 * sleep at high priority with ipc locked.  The SPSTART bit is
	 * always set if /proc is not controlling the process.
	 * If another ptrace() controller has the ipc locked, wait
	 * for it at interruptible priority.
	 */
	if ((p->p_flag & SPSTART) == 0 || ipc.ip_lock) {
		while ((p->p_flag & SPSTART) == 0)
			sleep((caddr_t)u.u_procp, PZERO+1);
		while (ipc.ip_lock)
			sleep((caddr_t)&ipc, PZERO+1);
		/*
		 * Go find the process again.  It may have disappeared.
		 */
		goto again;
	}

	ipc.ip_lock = p->p_pid;
	ipc.ip_data = uap->data;
	ipc.ip_addr = uap->addr;
	ipc.ip_req = uap->req;
	p->p_flag |= SXSTART;
	setrun(p);
	while (ipc.ip_req > 0)
		sleep((caddr_t)&ipc, IPCPRI);
	if (ipc.ip_req < 0)
		error = EIO;
	else
		rvp->r_val1 = ipc.ip_data;
	ipc.ip_lock = 0;
	wakeup((caddr_t)&ipc);
	return error;
}

/*
 * Code that the child process executes to implement the command
 * of the parent process in tracing.
 */
STATIC int ipcreg[] = {
	R0, R1, R2, R3, R4, R5, R6,
	R7, R8, AP, FP, PC, SP, PS
};

STATIC int
procxmt()
{
	register proc_t *p = u.u_procp;
	register int i;
	psw_t pswsave;
	register *ip;
	register struct as *as = p->p_as;
	register int *addr;

	/*
	 * If ipc is not locked on this process, return now.
	 * It was set running either because SIGKILL was sent
	 * or because its parent died.
	 */
	if (ipc.ip_lock != p->p_pid)
		return 0;

	i = ipc.ip_req;
	ipc.ip_req = 0;
	addr = ipc.ip_addr;
	switch (i) {

	case 1: /* read user I */
	case 2: /* read user D */

		if ((int)addr & (NBPW-1)
		  || copyin((caddr_t)addr, (caddr_t)&ipc.ip_data, sizeof(int)))
			goto error;
		break;


	case 3: /* read u */

		i = (int)addr;
		if (i >= 0  &&  i < ctob(USIZE)) {
			ipc.ip_data = ((physadr)&u)->r[i>>2];
			break;
		}

		ip = (int *)i;
		for (i = 0; i < sizeof(ipcreg)/sizeof(ipcreg[0]); i++) {
			if (ip == &u.u_ar0[ipcreg[i]]) {
				ipc.ip_data = *ip;
				goto ok3;
			}
		}
		goto error;
	ok3:
		break;

	case 4: /* write user I */
	case 5: /* write user D */
	{
		/*
		 * Zapped address must be on a word boundary; no explicit
		 * check here because the hardware enforces it and suword()
		 * will fail on a non-word address.
		 */
		if ((i = suword(addr, ipc.ip_data)) < 0) {
			/*
			 * Must change the permissions to allow writing.
			 * The act of writing should cause a copy-on-write
			 * for an executable file mapped with PRIVATE text.
			 */
			if (as_setprot(as, addr, NBPW, PROT_ALL) == 0)
				i = suword(addr, ipc.ip_data);
			/* XXX - this assumes the old permissions */
			(void) as_setprot(as, addr, NBPW,
			    PROT_ALL & ~PROT_WRITE);
		}

		if (i < 0)
			goto error;
		break;
	}

	case 6: /* write u */

		if ((i = (int)addr) >= 0 && i < ctob(USIZE))
			/* LINTED */
			ip = (int *)(((caddr_t)&u) + i);
		else
			ip = addr;
		ip = (int *)((int)ip & ~(NBPW-1));
		for (i = 0; i < sizeof(ipcreg)/sizeof(ipcreg[0]); i++)
			if (ip == &u.u_ar0[ipcreg[i]])
				goto ok6;
		goto error;
	ok6:
		if (ipcreg[i] == PS) {
			pswsave =  *(psw_t *)(&ipc.ip_data);
			ipc.ip_data = u.u_pcb.regsave[K_PS];
			((psw_t *)&ipc.ip_data)->NZVC = pswsave.NZVC;
		} else if (ipcreg[i] == SP)
			ipc.ip_data &= ~(NBPW - 1);
		*ip = ipc.ip_data;
		break;

	case 9:	/* set signal with trace trap and continue. */

		u.u_pcb.psw.TE = 1;

		/* FALLTHROUGH */

	case 7: /* set signal and continue */

		if ((int)addr != 1)
			u.u_pcb.regsave[K_PC] = (int)addr;
		if ((unsigned int)ipc.ip_data >= NSIG)
			goto error;
		/*
		 * If we're changing or clearing the current signal,
		 * the old current siginfo is invalid.  Discard it.
		 * We don't have a mechanism in ptrace(2), as we do
		 * in /proc, for redefining the current siginfo.
		 * Also, don't clobber SIGKILL.
		 */
		if (p->p_cursig != ipc.ip_data
		  && p->p_cursig != SIGKILL) {
			p->p_cursig = (char)ipc.ip_data;
			if (p->p_curinfo) {
				kmem_free((caddr_t)p->p_curinfo,
				  sizeof(*p->p_curinfo));
				p->p_curinfo = NULL;
			}
		}
		wakeup((caddr_t)&ipc);
		return 1;

	case 8: /* force exit */

		wakeup((caddr_t)&ipc);
		p->p_flag |= SPTRX;
		return 1;

	default:
	error:
		ipc.ip_req = -1;
	}
	wakeup((caddr_t)&ipc);
	return 0;
}

void
setsigact(sig, disp, mask, flags)
	register int sig;
	register void (*disp)();
	k_sigset_t mask;
	register int flags;
{
	register proc_t *pp = u.u_procp;

	ev_signal(pp, sig);

	u.u_signal[sig - 1] = disp;

	if (disp != SIG_DFL && disp != SIG_IGN) {
		sigdelset(&pp->p_ignore, sig);
		sigdiffset(&mask, &cantmask);
		u.u_sigmask[sig - 1] = mask;
		if (!sigismember(&cantreset, sig)) {
			if (flags & SA_RESETHAND)
				sigaddset(&u.u_sigresethand, sig);
			else
				sigdelset(&u.u_sigresethand, sig);
		}
		if (flags & SA_NODEFER)
			sigaddset(&u.u_signodefer, sig);
		else
			sigdelset(&u.u_signodefer, sig);
		if (flags & SA_RESTART)
			sigaddset(&u.u_sigrestart, sig);
		else
			sigdelset(&u.u_sigrestart, sig);
		if (flags & SA_ONSTACK)
			sigaddset(&u.u_sigonstack, sig);
		else
			sigdelset(&u.u_sigonstack, sig);
		if (flags & SA_SIGINFO)
			sigaddset(&pp->p_siginfo, sig);
		else if (sigismember(&pp->p_siginfo, sig))
			sigdelset(&pp->p_siginfo, sig);
	} else if (disp == SIG_IGN
	  || (disp == SIG_DFL && sigismember(&ignoredefault, sig))) {
		sigaddset(&pp->p_ignore, sig);
		sigdelset(&pp->p_sig, sig);
		sigdelq(pp, sig);
	} else
		sigdelset(&pp->p_ignore, sig);

	if (sig == SIGCLD) {
		if (flags & SA_NOCLDWAIT) {
			register proc_t *cp;
			pp->p_flag |= SNOWAIT;
			for (cp = pp->p_child; cp; cp = cp->p_sibling)
				if (cp->p_stat == SZOMB)
					freeproc(cp);
		}
		else
			pp->p_flag &= ~SNOWAIT;
		if (flags & SA_NOCLDSTOP)
			pp->p_flag &= ~SJCTL;
		else
			pp->p_flag |= SJCTL;
	}
}

void
sigcld(cp)
	register proc_t *cp;
{
	register proc_t *pp = cp->p_parent;
	k_siginfo_t info;

	switch (cp->p_wcode) {

		case CLD_EXITED:
		case CLD_DUMPED:
		case CLD_KILLED:
			wakeup((caddr_t)pp);
			break;

		case CLD_STOPPED:
		case CLD_CONTINUED:
			wakeup((caddr_t)pp);
			if (pp->p_flag & SJCTL)
				break;

			/* fall through */
		default:
			return;
	}

	winfo(cp, &info, 0);
	sigaddq(pp, &info, KM_SLEEP);
}

int
sigsendproc(p, pv)
	register proc_t *p;
	register sigsend_t *pv;
{
	if (p->p_pid == 1 && sigismember(&cantmask, pv->sig))
		return EPERM;

	if (pv->checkperm == 0 || hasprocperm(p->p_cred, u.u_procp->p_cred)
	  || (pv->sig == SIGCONT && p->p_sessp == u.u_procp->p_sessp)) {
		pv->perm++;
		if (pv->sig) {
			k_siginfo_t info;
			struct_zero((caddr_t)&info, sizeof(info));
			info.si_signo = pv->sig;
			info.si_code = SI_USER;
			info.si_pid = u.u_procp->p_pid;
			info.si_uid = u.u_procp->p_cred->cr_ruid;
			sigaddq(p, &info, KM_SLEEP);
		}
	}

	return 0;
}

int
sigsendset(psp, sig)
	register procset_t *psp;
	register sig;
{
	sigsend_t v;
	register int error;

	v.sig = sig;
	v.perm = 0;
	v.checkperm = 1;

	error = dotoprocs(psp, sigsendproc, (char *)&v);
	if (error == 0 && v.perm == 0)
		return EPERM;

	return error;
}

void
sigdeq(p, sig, qpp)
	register proc_t *p;
	int sig;
	sigqueue_t **qpp;
{
	register sigqueue_t **psqp, *sqp;

	*qpp = NULL;

	for (psqp = &p->p_sigqueue; ;) {
		if ((sqp = *psqp) == NULL)
			return;
		if (sqp->sq_info.si_signo == sig)
			break;
		else
			psqp = &sqp->sq_next;
	}

	*qpp = sqp;
	*psqp = sqp->sq_next;
	for (sqp = *psqp; sqp; sqp = sqp->sq_next) {
		if (sqp->sq_info.si_signo == sig) {
			sigaddset(&p->p_sig, sig);
			break;
		}
	}
}

void
sigdelq(p, sig)
	proc_t *p;
	int sig;
{
	register sigqueue_t **psqp, *sqp;

	for (psqp = &p->p_sigqueue; *psqp; ) {
		sqp = *psqp;
		if (sig == 0 || sqp->sq_info.si_signo == sig) {
			*psqp = sqp->sq_next;
			kmem_free(sqp, sizeof(sigqueue_t));
		} else
			psqp = &sqp->sq_next;
	}
}

void
sigaddq(p, infop, km_flags)
	proc_t *p;
	k_siginfo_t *infop;
	int km_flags;
{
	register sigqueue_t *sqp, **psqp;
	register int sig = infop->si_signo;

	ASSERT(sig >= 1 && sig < NSIG);
	if (!tracing(p, sig) && sigismember(&p->p_ignore, sig))
		goto postsig;

	sqp = (sigqueue_t *)kmem_alloc(sizeof(sigqueue_t), km_flags);
	if (sqp == NULL)
		goto postsig;

	for (psqp = &p->p_sigqueue; *psqp != NULL; psqp = &(*psqp)->sq_next)
		if ((*psqp)->sq_info.si_signo == sig) {
			kmem_free(sqp, sizeof(sigqueue_t));
			return;
		}

	*psqp = sqp;
	sqp->sq_next = NULL;

	bcopy((caddr_t)infop, (caddr_t)&sqp->sq_info, sizeof(k_siginfo_t));

postsig:
	sigtoproc(p, sig, infop->si_code <= 0);
}

/*
 * Duplicate siginfo_t structures.  Called from newproc().
 */
void
sigdupq(cp, pp)
	proc_t *cp;	/* child process */
	proc_t *pp;	/* parent process */
{
	register sigqueue_t *sqp;
	register sigqueue_t *nsqp;
	register sigqueue_t **psqp = &cp->p_sigqueue;

	/*
	 * Duplicate the queue of pending siginfo_t's.
	 */
	for (sqp = pp->p_sigqueue; sqp; sqp = sqp->sq_next) {
		nsqp = (sigqueue_t *)kmem_alloc(sizeof(sigqueue_t), KM_SLEEP);
		bcopy((caddr_t)&sqp->sq_info, (caddr_t)&nsqp->sq_info,
		  sizeof(k_siginfo_t));
		*psqp = nsqp;
		psqp = &nsqp->sq_next;
	}
	*psqp = NULL;

	/*
	 * Duplicate the current siginfo_t, if any.
	 */
	if ((sqp = pp->p_curinfo) == NULL)
		cp->p_curinfo = NULL;
	else {
		nsqp = (sigqueue_t *)kmem_alloc(sizeof(sigqueue_t), KM_SLEEP);
		bcopy((caddr_t)&sqp->sq_info, (caddr_t)&nsqp->sq_info,
		  sizeof(k_siginfo_t));
		nsqp->sq_next = NULL;
		cp->p_curinfo = nsqp;
	}
}

sigqueue_t	*
sigappend(toks, toqueue, fromks, fromqueue)
	register k_sigset_t	*toks;
	register sigqueue_t	*toqueue;
	register k_sigset_t	*fromks;
	register sigqueue_t	*fromqueue;
{
	register sigqueue_t	**endsq;

	sigorset(toks, fromks);
	if (fromqueue != NULL) {
		if (toqueue == NULL)
			toqueue = fromqueue;
		else {
			for (endsq = &toqueue->sq_next;
			  *endsq != NULL;
			  endsq = &(*endsq)->sq_next)
				;
			*endsq = fromqueue;
		}
	}
	return toqueue;
}

sigqueue_t	*
sigprepend(toks, toqueue, fromks, fromqueue)
	register k_sigset_t	*toks;
	register sigqueue_t	*toqueue;
	register k_sigset_t	*fromks;
	register sigqueue_t	*fromqueue;
{
	sigorset(toks, fromks);
	if (fromqueue != NULL) {
		fromqueue->sq_next = toqueue;
		toqueue = fromqueue;
	}
	return toqueue;
}
