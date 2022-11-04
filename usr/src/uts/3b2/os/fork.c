/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/fork.c	1.56"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/signal.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/immu.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/systm.h"
#include "sys/sysinfo.h"
#include "sys/map.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/file.h"
#include "sys/buf.h"
#include "sys/var.h"
#include "sys/errno.h"
#include "sys/time.h"
#include "sys/proc.h"
#include "sys/debug.h"
#include "sys/cmn_err.h"
#include "sys/acct.h"
#include "sys/tuneable.h"
#include "sys/inline.h"
#include "sys/disp.h"
#include "sys/class.h"
#include "sys/kmem.h"
#include "sys/session.h"
#include "sys/fault.h"
#include "sys/syscall.h"
#include "sys/ucontext.h"
#include "sys/procfs.h"
#include "sys/vmsystm.h"
#include "vm/hat.h"
#include "vm/seg.h"
#include "vm/as.h"
#include "vm/seg_u.h"

#if defined(__STDC__)
STATIC pid_t pid_assign(int, proc_t **);
STATIC int procdup(proc_t *, proc_t *, int);
STATIC void setuctxt(proc_t *, user_t *);
STATIC int fork1(char *, rval_t *, int);
#else
STATIC pid_t pid_assign();
STATIC int procdup();
STATIC void setuctxt();
STATIC int fork1();
#endif

/*
 * fork system call.
 */

fork(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	return fork1(uap, rvp, 0);
}

vfork(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	return fork1(uap, rvp, 1);
}

/* ARGSUSED */
STATIC int
fork1(uap, rvp, isvfork)
	char *uap;
	rval_t *rvp;
	int isvfork;
{
	register npcond;
	pid_t newpid;
	int error = 0;

	sysinfo.sysfork++;

	/*
	 * Disallow if
	 *	- no processes at all, or
	 *	- not super-user and too many procs owned, or
	 *	- not super-user and would take last slot.
	 * Check done in pid_assign().
	 */

	npcond = NP_FAILOK | (isvfork ? NP_VFORK : 0)
	  | ((u.u_cred->cr_uid && u.u_cred->cr_ruid) ? NP_NOLAST : 0);
	
	switch (newproc(npcond, &newpid, &error)) {
	case 1:	/* child -- successful newproc */
		rvp->r_val1 = u.u_procp->p_ppid;
		rvp->r_val2 = 1;	/* child */
		u.u_start = hrestime.tv_sec;
		u.u_ticks = lbolt;
		u.u_mem = rm_assize(u.u_procp->p_as);
		u.u_ior = u.u_iow = u.u_ioch = 0;
		u.u_procvirt = 0;
		u.u_uservirt = 0;
		u.u_acflag = AFORK;
		u.u_lock = 0;
		break;
	case 0: /* parent */
		rvp->r_val1 = (int) newpid;
		rvp->r_val2 = 0;	/* parent */
		break;
	default:	/* couldn't fork */
		error = EAGAIN;
		break;
	}
	return error;
}

/*
 * Create a new process -- internal version of sys fork().
 *
 * This changes the new proc structure and
 * alters only the u.u_procp of its u-area.
 *
 * It returns 1 in the new process, 0 in the old.
 */

