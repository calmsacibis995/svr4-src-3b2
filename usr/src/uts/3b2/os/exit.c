/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/exit.c	1.67"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/signal.h"
#include "sys/sbd.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/immu.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/proc.h"
#include "sys/fault.h"
#include "sys/syscall.h"
#include "sys/ucontext.h"
#include "sys/procfs.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/acct.h"
#include "sys/var.h"
#include "sys/cmn_err.h"
#include "sys/debug.h"
#include "sys/inline.h"
#include "sys/wait.h"
#include "sys/siginfo.h"
#include "sys/procset.h"
#include "sys/disp.h"
#include "sys/class.h"
#include "sys/file.h"
#include "sys/session.h"
#include "sys/lock.h"
#include "sys/kmem.h"
#include "vm/page.h"

#ifdef KPERF
int exitflg;
#endif /* KPERF */

extern void semexit();
extern void shmexit();
extern void xsdexit();
extern void hdeexit();

/*
 * convert code/data pair into old style wait status
 */

STATIC int
wstat(code, data)
{
	register stat = (data & 0377);

	switch (code) {
		case CLD_EXITED:
			stat <<= 8;
			break;
		case CLD_DUMPED:
			stat |= WCOREFLG;
			break;
		case CLD_KILLED:
			break;
		case CLD_TRAPPED:
		case CLD_STOPPED:
			stat <<= 8;
			stat |= WSTOPFLG;
			break;
		case CLD_CONTINUED:
			stat = WCONTFLG;
			break;
	}
	return stat;
}
			
/*
 * exit system call: pass back caller's arg.
 */

struct exita {
	int	rval;
};

/* ARGSUSED */

void
rexit(uap, rvp)
	register struct exita *uap;
	rval_t *rvp;
{
	exit(CLD_EXITED, uap->rval);
}

/*
 * Release resources.
 * Enter zombie state.
 * Wake up parent and init processes,
 * and dispose of children.
 */
		
void
exit(why, what)
	int why;
	int what;
{
	register int rv;
	register struct proc *p, *q;
	struct ufchunk *ufp, *nufp;
	sess_t *sp = u.u_procp->p_sessp;

	p = u.u_procp;
	p->p_flag &= ~STRC;
	p->p_clktim = 0;

	ASSERT(p->p_curinfo == NULL);
	ASSERT(p->p_cursig == 0);

	sigfillset(&p->p_ignore);
	sigemptyset(&p->p_sig);
	sigemptyset(&p->p_sigmask);
	sigdelq(p, 0);

	closeall(0);

	ev_signal(p, 0);

	(void)hrt_cancel_proc();

	if (sp->s_vp && *sp->s_sidp == p->p_pid) {
		signal(*sp->s_fgidp,SIGHUP);
		if (sp->s_vp->v_stream)
			strclearctty(sp->s_vp->v_stream);
		freectty(sp);
	}

	ufp = u.u_flist.uf_next;
	while (ufp) {
		nufp = ufp->uf_next;
		kmem_free(ufp, sizeof(struct ufchunk));
		ufp = nufp;
	}

	(void)punlock();
	VN_RELE(u.u_cdir);
	if (u.u_rdir)
		VN_RELE(u.u_rdir);

	semexit();
	if (u.u_nshmseg)
		shmexit(p);
	if (p->p_sdp)
		xsdexit();	/* XENIX shared data exit */
	hdeexit();

	rv = wstat(why, what);

	ev_exit(p, rv); /* Let the events VFS clean up. */

	acct(rv & 0xff);

	/* 
	 * Free address space.
	 * relvm can take long; preempt. 
	 */

	PREEMPT();
	relvm(p);
	PREEMPT();

	crfree(u.u_cred);

	p->p_utime += p->p_cutime;
	p->p_stime += p->p_cstime;

	detachcld(p);

	if ((q = p->p_child) != NULL) {

		register proc_t *proc1 = nproc[1];

		for (; ; q = q->p_sibling) {
			q->p_ppid = 1;
			q->p_oppid = 1;
			q->p_parent = proc1;
			if (q->p_flag & STRC)
				psignal(q, SIGKILL);
			sigcld(q);
			if (q->p_sibling == NULL)
				break;
		}

		q->p_sibling = proc1->p_child;
		proc1->p_child = p->p_child;
		p->p_child = NULL;

	}	/* end if children */


	/*
	 * Hook for /proc.
	 * If process is undergoing /proc I/O, it must be a system process.
	 * Become an ordinary process and perform preempt() in order to keep
	 * the ublock around until the controlling process is done with it.
	 */
	if (p->p_flag & SPROCIO) {
		ASSERT(p->p_flag & SSYS);
		p->p_flag &= ~SSYS;
		availrmem += USIZE;
		pages_pp_kernel -= USIZE;
		preempt();
	}

	/*
	 * These MUST be set here because parents waiting for children
	 * will only check p_wcode since more conditions than zombie
	 * status can be waited for.
	 */

	p->p_stat = SZOMB;
	p->p_wcode = (char)why;
	p->p_wdata = what;

	if (p->p_trace)
		wakeup((caddr_t)p->p_trace);

	sigcld(p);

	/*
	 * Call class-specific function to clean up as necessary.
	 */
	
	CL_EXITCLASS(p, p->p_clproc);

	/* pswtch() frees up stack and ublock */

#ifdef KPERF
	if(kpftraceflg)
		exitflg = 1;
#endif /* KPERF */

	swtch();

	/* NOTREACHED */
}