int
newproc(cond, pidp, perror)
	int cond;
	pid_t *pidp;
	int *perror;
{
	extern void shmfork();
	extern int xsemfork();
	extern int xsdfork();

	register proc_t *pp, *cp;
	register n;
	proc_t *cpp;
	pid_t newpid;
	file_t *fp;

	if ((newpid = pid_assign(cond, &cpp)) == -1) {
		/* no proc table entry is available */
		if (cond & NP_FAILOK) {
			return -1;	/* out of memory or proc slot */
		} else {
			cmn_err(CE_PANIC, "newproc - no procs\n");
		}
	}

	/*
	 * Make proc entry for new proc.
	 */

	cp = cpp;
	pp = u.u_procp;
	cp->p_cstime = 0;
	cp->p_stime = 0;
	cp->p_cutime = 0;
	cp->p_utime = 0;
	cp->p_italarm[0] = NULL;
	cp->p_italarm[1] = NULL;
	cp->p_uid = pp->p_uid;
	cp->p_cred = pp->p_cred;
	crhold(pp->p_cred);
	cp->p_ignore = pp->p_ignore;
	cp->p_sig = pp->p_sig;
	cp->p_cursig = pp->p_cursig;
	cp->p_hold = pp->p_hold;
	cp->p_stat = SIDL;
	cp->p_clktim = 0;
	cp->p_flag = SLOAD | (pp->p_flag & (SDETACHED|SJCTL|SNOWAIT));
	forksession(cp, pp->p_sessp);
	joinpg(cp, pp->p_pgrp);
	cp->p_siginfo = pp->p_siginfo;

	if (cond & NP_SYSPROC)
		cp->p_flag |= (SSYS | SLOCK);
	cp->p_brkbase = pp->p_brkbase;
	cp->p_brksize = pp->p_brksize;
	cp->p_stkbase = pp->p_stkbase;
	cp->p_stksize = pp->p_stksize;
	cp->p_swlocks = 0;
	cp->p_segacct = 0;
	if (cond & NP_VFORK)
		cp->p_flag |= SVFORK;
	cp->p_pid = newpid;
	if (newpid <= SHRT_MAX)
		cp->p_opid = (o_pid_t)newpid;
	else
		cp->p_opid = (o_pid_t)NOPID;
	cp->p_epid = newpid;
	cp->p_ppid = pp->p_pid;
	cp->p_oppid = pp->p_opid;
	cp->p_cpu = 0;
	cp->p_pri = pp->p_pri;
	/*
	 * If inherit-on-fork, copy /proc tracing flags to child.
	 * New system processes never inherit tracing flags.
	 */
	if ((pp->p_flag & (SPROCTR|SPRFORK)) == (SPROCTR|SPRFORK)
	  && !(cond & (NP_SYSPROC|NP_INIT))) {
		cp->p_flag |= (SPROCTR|SPRFORK);
		cp->p_sigmask = pp->p_sigmask;
		cp->p_fltmask = pp->p_fltmask;
	} else {
		premptyset(&cp->p_sigmask);
		premptyset(&cp->p_fltmask);
		/*
		 * Syscall tracing flags are in the u-block.
		 * They are cleared when the child begins execution, below.
		 */
	}
	cp->p_cid = pp->p_cid;
	cp->p_clfuncs = pp->p_clfuncs;
	if (CL_FORK(pp, pp->p_clproc, cp, &cp->p_stat, &cp->p_pri,
	  &cp->p_flag, &cp->p_cred, &cp->p_clproc)) {
		cp->p_stat = 0;
		cp->p_ppid = 0;
		cp->p_oppid = 0;
		crfree(cp->p_cred);
		leavepg(cp);
		exitsession(cp);
		pid_release(cp->p_pid);	/* free the proc table entry */
		return -1;
	}

	/*
	 * Initialize child process's async request count.
	 */
	cp->p_aiocount = 0;
	cp->p_aiowcnt = 0;

	/*
	 * Link up to parent-child-sibling chain.  No need to lock
	 * in general since only a call to freeproc() (done by the
	 * same parent as newproc()) diddles with the child chain.
	 */
	cp->p_sibling = pp->p_child;
	cp->p_parent = pp;
	pp->p_child = cp;
	cp->p_sysid = pp->p_sysid;	/* RFS HOOK */

	/*
	 * Make duplicate entries where needed.
	 */
	for (n = 0; n < u.u_nofiles; n++) {
		if (getf(n, &fp) == 0) 
			fp->f_count++;
	}

	sigdupq(cp, pp);

	VN_HOLD(u.u_cdir);
	if (u.u_rdir)
		VN_HOLD(u.u_rdir);
	
	/*
	 * Copy process.
	 */
	switch (procdup(cp, pp, (cond & (NP_VFORK|NP_SHARE)))) {
	case 0:
		/* Successful copy */
		break;
	case -1:
		if (!(cond & NP_FAILOK))
			cmn_err(CE_PANIC, "newproc - fork failed\n");

		/* Reset all incremented counts. */

		pexit();

		CL_EXITCLASS(cp, cp->p_clproc);

		/*
		 * Clean up parent-child-sibling pointers.  No lock
		 * necessary since nobody else could be diddling with
		 * them here.
		 */
		pp->p_child = cp->p_sibling;
		cp->p_parent = NULL;
		cp->p_sibling = NULL;
		cp->p_stat = 0;
		cp->p_ppid = 0;
		cp->p_oppid = 0;
		crfree(cp->p_cred);
		leavepg(cp);
		exitsession(cp);
		pid_release(cp->p_pid);	/* free the proc table entry */
		if (pidp)
			*pidp = -1;
		return -1;
	case 1:
		/* Child resumes here */
		if ((u.u_procp->p_flag & SPROCTR) == 0) {
			/*
			 * /proc tracing flags have not been
			 * inherited; clear syscall flags.
			 */
			u.u_systrap = 0;
			premptyset(&u.u_entrymask);
			premptyset(&u.u_exitmask);
		}
		if (pidp)
			*pidp = cp->p_ppid;

		if (xsemfork())
			return -1;
		if (pp->p_sdp)
			*perror = xsdfork(cp, pp);

		return 1;
	}

	if (u.u_nshmseg)
		shmfork(pp, cp);

	cp->p_stat = SRUN;

	/*
	 * If we just created init process put it in the
	 * correct scheduling class.
	 */
	if (cond & NP_INIT) {
		if (getcid(initclass, &cp->p_cid) || cp->p_cid <= 0) {
			cmn_err(CE_PANIC,
	  "Illegal or unconfigured class (%s) specified for init process.\n\
Change INITCLASS configuration parameter.", initclass);
		}
		if (CL_ENTERCLASS(&class[cp->p_cid], NULL, cp, &cp->p_stat,
		  &cp->p_pri, &cp->p_flag, &cp->p_cred,
		  &cp->p_clproc, NULL, NULL)) {
			cmn_err(CE_PANIC,
	  "Init process cannot enter %s scheduling class.\n\
Verify that %s class is properly configured.", initclass, initclass);
		}
		cp->p_clfuncs = class[cp->p_cid].cl_funcs;

		/*
		 * We call CL_FORKRET with a NULL parent pointer
		 * in this special case because the parent may
		 * be in a different class from the function we are
		 * calling and its class specific data would be
		 * meaningless to the function.
		 */
		CL_FORKRET(cp, cp->p_clproc, NULL);
	} else {
		CL_FORKRET(cp, cp->p_clproc, pp->p_clproc);
	}

	if (pidp)
		*pidp = cp->p_pid;	/* parent returns pid of child */

	return 0;
}

/*
 * Create a duplicate copy of a process.
 */
STATIC int
procdup(cp, pp, isvfork)
	proc_t	*cp;
	proc_t	*pp;
	register int	isvfork;
{
	register user_t		*uservad;
	register int		*sp;
	register int		tmppsw;
	register r8, r7, r6, r5, r4, r3;	/* ensure registers are saved */
	caddr_t			addr;
	
	extern void ev_exit();

	/*
	 * We must save all the registers when we
	 * enter procdup if fork is to work correctly.
	 */
	r8 = r7 = r6 = r5 = r4 = r3 = 1;	/* very machine-dependent */

	if (pp->p_as != NULL) {
		/*
		 * Duplicate address space of current process.  For
		 * vfork we just share the address space structure.
		 */
		if (isvfork) {
			cp->p_as = pp->p_as;
		} else {
			cp->p_as = as_dup(pp->p_as);
			if (cp->p_as == NULL) {
				return -1;
			}
		}
	}
	
	/*
	 * Tell the general events VFS that the process forked.
	 * If this fails, then we must fail the fork.
	 */
	if (ev_config() && ev_fork(pp, cp) != 0) {
		if (isvfork == 0 && cp->p_as != NULL)
			as_free(cp->p_as);
		cp->p_as = NULL;
		return -1;
	}

	/* LINTED */
	if ((cp->p_segu = (struct seguser *)segu_get(cp)) == NULL) {
		ev_exit(cp, 0);
		if (isvfork == 0 && cp->p_as != NULL)
			as_free(cp->p_as);
		cp->p_as = NULL;
		return -1;
	}
	uservad = (user_t *)KUSER(cp->p_segu);

	/*
	 * Assumption that nothing past this point fails.
	 * Otherwise, segu_release() would have to handle 
	 * being called on a process that was not ONPROC.
	 */

	/*
	 * Setup child u-block.
	 */
	setuctxt(cp, uservad);

	/*
	 * Set up values for child to return to "newproc".
	 */
	ASSERT(u.u_pcbp == (pcb_t *)&u.u_kpcb.psw);

	/*
	 * This is really machine dependent --
	 * set the pc to return to our caller.
	 */
	sp = (int *)&pp + howmany(8, sizeof(int *));
	movpsw((int *)&uservad->u_kpcb.psw);
	uservad->u_kpcb.pc = (void (*)())sp[0];
	uservad->u_kpcb.sp = (int *)&cp;
	uservad->u_kpcb.regsave[K_AP] = sp[1];
	uservad->u_kpcb.regsave[K_FP] = sp[2];
	uservad->u_kpcb.regsave[K_R0] = 1;
	uservad->u_kpcb.regsave[K_R3] = sp[3];
	uservad->u_kpcb.regsave[K_R4] = sp[4];
	uservad->u_kpcb.regsave[K_R5] = sp[5];
	uservad->u_kpcb.regsave[K_R6] = sp[6];
	uservad->u_kpcb.regsave[K_R7] = sp[7];
	uservad->u_kpcb.regsave[K_R8] = sp[8];

	/*
	 * Put the child on the run queue.
	 */
	cp->p_flag |= SULOAD;
	return 0;
}