/*
 * Format siginfo structure for wait system calls.
 */

void
winfo(pp, ip, waitflag)
	register proc_t *pp;
	register k_siginfo_t *ip;
{
	struct_zero((caddr_t)ip, sizeof(k_siginfo_t));
	ip->si_signo = SIGCLD;
	ip->si_code = pp->p_wcode;
	ip->si_pid = pp->p_pid;
	ip->si_status = pp->p_wdata;
	ip->si_stime = pp->p_stime;
	ip->si_utime = pp->p_utime;
	
	if (waitflag) {
		pp->p_wcode = 0;
		pp->p_wdata = 0;
	}
}

/*
 * Wait system call.
 * Search for a terminated (zombie) child,
 * finally lay it to rest, and collect its status.
 * Look also for stopped children,
 * and pass back status from them.
 */

int
waitid(idtype, id, ip, options)
	idtype_t idtype;
	id_t id;
	int options;
	k_siginfo_t *ip;
{
	int found, ready;
	register proc_t *cp, *pp;

	if (options & ~WOPTMASK)
		return EINVAL;
	
	switch (idtype) {
		case P_PID:
		case P_PGID:
			if (id < 0 || id >= MAXPID)
				return EINVAL;
		case P_ALL:
			break;
		default:
			return EINVAL;
	}
	
	pp = u.u_procp;
	while ((cp = pp->p_child) != NULL) {

		found = 0;
		ready = 0;

		do {

			if (idtype == P_PID && id != cp->p_pid)
				continue;
			if (idtype == P_PGID && id != cp->p_pgrp)
				continue;

			found++;

			/*
			 * The setting of p_wcode due to a process exiting
			 * is only done after the process has entered the
			 * zombie state.
			 */
			switch (cp->p_wcode) {

			case CLD_EXITED:
			case CLD_DUMPED:
			case CLD_KILLED:
				ready++;
				if (!(options & WEXITED))
					break;
				if (options & WNOWAIT)
					winfo(cp, ip, 0);
				else {
					winfo(cp, ip, 1);
					pp->p_cpu += cp->p_cpu;
					if (pp->p_cpu > 80)
						pp->p_cpu = 80;
					freeproc(cp);
				}
				return 0;

			case CLD_TRAPPED:
				ready++;
				if (!(options & WTRAPPED))
					break;
				winfo(cp, ip, !(options & WNOWAIT));
				return 0;

			case CLD_STOPPED:
				ready++;
				if (!(options & WSTOPPED))
					break;
				winfo(cp, ip, !(options & WNOWAIT));
				return 0;

			case CLD_CONTINUED:
				if (!(options & WCONTINUED))
					break;
				winfo(cp, ip, !(options & WNOWAIT));
				return 0;
			}

		} while ((cp = cp->p_sibling) != NULL);

		if (!found || ready == found)
			return ECHILD;

		if (options & WNOHANG) {
			ip->si_pid = 0;
			return 0;
		}

		if (sleep((caddr_t)pp, PWAIT | PCATCH))
			return intrerr(1);
	}
	return ECHILD;
}

/*
 * For implementations that don't require binary compatibility,
 * the wait system call may be made into a library call to the
 * waitid system call.
 */

struct waita {
	int *stat_loc; /* the library function copies this value from r_val2 */
};

/* ARGSUSED */

int
wait(uap, rvp)
	struct waita *uap;
	rval_t *rvp;
{
	register error;
	k_siginfo_t info;

	error =  waitid(P_ALL, (id_t)0, &info, WEXITED|WTRAPPED);
	if (error)
		return error;
	rvp->r_val1 = info.si_pid;
	rvp->r_val2 = wstat(info.si_code, info.si_status);
	return 0;
}

struct waitida {
	idtype_t idtype;
	id_t	 id;
	siginfo_t *infop;
	int	 options;
};

/* ARGSUSED */

int
waitsys(uap, rvp)
	struct waitida *uap;
	rval_t *rvp;
{
	register error;
	k_siginfo_t info;

	error =  waitid(uap->idtype, uap->id, &info, uap->options);
	if (error)
		return error;
	if (copyout((caddr_t)&info, (caddr_t)uap->infop, sizeof(k_siginfo_t)))
		return EFAULT;
	return 0;
}

/*
 * Remove zombie children from the process table.
 */

void
freeproc(p)
	proc_t *p;
{
	register proc_t	*q;

	ASSERT(p->p_stat == SZOMB);

	leavepg(p);
	exitsession(p);

	q = p->p_parent;
	q->p_cutime += p->p_utime;
	q->p_cstime += p->p_stime;
	if (p->p_trace)
		prexit(p);
	if (q->p_child == p)
		q->p_child = p->p_sibling;
	else {
		for (q = q->p_child; q; q = q->p_sibling)
			if (q->p_sibling == p)
				break;
		ASSERT(q && q->p_sibling == p);
		q->p_sibling = p->p_sibling;
	}
	pid_release(p->p_pid);	/* frees pid and proc structure */
}

/*
 * Clean up common process stuff -- called from newproc()
 * on error in fork() due to no swap space.
 */

void
pexit()
{
	/* Don't include punlock() since not needed for newproc() clean. */

	closeall(0);
	sigdelq(u.u_procp, 0);

	VN_RELE(u.u_cdir);
	if (u.u_rdir)
		VN_RELE(u.u_rdir);

	semexit();
	hdeexit();
}