extern int mau_present;

/*
 * Setup context of child process.
 */

STATIC void
setuctxt(p, up)
	proc_t *p;		/* child proc pointer */
	register user_t *up;	/* child u-block pointer */
{
	register struct ufchunk *pufp, *cufp, *tufp;


	/* Save the current mau status for the child. */

	if (mau_present)
		mau_save();

	/* Copy u-block.  XXX - The amount to copy is machine dependent. */

	bcopy((caddr_t)&u, (caddr_t)up, (char *)&p - (char *)&u);
	up->u_procp = p;
	pufp = u.u_flist.uf_next;
	cufp = &up->u_flist;
	cufp->uf_next = NULL;
	while (pufp) {
		tufp = (struct ufchunk *)kmem_alloc(sizeof(struct ufchunk), KM_SLEEP);
		*tufp = *pufp;
		tufp->uf_next = NULL;
		cufp->uf_next = tufp;
		cufp = tufp;
		pufp = pufp->uf_next;
	}
}

/*
 * Release virtual memory.
 * Called by exit and getxfile (via execve).
 */

void
relvm(p)
	register proc_t *p;	/* process exiting or exec'ing */
{
	if ((p->p_flag & SVFORK) == 0) {
		if (p->p_as != NULL) {
			as_free(p->p_as);
			p->p_as = NULL;
		}
	} else {
		p->p_flag &= ~SVFORK;	/* no longer a vforked process */
		p->p_as = NULL;		/* no longer using parent's adr space */
		wakeup((caddr_t)p);	/* wake up parent */
		while ((p->p_flag & SVFDONE) == 0) {	/* wait for parent */
			(void) sleep((caddr_t)p, PZERO - 1);
		}
		p->p_flag &= ~SVFDONE;	/* continue on to die or exec */
	}
}

/*
 * Wait for child to exec or exit.
 * Called by parent of vfork'ed process.
 */

void
vfwait(pid)
	pid_t pid;
{
	register proc_t *pp = u.u_procp;
	register proc_t *cp = prfind(pid);

	ASSERT(cp != NULL && cp->p_parent == pp);

	/*
	 * Wait for child to exec or exit.
	 */
	while (cp->p_flag & SVFORK)
		(void) sleep((caddr_t)cp, PZERO-1);

	/*
	 * Copy back sizes to parent; child may have grown.
	 * We hope that this is the only info outside the
	 * "as" struct that needs to be shared like this!
	 */
	if (pp->p_brkbase == cp->p_brkbase)
		pp->p_brksize = cp->p_brksize;
	if (pp->p_stkbase == cp->p_stkbase)
		pp->p_stksize = cp->p_stksize;

	/*
	 * Wake up child, send it on its way.
	 */
	cp->p_flag |= SVFDONE;
	wakeup((caddr_t)cp);
}

/*
 * This function assigns a pid for use in a fork request.  It checks
 * to see that there is an empty slot in the proc table, that the
 * requesting user does not have too many processes already active,
 * and that the last slot in the proc table is not being allocated to
 * anyone who should not use it.
 *
 * After a proc slot is allocated, it will try to allocate a proc
 * structure for the new process. 
 *
 * If all goes well, pid_assign() will return a new pid and set up the
 * proc structure pointer for the child process.  Otherwise it will
 * return -1.
 */
STATIC pid_t
pid_assign(cond, pp)
	int	cond;	/* allow assignment of last slot? */
	proc_t	**pp;	/* child process proc structure pointer */
{
	register pincr_t	*cp;
	register proc_t		**p; 
	register proc_t		**endp = &nproc[v.v_proc];
	register uid_t ruid = u.u_cred->cr_ruid;
	int	uid_procs, slot;

	/*
	 * Assign a slot.
	 */
	if ((cp = pfreelisthead) == NULL) {	/* no free proc entry */
		syserr.procovf++;
		return -1;
	}

	if (cp->pi_link == NULL) {	/* last entry */
		if (cond & NP_NOLAST)
			return -1;
		pfreelisttail = NULL;
	}

	pfreelisthead = cp->pi_link;

	/*
	 * If not super-user then make certain that the maximum
	 * number of children don't already exist.
	 */
	if (u.u_cred->cr_uid && ruid) {
		uid_procs = 0;

		for (p = &nproc[0]; p < endp; ++p) {
			if (*p && (*p)->p_cred->cr_ruid == ruid)
				uid_procs++;
		}

		if (uid_procs >= v.v_maxup) {	/* make cp head of list again */
			pfreelisthead = cp;
			if (pfreelisttail == NULL)
				pfreelisttail = cp;
			return -1;
		}
	}

	cp->pi_link = NULL;

	/*
	 * Allocate a proc structure.
	 */

	slot = GET_INDEX(cp->pi_pid);

	*pp = (proc_t *)kmem_zalloc(sizeof(proc_t), KM_NOSLEEP);

	if (*pp == NULL) {		/* out of memory now! */
		pid_release(cp->pi_pid);	/* clean up */
		return -1;
	} else
		nproc[slot] = *pp;

	return cp->pi_pid;
}

/*
 * This function deactivates a proc table entry and returns
 * it to the free list.
 */
void
pid_release(pid)
	pid_t pid;
{
	register proc_t *p;
	register pincr_t *pip;	/* slot to be returned */
	int slot;	/* slot number for the given pid */

	slot = GET_INDEX(pid);
	pip = &pincr[slot];

	if ((p = nproc[slot]) != NULL) {

		/* Free the proc structure and proc slot. */

		nproc[slot] = NULL;
		kmem_free(p, sizeof(proc_t));

		/*
		 * Put this entry at the end of the free list so it is
		 * reassigned last.  Bump the incarnation count.
		 */

		INC_INCAR(pip->pi_pid);
		pip->pi_link = NULL;
		if (pfreelisttail) {
			pfreelisttail->pi_link = pip;
			pfreelisttail = pip;
		} else {
			pfreelisthead = pip;
			pfreelisttail = pip;
		}
	} else {

		/*
		 * This slot was never actually used; put it on the
		 * head of the list so it will be reused first.
		 */

		if (pfreelisthead) {
			pip->pi_link	= pfreelisthead;
			pfreelisthead	= pip;
		} else {
			pip->pi_link	= NULL;
			pfreelisthead	= pip;
			pfreelisttail	= pip;
		}
	}
}

/*
 * This routine is called from mlsetup() to initialize the process table
 * pointers, pids and status fields before they are first used.
 */

void
ptbl_init()
{
	register proc_t	**pp;
	register pincr_t *pip;
	ushort pid;
	
	pp = &nproc[0];
	pip = &pincr[0];

	for (pid = 0; pp < v.ve_proc; pp++, pip++) {
		*pp = NULL;
		pip->pi_pid = pid++;
		pip->pi_link = pip + 1;
	}

	/*
	 * Set up the pointers to process table entry 1
	 * and the first and last free entry. 
	 */

	pfreelisthead = &pincr[1];
	pip--;
	pip->pi_link = NULL;
	pfreelisttail = pip;
}
