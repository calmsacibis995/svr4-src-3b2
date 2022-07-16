/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/trap.c	1.44"

#include "sys/sysmacros.h"
#include "sys/param.h"
#include "sys/types.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/systm.h"
#include "sys/sbd.h"
#include "sys/csr.h"
#include "sys/sit.h"
#include "sys/immu.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/proc.h"
#include "sys/signal.h"
#include "sys/siginfo.h"
#include "sys/fault.h"
#include "sys/syscall.h"
#include "sys/ucontext.h"
#include "sys/prsystm.h"
#include "sys/reg.h"
#include "sys/sysinfo.h"
#include "sys/edt.h"
#include "sys/utsname.h"
#include "sys/firmware.h"
#include "sys/cmn_err.h"
#include "sys/var.h"
#include "sys/debug.h"
#include "sys/inline.h"
#include "sys/mau.h"
#include "sys/disp.h"
#include "sys/class.h"
#include "sys/kmem.h"
#include "sys/vmsystm.h"
#include "sys/cred.h"
#include "sys/vnode.h"
#include "sys/evecb.h"
#include "sys/hrtcntl.h"
#include "sys/priocntl.h"
#include "sys/events.h"
#include "sys/evsys.h"
#include "sys/sys3b.h"

#include "vm/as.h"
#include "vm/seg.h"
#include "vm/seg_vn.h"
#include "vm/seg_kmem.h"
#include "vm/faultcode.h"

#ifndef S_EXEC
#define S_EXEC  S_READ
#endif

extern int rf_state;

extern void systrap();
extern void setintret();

#if __STDC__
STATIC int usrxmemflt(caddr_t, psw_t, k_siginfo_t *);
STATIC int krnxmemflt(caddr_t, psw_t);
STATIC int stop_on_fault(u_int, k_siginfo_t *);
void krnlflt(psw_t);
STATIC void fault_to_info(int, k_siginfo_t *);
#else
STATIC int usrxmemflt();
STATIC int krnxmemflt();
STATIC int stop_on_fault();
void krnlflt();
STATIC void fault_to_info();
#endif

int	mau_present;	/* flag if mau is in system */

/*
 * Read-only table of WE32100 support processor opcodes
 * used to distinguish between the (anonymous) external memory fault
 * caused by "support processor not present" and all other faults.
 * The entry format is
 *	1) indicates MAU instruction
 *	2) indicates double/triple write instruction
 */
#define MAU_SPECIAL 2	/* flags that a page fault on this instruction needs
			 * special processing to restart the MAU
			 */
STATIC char spopcode[256] = {
	0,0,1,2,0,0,1,2,0,0,0,0,0,0,0,0,	/* 0x00 - 0x0F */
	0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,	/* 0x10 - 0x1F */
	0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x20 - 0x2F */
	0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x30 - 0x3F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x40 - 0x4F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x50 - 0x5F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x60 - 0x6F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x70 - 0x7F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x80 - 0x8F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0x90 - 0x9F */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0xA0 - 0xAF */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0xB0 - 0xBF */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0xC0 - 0xCF */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0xD0 - 0xDF */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0xE0 - 0xEF */
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	/* 0xF0 - 0xFF */
};

int	*save_r0ptr;	/* pansave() uses this to find the registers */
STATIC int fltcr_type;	/* used to pass fltcr to krnlflt */

#ifdef DEBUG
/*
 * Data watchpoint to help with debugging.
 */
u_long	watchpt = 0;
int	wp_fault = 0;
int	wp_hit = 0;
paddr_t	wp_pdt = 0;
pte_t	*wp_pte = 0;
static	u_long	pdt[NPGPT + 7];

void
wpset(wp)
	register u_long wp;
{
	register u_int	pfn;
	register u_int	seglen;
	register int	i;
	register pte_t	*pte;
	register sde_t	*sde;

	if (watchpt != 0) {
		cmn_err(CE_CONT, "For now, only one watchpoint at a time.\n");
		return;
	}

	/*
	 * Sanity checking on the address.
	 */
	if ( (SECNUM(wp) != SCN1)  ||
	     ((wp & 0x3) != 0)     ||
	     (SEGNUM(wp) > sramb[SCN1].SDTlen) ||
	     (!SD_ISVALID(sde = (sde_t *)srama[SCN1] + SEGNUM(wp))) ){
		/*
		 * other checks: like not in the page containing an
		 * interrupt stack, not in a page needed by the trap
		 * code, ...
		 */
		cmn_err(CE_CONT, "Bad addr 0x%x . Watchpoint not set.\n", wp);
		return;
	}

	if (SD_ISCONTIG(sde)) {
		wp_pdt = kvtophys(((u_long)pdt + 0x1F) & (~0x1F));
		pte = (pte_t *)wp_pdt;
		pfn = phystopfn(sde->wd2.address);
		seglen = SD_LASTPG(sde);
		for (i = 0; i <= seglen; ++i, ++pfn, ++pte)
			pte->pg_pte = mkpte(PG_V, pfn);
		for (;i < NPPS; ++i, ++pfn, ++pte)
			pte->pg_pte = 0;

		sde->seg_flags &= ~SDE_C_bit;
		sde->wd2.address = wp_pdt;
		pte = (pte_t *)wp_pdt;
	} else {
		pte = (pte_t *)sde->wd2.address;
	}

	watchpt = wp;
	pte += PAGNUM(wp);
	wp_pte = pte;
	PG_SETW(pte);

	/*
	 * XXX - I don't think we care if the rest of the segment
	 * gets flushed. We only care about the page containing our
	 * watchpoint.
	 *
	 * I'm not sure, flush whole segment for now just to be safe.
	 * I'll check later.
	 *
	 * flushaddr(wp);
	 */
	 flushmmu((addr_t)(wp & ~SOFFMASK), NPPS);
}

void
wpclr(wp)
	register u_long wp;
{
	register sde_t	*sde;

	if (wp != watchpt) {
		cmn_err(CE_CONT, "There is no watchpoint at 0x%x .\n", wp);
		return;
	}

	if (wp_pdt == NULL) {
		/*
		 * We can assume the fault-on-write bit was off before
		 * we set the watchpoint. If it were on, the kernel
		 * would have panic'd; the kernel doesn't copy-on-write.
		 */
		PG_CLRW(wp_pte);
	} else {
		sde = (sde_t *)srama[SCN1] + SEGNUM(wp);
		sde->wd2.address = kvtophys(wp & ~SOFFMASK);
		sde->seg_flags |= SDE_C_bit;
		flushmmu((addr_t)(wp & ~SOFFMASK), NPPS);
		wp_pdt = NULL;
	}

	watchpt = 0;
	wp_pte = NULL;
}
#endif

STATIC void
trapsig(pp, ip)
	register proc_t *pp;
	register k_siginfo_t *ip;
{
	if (ip->si_signo) {
		/*
		 * Avoid a possible infinite loop
		 * if process is holding
		 * signal generated by a trap of a
		 * restartable instruction
		 */
		sigdelset(&pp->p_hold, ip->si_signo);
		sigaddq(pp, ip, KM_SLEEP);
	}
}


/*
 * s_trap() is called just before return from kernel mode to user mode
 * if there are pending signals or events to the current process or the
 * current process needs to be preempted.
 */

void
s_trap()
{
	time_t		syst;
	register proc_t	*pp;
	extern int		nrmx_ilc;
	extern int		nrmx_ilc2;
	extern int		nrmx_iop;
	extern int		nrmx_iop2;


	pp = u.u_procp;
	syst = pp->p_stime;

	if (runrun != 0)
		preempt();

	u.u_sigevpend = 0;

	/*
	 * We may be fielding an ilc or returning from a floating-point
	 * routine called via interrupt.  In this case, the user context
	 * we ultimately want to return to is in the u_ipcb rather than
	 * the u_pcb as is normally the case.  If we try to call a user
	 * signal routine from here we can end up returning from the
	 * handler back to nrmx_ilc where the context saved in u_ipcb
	 * will get clobbered.  We bypass signal processing in this case
	 * which means any signals that are currently pending won't be
	 * seen until we come back into the kernel.  This problem is
	 * rare enough that it is not of practical concern.
	 */
	if ((u.u_pcb.regsave[K_PC] < (int)&nrmx_ilc
	    || u.u_pcb.regsave[K_PC] > (int)&nrmx_ilc2)
	  && (u.u_pcb.regsave[K_PC] < (int)&nrmx_iop
	    || u.u_pcb.regsave[K_PC] > (int)&nrmx_iop2)) {
		if (ISSIG(pp, FORREAL))
			psig();
		else if (EV_ISTRAP(pp))
			ev_traptousr();
	}

	if (u.u_prof.pr_scale & ~1)
		addupc((void(*)())u.u_ar0[PC], &u.u_prof, (int)(pp->p_stime - syst));
}

void
addupc_clk()
{
	u.u_kpcb.ipcb.pc = systrap;	/* reset entry point */
	u.u_pcbp = (struct pcb *)&u.u_kpcb.psw;	/* running on kernel now */

	addupc(u.u_pcb.pc, &u.u_prof, 1);
	if (runrun != 0) 
		preempt();
	trap_ret();
}

/*
 * Called from ttrap.s if a trap occurs while on the kernel stack.
 */

int
k_trap(r0ptr)
	register int	*r0ptr;
{
	register int		i;
	register int		caddrsave;
	register union {
		psw_t	cps;
		int	cint;
	}			ps;

	ps.cint = r0ptr[PS];

	if (Rcsr & CSRTIMO && u.u_caddrflt) {
		Wcsr->c_sanity = 0;
		r0ptr[PC] = u.u_caddrflt;
		return 0;
	}

	/*
	 * If we were moving data to or from a user's
	 * process space and we got a memory fault,
	 * it may be an invalid page.  If so, validate
	 * it.  If not, give an error on the system
	 * call.
	 */

	if (ps.cps.FT == ON_NORMAL && ps.cps.ISC == XMEMFLT) {
		if (u.u_caddrflt) {
			k_siginfo_t info;
			/*
			 * Try to correct the fault.
			 */
			caddrsave = u.u_caddrflt;
			u.u_caddrflt = 0;
			if (valid_usr_range((caddr_t)*fltar, 1)) 
				i = usrxmemflt(r0ptr[PC], ps, &info);
                        else {
#ifdef KPERF
                                if (kpftraceflg) {
                                        asm(" MOVAW 0(%pc),Kpc ");
                                        kperf_write(KPT_UXMEMF, Kpc, curproc);
                                }
#endif /* KPERF */
                                i = krnxmemflt(r0ptr[PC], ps);
#ifdef DEBUG
				if (wp_fault != 0) {
					ps.cps.TE = 1;
					r0ptr[PS] = ps.cint;
				}
#endif
                        }

			/*
			 * If *xmemflt returned a non-zero value,
			 * then the fault couldn't be corrected.
			 * Return to the error routine indicated
			 * by u.u_caddrflt.  Otherwise, just return
			 * to try the access again.
			 */

			u.u_caddrflt = caddrsave;
			if (i != 0)
				r0ptr[PC] = caddrsave;
			else
				u.u_pgproc = 0;

#ifdef  KPERF
			if (kpftraceflg) {
				asm(" MOVAW 0(%pc),Kpc ");
				kperf_write(KPT_TRAP_RET, Kpc, curproc);
			}
#endif /* KPERF */

			return 0;
		} else {
			if ((i = krnxmemflt(r0ptr[PC], ps)) == 0) {
#ifdef DEBUG
				if (wp_fault != 0) {
					ps.cps.TE = 1;
					r0ptr[PS] = ps.cint;
				}
#endif
				return 0;
			}
		}
	}
#ifdef DEBUG
	if (ps.cps.FT == ON_NORMAL && ps.cps.ISC == TRCTRAP && wp_fault != 0) {
		if (wp_hit) {
			cmn_err(CE_CONT, "Hit watchpoint 0x%x:\n", watchpt);
			call_demon();
		}
		ps.cps.TE = 0;
		r0ptr[PS] = ps.cint;
		wp_fault = 0;
		wp_hit = 0;
		if (watchpt != 0) {
			PG_SETW(wp_pte);
			flushaddr(watchpt);
		}
		return 0;
	}
#endif

	cmn_err(CE_CONT, "TRAP\nproc = %x psw = %x\npc = %x",
		u.u_procp, ps.cint, r0ptr[PC]);

	return 1;
}

/*
 * Called from the trap handler when a trap occurs on the user stack.
 * Computes the associated fault (if any) to be delivered to the process,
 * stops the process if a debugger is tracing that fault, and subsequently
 * (or instead) delivers a signal to the process.
 *
 * Note that u_trap() is CALLPSed, not CALLed.  It must RETPS, not RETURN.
 */

void
u_trap()
{
	register struct user	*uptr = &u;
	register time_t		syst;
	register struct proc	*pp;
	register union {
		psw_t	cps;
		int	cint;
	}			ps;
	u_int			fault;
	k_siginfo_t		info;
	extern int		nrmx_ilc;
	extern int		nrmx_ilc2;
	extern int		nrmx_iop;
	extern int		nrmx_iop2;

	/*
	 * Finish storing the user state in u.u_pcb.
	 */

	uptr->u_pcb.regsave[K_R0] = uptr->u_r0tmp;
	uptr->u_pcb.regsave[K_PS] = *(--uptr->u_pcb.sp);
	uptr->u_pcb.regsave[K_PC] = *(--uptr->u_pcb.sp);

	/*
	 * Reset the kpcb initial pc so syscalls whiz through.
	 */

	uptr->u_kpcb.ipcb.pc = systrap;

	/* Get off the interrupt stack! */

	asm("	SUBW2	&4,%isp");

	ps.cint = uptr->u_pcb.regsave[K_PS];
	pp = uptr->u_procp;
	syst = pp->p_stime;
	fault = 0;
	struct_zero((caddr_t)&info, sizeof(info));

	info.si_addr = (caddr_t)(uptr->u_pcb.regsave[K_PC]);

	if (ps.cps.FT == ON_NORMAL) {
		switch (ps.cps.ISC) {
		case IOVFLT:
			u.u_fpovr = 0;
			info.si_signo = SIGFPE;
			info.si_code = FPE_INTOVF;
			fault = FLTIOVF;
			break;

		case IZDFLT:
			info.si_signo = SIGFPE;
			info.si_code = FPE_INTDIV;
			fault = FLTIZDIV;
			break;

		case BPTRAP:
			ps.cps.TE = 0;
			uptr->u_pcb.regsave[K_PS] = ps.cint;
			info.si_signo = SIGTRAP;
			info.si_code = TRAP_BRKPT;
			fault = FLTBPT;
			break;

		case TRCTRAP:
			ps.cps.TE = 0;
			uptr->u_pcb.regsave[K_PS] = ps.cint;
			info.si_signo = SIGTRAP;
			info.si_code = TRAP_TRACE;
			fault = FLTTRACE;
			break;

		case ILLOPFLT:
		case RESOPFLT:
			info.si_signo = SIGILL;
			info.si_code = ILL_ILLOPC;
			fault = FLTILL;
			break;

		case IVDESFLT:
			info.si_signo = SIGILL;
			info.si_code = ILL_ILLADR;
			fault = FLTILL;
			break;

		case RDTPFLT:
			info.si_signo = SIGILL;
			info.si_code = ILL_ILLOPN;
			fault = FLTILL;
			break;

		case ILCFLT:
		case PRVOPFLT:
			info.si_signo = SIGILL;
			info.si_code = ILL_PRVOPC;
			fault = FLTPRIV;
			break;

		case PRVREGFLT:
			info.si_signo = SIGILL;
			info.si_code = ILL_PRVREG;
			fault = FLTPRIV;
			break;

		case GVFLT:
			info.si_signo = SIGSYS;
			info.si_code = ILL_ILLTRP;
			fault = FLTILL;
			break;

		case XMEMFLT:
			if (usrxmemflt(uptr->u_pcb.pc, uptr->u_pcb.psw,
			  &info)) {
				if (info.si_signo == SIGSEGV)
					fault = FLTBOUNDS;
				else if (info.si_signo == SIGBUS 
				  || info.si_signo == SIGEMT)
					fault = FLTACCESS;
				else if (info.si_signo == SIGFPE)
					fault = FLTFPE;
				else
					fault = FLTILL;
			} else
				fault = FLTPAGE;
			break;
		}
	} else {
		/*
		 * We must have gotten here for an
		 * invalid "gate" done by the user.
		 */
		info.si_signo = SIGKILL;
	}

	/*
	 * We may be fielding an ilc or returning from a floating-point
	 * routine called via interrupt.  In this case, the user context
	 * we ultimately want to return to is in the u_ipcb rather than
	 * the u_pcb as is normally the case.  If we try to call a user
	 * signal routine from here we can end up returning from the
	 * handler back to nrmx_ilc where the context saved in u_ipcb
	 * will get clobbered.  We bypass signal processing in this case
	 * which means any signals that are currently pending won't be
	 * seen until we come back into the kernel.  This problem is
	 * rare enough that it is not of practical concern.
	 */
	if ((uptr->u_pcb.regsave[K_PC] >= (int)&nrmx_ilc
	    && uptr->u_pcb.regsave[K_PC] <= (int)&nrmx_ilc2)
	  || (uptr->u_pcb.regsave[K_PC] >= (int)&nrmx_iop
	    && uptr->u_pcb.regsave[K_PC] <= (int)&nrmx_iop2)) {
		/*
		 * Only rarely will there be a fault pending at 
		 * this point.  In this case just deliver the
		 * signal and bypass the stop-on-fault mechanism.
		 */
		trapsig(pp, &info);
		if (runrun != 0)
			preempt();

		/*
		 * Make sure u_sigevpend flag is clear so s_trap() won't be
		 * called to do signal processing on the way back to the user.
		 */
		u.u_sigevpend = 0;
		trap_ret();
	}

	/*
	 * If a debugger has directed the process to stop upon
	 * incurring this fault, do so.  Otherwise deliver the
	 * associated signal.
	 */
	if (fault
	  && info.si_signo != SIGKILL
	  && prismember(&pp->p_fltmask, fault)
	  && stop_on_fault(fault, &info) == 0)
		info.si_signo = 0;

	trapsig(pp, &info);
	if (runrun != 0)
		preempt();
	if (ISSIG(pp, FORREAL)) 
		psig();
	else if (EV_ISTRAP(pp))
		ev_traptousr();

	if (uptr->u_prof.pr_scale & ~1)
		addupc((void(*)())uptr->u_pcb.regsave[K_PC], &uptr->u_prof,
		  (int)(pp->p_stime - syst));

	
	/*
	 * Return through common interrupt return sequence.
	 */

	trap_ret();
}

/*
 * Called from the trap handler when a system call occurs.
 *
 * Note that systrap() is CALLPSed, not CALLed.  It must RETPS, not RETURN.
 */

void
systrap()
{
	register struct user	*uptr = &u;
	register struct sysent	*callp;
	register u_int		scall;
	register proc_t		*pp;
	time_t			syst;
	pid_t			pid;
	int			error = 0;
	rval_t			rval;

	/* Finish storing the user state into u.u_pcb. */

	uptr->u_pcb.regsave[K_R0] = uptr->u_r0tmp;
	uptr->u_pcb.regsave[K_PS] = *(--uptr->u_pcb.sp);
	uptr->u_pcb.regsave[K_PC] = *(--uptr->u_pcb.sp);

	/*
	 * The following line is only really needed
	 * for process 1.  We come in here on the
	 * user stack but the saved PSW has kernel
	 * mode.  This is necessary since we had to
	 * write the pcb to switch stacks.  See the
	 * code in misc.s/icode.
	 */

	((psw_t *)(&uptr->u_pcb.regsave[K_PS]))->CM = PS_USER;

	/* Get off the interrupt stack. */

	asm("	SUBW2	&4,%isp ");

	pp = uptr->u_procp;
	syst = pp->p_stime;
	pid = pp->p_pid;

	sysinfo.syscall++;
	uptr->u_error = 0;			/* XXX */
	((psw_t *)(uptr->u_pcb.regsave))[K_PS].NZVC &= ~PS_C;

	scall = (uptr->u_pcb.regsave[K_R1]&0x7FF8) >> 3;
	callp = &sysent[scall];

	uptr->u_sysabort = 0;
	/*
	 * Do stop-on-syscall-entry test.
	 */
	if (uptr->u_systrap
	  && prismember(&uptr->u_entrymask, scall)
	  && stop(pp, PR_SYSENTRY, scall, 0))
		swtch();

	/*
	 * Fetch the syscall arguments.
	 */
	{
		register u_int *ap = (u_int *)uptr->u_pcb.regsave[K_AP];
		register u_int i;

		for (i = 0; i < callp->sy_narg; i++)
			uptr->u_arg[i] = lfuword((int *)(ap++));
		uptr->u_ap = uptr->u_arg;
	}

	rval.r_val1 = 0;
	rval.r_val2 = uptr->u_pcb.regsave[K_R1];

	uptr->u_rval1 = rval.r_val1;		/* XXX */
	uptr->u_rval2 = rval.r_val2;		/* XXX */

#ifdef DEBUG
	sysin();
#endif
	if (uptr->u_sysabort) {
		/*
		 * u_sysabort may have been set by a debugger while
		 * the process was stopped.  If so, don't execute
		 * the syscall code.
		 */
		uptr->u_sysabort = 0;
		error = EINTR;
	} else {
		if (rf_state)
			uptr->u_syscall = scall;
		if ((callp->sy_flags & SETJUMP) && setjmp(&uptr->u_qsav)) {
			if ((error = uptr->u_error) == 0)
				error = EINTR;
		} else {
#ifdef KPERF
			if (pre_trace) {
				kpftraceflg = 1;
				pre_trace = 0;
			}
			if (kpftraceflg)
				kperf_write(KPT_SYSCALL, *callp->sy_call,
				    curproc);
#endif /* KPERF */
			error = (*callp->sy_call)(uptr->u_ap, &rval);
		}
	}

	if (error) {

#ifdef DEBUG
		sysout();
#endif
		if (error == EFBIG)
			psignal(uptr->u_procp, SIGXFSZ);
		else if (error == EINTR) {
			register int cursig = uptr->u_procp->p_cursig;
			if ((cursig && sigismember(&uptr->u_sigrestart, cursig))
			  || ev_intr_restart(uptr->u_procp))
				error = ERESTART;
		}

		uptr->u_pcb.regsave[K_R0] = error;
		uptr->u_error = 0;
		((psw_t *)(uptr->u_pcb.regsave))[K_PS].NZVC |= PS_C;

	} else {
#ifdef DEBUG
		sysok();
#endif
		uptr->u_pcb.regsave[K_R0] = rval.r_val1;
		uptr->u_pcb.regsave[K_R1] = rval.r_val2;
	}

	/*
	 * Reset pp in case we are the child process returning from
	 * a fork system call.
	 */
	pp = uptr->u_procp;

	/*
	 * Do stop-on-syscall-exit test.
	 */
	if (uptr->u_systrap
	  && prismember(&uptr->u_exitmask, scall)
	  && stop(pp, PR_SYSEXIT, scall, 0))
		swtch();

	/*
	 * If we are the parent returning from a successful
	 * vfork, wait for the child to exec or exit.
	 */
	if (scall == SYS_vfork && pid == pp->p_pid && error == 0)
		vfwait((pid_t)rval.r_val1);

	if (runrun != 0) 
		preempt();

	if (ISSIG(pp, FORREAL))
		psig();
	else if (EV_ISTRAP(pp))
		ev_traptousr();

	/*
	 * If pid != pp->p_pid, then we are the child
	 * returning from a fork system call.  In this
	 * case, ignore syst since our time was reset
	 * in fork.
	*/

	if (uptr->u_prof.pr_scale & ~1)
		addupc((void(*)())uptr->u_ar0[PC], &uptr->u_prof, 
			pid == pp->p_pid ? (int)(pp->p_stime - syst)
					 : (int)pp->p_stime);


	/*
	 * Return through common interrupt return sequence.
	 */

	trap_ret();
}

/*
 * Nonexistent system call-- signal bad system call.
 */
/* ARGSUSED */
int
nosys(uap, rvp)
	char *uap;
	rval_t *rvp;	
{
	psignal(u.u_procp, SIGSYS);
	return EINVAL;
}

/* 
 * Package not installed -- return ENOPKG error (STUBS support).
 */
/* ARGSUSED */
int
nopkg(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	return ENOPKG;
}

/*
 * Internal function call for uninstalled package -- panic system.  If the
 * system ever gets here, it means that an internal routine was called for
 * an optional package, and the OS is in trouble.  (STUBS support.)
 */

void
noreach()
{
	cmn_err(CE_PANIC,"Call to internal routine of uninstalled package");
}

/*
 * stray interrupts enter here
 */

void
intnull()
{
}

/*
 * This routine is called for all level 15 interrupts
 * except clock interrupts.
 */

void
intsyserr()
{
	extern char pwrflag;

	if (SBDSIT->count0 != SITINIT) { /* power down */
		pwrflag = 1;
		Wcsr->s_pir9 = 0x1;  /* set PIR 9 */
		SBDSIT->command=0x16;

		/* clear softpwr-bus timer bit */

		Wcsr->c_sanity = 0x00;
		return;
	}
	if ((Rcsr & CSRPARE) != 0) {
		Wcsr->c_parity = 0;
		cmn_err(CE_PANIC,"SYSTEM PARITY ERROR INTERRUPT");
	}
	if ((Rcsr & CSRTIMO) != 0) {
		Wcsr->c_sanity = 0;
		if (u.u_caddrflt)
			setintret(u.u_caddrflt);
		else
			cmn_err(CE_PANIC,"SYSTEM BUS TIME OUT INTERRUPT");
	}
}

/*
 * intspwr()
 *
 * Handle a softpower shutdown.  Simply sends SIGPWR to init, allowing
 * it to gracefully stop the machine.
 */

void
intspwr()
{
	psignal(proc_init, SIGPWR);
}

/*
 * process exception
 */

void
intpx(psw, pcbp)
	psw_t psw;
	struct pcb *pcbp;
{
	splhi();
	cmn_err(CE_PANIC,
	  "process exception, proc = 0x%x, psw= 0x%x, pcbp = 0x%x.\n",
	  u.u_procp, psw, pcbp);
}

/*
 * This routine is called for user stack exceptions.
 */

void
intsx(pcbp)
	register struct pcb *pcbp;
{
	register struct proc	*pp;
	u_int 			fault = 0;
	caddr_t			mau_faddr;
	int			mau_flg = 0;
	time_t			syst;
	k_siginfo_t		info;

	pp = u.u_procp;
	syst = pp->p_stime;

	struct_zero((caddr_t)&info, sizeof(info));

	/*
	 * If it was a stack bounds fault, try to grow the stack.
	 * If this succeeds, then just return.  Otherwise, the
	 * user's stack is blown.  Deliver a SIGSEGV and be sure
	 * we don't return to user level with the stack messed up.
	 *
	 * If it was a stack fault, try to fix it up
	 * by loading the page or making it writable.
	 *
	 * We don't expect to get an interrupt vector fetch fault.
	 */

	if (pcbp->psw.ISC == STKBOUND) {

#ifdef  KPERF
		if (kpftraceflg) {
			asm(" MOVAW 0(%pc),Kpc");
			kperf_write(KPT_STKBX, Kpc, curproc);
		}
#endif /* KPERF */

		/* Check for the "lost SPOP write" */
		if (mau_present)
			mau_pfault(MAU_PROBESB, &mau_faddr, &mau_flg);
		if (grow(pcbp->sp)) {
			fault = FLTPAGE;
			/*
			 * mau_flg == 1 only if mau_pfault was called and
			 * the current instruction is a non-restartable MAU
			 * operation -- complete the operation.
			 */
			if (mau_flg)
				mau_pfault(MAU_NOPROBE, &mau_faddr, &mau_flg);
		} else {
			info.si_signo = SIGSEGV;
			info.si_code = SEGV_MAPERR;
			fault = FLTSTACK;
		}
	} else if (pcbp->psw.ISC == STKFLT) {
		if (usrxmemflt(pcbp->pc, pcbp->psw, &info))
			fault = FLTSTACK;
		else
			fault = FLTPAGE;
	} else
		cmn_err(CE_PANIC, "Unexpected user stack fault, ISC = %x.",
		  pcbp->psw.ISC);

	/*
	 * The following code provides two hardware workarounds to
	 * make trace trap and FP overflow trap work.  If
	 * you have just executed an instruction with the TE
	 * (OE) bit set in the psw, you want to give a normal
	 * exception for trace (FP overflow).  However,
	 * if the current stack page is not valid or you are
	 * at the top of the stack and have to grow it, then
	 * you lose the normal exception and have no idea what
	 * happened.  This code checks for this.
	 * 
	 * The FP overflow check is based on the u_fpovr
	 * field of the u-block.  If this bit has been set by
	 * a sys3b() call, and the OE bit is clear, it indicates
	 * an overflow has occurred and cleared the OE bit from
	 * the PSW.
	 * 
	 * The trace trap check is based on the u_tracepc
	 * field of the u-block.  The code in ttrap.s at
	 * trap_ret2 checks if the psw being restored has the
	 * TE bit set.  If so, u_tracepc is set to the pc
	 * being returned to.  If the TE bit is off, then
	 * u_tracepc is cleared.  If we get here with u_tracepc
	 * non-zero and not equal to the pc we got the stack
	 * fault on, then we must have gotten the stack fault
	 * trying to do the normal exception for the trace.
	 */

	if (info.si_signo == 0 && u.u_fpovr && pcbp->psw.OE == 0) {
		u.u_fpovr = 0;
		info.si_signo = SIGFPE;
		info.si_code = FPE_FLTOVF;
		fault = FLTFPE;
	}

	if (info.si_signo == 0 
	  && u.u_tracepc 
	  && u.u_tracepc != (char *)pcbp->pc) {
		pcbp->psw.TE = 0;
		info.si_signo = SIGTRAP;
		info.si_code = TRAP_TRACE;
	}

	/*
	 * If the stop-on-fault bit is set for this fault, stop
	 * the process; otherwise deliver the signal, if any.
	 */
	if (fault
	  && info.si_signo != SIGKILL
	  && prismember(&pp->p_fltmask, fault)
	  && stop_on_fault(fault, &info) == 0)
		info.si_signo = 0;

	trapsig(pp, &info);

	if (runrun != 0)
		preempt();

	if (ISSIG(pp, FORREAL))
		psig();
	else if (EV_ISTRAP(pp))
		ev_traptousr();

	if (u.u_prof.pr_scale & ~1)
		addupc((void(*)())u.u_ar0[PC], &u.u_prof, (int)(pp->p_stime - syst));

	/*
	 * Return through common interrupt return sequence.
	 */

	trap_ret();
}

/*
 * Handle stop-on-fault processing for the debugger.  Returns 0
 * if the fault is cleared during the stop, nonzero if it isn't.
 */

STATIC int
stop_on_fault(fault, sip)
	register u_int fault;
	register k_siginfo_t *sip;
{
	register proc_t *p = u.u_procp;

	ASSERT(prismember(&p->p_fltmask, fault));

	/*
	 * Record current fault and siginfo structure so debugger can
	 * find it.
	 */
	p->p_curflt = (u_char)fault;
	u.u_siginfo = *sip;
	
	if (stop(p, PR_FAULTED, fault, 0))
		swtch();
	fault = p->p_curflt;
	p->p_curflt = 0;
	return fault;
}

/*
 * This routine is called for kernel stack exceptions.
 */

void
intsxk(pcbp)
	register struct pcb	*pcbp;
{
	splhi();
	cmn_err(CE_PANIC, "kernel process stack exception, ISC=%d.\n",
	  pcbp->psw.ISC);
}

/*
 * Ignored system call
 */

void
nullsys()
{
}

void
stray(addr)
	int addr;
{
	cmn_err(CE_NOTE,"stray interrupt at %x\n", addr);
}

STATIC void
fault_to_info(flt, infop)
	int flt;
	k_siginfo_t *infop;
{
	switch (FC_CODE(flt)) {

	case 0:
		infop->si_signo = 0;
		break;

	case FC_HWERR:
		infop->si_signo = SIGBUS;
		infop->si_code = BUS_ADRERR;
		break;

	case FC_ALIGN:
		infop->si_signo = SIGBUS;
		infop->si_code = BUS_ADRALN;
		break;

	case FC_NOMAP:
		infop->si_signo = SIGSEGV;
		infop->si_code = SEGV_MAPERR;
		break;

	case FC_PROT:
		infop->si_signo = SIGSEGV;
		infop->si_code = SEGV_ACCERR;
		break;

	case FC_OBJERR:
		infop->si_signo = SIGBUS;
		infop->si_code = BUS_OBJERR;
		infop->si_errno = FC_ERRNO(flt);
		break;

	default:
		infop->si_signo = SIGKILL;
		break;
	}
}


STATIC int
usrxmemflt(pc, ps, infop)
	register caddr_t pc;
	psw_t	ps;
	k_siginfo_t *infop;
{
	struct seg *sp;
	struct as *asp;
	FLTCR		faultcr;
	int		flt;
	caddr_t		mau_faddr;
	int		mau_flg;
	caddr_t		faultadr;
	enum fault_type ftype;
	enum seg_rw rw;

	faultcr = *fltcr;
	faultadr = (caddr_t)(*fltar);
	*(int *)fltcr = 0;
	*(int *)fltar = 0;

	infop->si_addr = faultadr;

#ifdef KPERF
	if (kpftraceflg) {
		asm(" MOVAW 0(%pc),Kpc ");
		kperf_write(KPT_UXMEMF, Kpc, curproc);
	}
#endif /* KPERF */

	ftype = F_INVAL;
	rw = S_READ;
	switch (faultcr.reqacc) {
	case AT_IFAD:
	case AT_IFTCH:
	case AT_IPFTCH:
		rw = S_EXEC;
		break;
	case AT_OWRITE:
		cmn_err(CE_NOTE,"usrxmemflt: reqacc is AT_OWRITE\n");
		/* FALLTHROUGH */
	case AT_WRITE:
	case AT_SPOPWRITE:
		rw = S_WRITE;
		break;
	}
	sp = NULL;
	if ((asp = u.u_procp->p_as) == NULL)
		cmn_err(CE_PANIC, "usrxmemflt: no as allocated.");

	sp = as_segat(asp, faultadr);

	if (mau_present && ps.FT == ON_STACK) {
		mau_pfault(MAU_PROBESF, &mau_faddr, &mau_flg);
		/*
		 * Check to see if mau_probe faulted in the page
		 * indicated by faultadr.
		 */
		if (((u_int)faultadr & PG_ADDR) 
		  == ((u_int)mau_faddr & PG_ADDR))
			return 0;
	}
	switch (faultcr.ftype) {

	case F_INVALID:
	case F_SDTLEN:
	case F_PTLEN:
		if (sp != NULL
		  || (faultadr >= u.u_procp->p_stkbase + u.u_procp->p_stksize
		    && grow((int *)(_VOID *)faultadr))) {
			if ((flt = as_fault(asp, faultadr, 1, ftype, rw)) == 0
			  && spopcode[lfubyte(pc)] == MAU_SPECIAL
			  && faultcr.reqacc == AT_SPOPWRITE)
				mau_pfault(MAU_NOPROBE, &faultadr, &mau_flg);
			fault_to_info(flt, infop);
			break;
		}
		infop->si_signo = SIGSEGV;
		infop->si_code = SEGV_MAPERR;
		break;

	case F_P_N_P:
		if (sp == NULL
		  && faultadr >= u.u_procp->p_stkbase + u.u_procp->p_stksize
		  && !grow((int *)(_VOID *)faultadr)) {
			infop->si_signo = SIGSEGV;
			infop->si_code = SEGV_MAPERR;
			break;
		}
		if ((flt = as_fault(asp, faultadr, 1, ftype, rw)) == 0
		  && spopcode[lfubyte(pc)] == MAU_SPECIAL
		  && faultcr.reqacc == AT_SPOPWRITE)
			mau_pfault(MAU_NOPROBE, &faultadr, &mau_flg);
		fault_to_info(flt, infop);
		break;

	case F_PWRITE:
		if ((flt = as_fault(asp, faultadr, 1, F_PROT, rw)) == 0
		  && spopcode[lfubyte(pc)] == MAU_SPECIAL
		  && faultcr.reqacc == AT_SPOPWRITE)
			mau_pfault(MAU_NOPROBE, &faultadr, &mau_flg);
		fault_to_info(flt, infop);
		break;

	case F_ACCESS:
	{
		extern int gateSIZE[];

	/*
	 * if this is a locore access, and printing is turned on, then print a message
	 */
		if ((s3btlc_state == S3BTLC_PRINT) && (rw == S_READ) &&
				(faultadr < (caddr_t)ctob(btoc((int)gateSIZE)))) {
			cmn_err(CE_NOTE,
		"locore access: pid: %d\tpc: 0x%x\taddress: 0x%x\n\tCOMMAND: %s\n",
				u.u_procp->p_pid, pc, faultadr, u.u_psargs);
			}

		infop->si_signo = SIGSEGV;
		infop->si_code = SEGV_MAPERR;
		break;
	}

	case F_OFFSET:
	case F_ACC_OFF:
		infop->si_signo = SIGSEGV;
		infop->si_code = SEGV_MAPERR;
		break;

	case F_MPROC:
	case F_RMUPDATE:
	case F_SEG_N_P:
	case F_OBJECT:
	case F_PT_N_P:
	case F_INDIRECT:
	case F_D_P_HIT:
		infop->si_signo = SIGKILL;
		break;

	default:
		/*
		 * We can get here for 3 cases: an alignment error,
		 * a support processor instruction executed on a 32100
		 * which does not have the support processor hardware,
		 * or a gate instruction executed with one or two bad
		 * index values.  Note that on a 32000, an spop
		 * instruction will generate an ILLOPFLT fault.
		 */
		if (Rcsr & CSRALGN) {
			infop->si_signo = SIGBUS;
			infop->si_code = BUS_ADRALN;
			Wcsr->c_align = 0;
		} else if ((Rcsr & CSRPARE) != 0) {
			Wcsr->c_parity = 0;
			cmn_err(CE_PANIC, "SYSTEM PARITY ERROR INTERRUPT");
		} else if (spopcode[lfubyte(pc)] != 0) {
			if (mau_present)
				mau_fault(infop);
			else {
				infop->si_signo = SIGILL;
				infop->si_code = ILL_ILLOPC;
			}
		} else
			infop->si_signo = SIGKILL;
		break;
	}

	return infop->si_signo;
}


/* ARGSUSED */
STATIC int
krnxmemflt(pc, ps)
	register caddr_t pc;
	psw_t	ps;
{
	register int	sig;
	struct seg *sp;
	struct as *asp;
	FLTCR		faultcr;
	caddr_t		faultadr;
	enum fault_type ftype;
	enum seg_rw rw;

	faultcr = *fltcr;
	faultadr = (caddr_t)*fltar;
	*(int *)fltcr = 0;
	*(int *)fltar = 0;
	fltcr_type = faultcr.ftype;	/* for krnlflt */

	ftype = F_INVAL;
	rw = S_READ;

	switch (faultcr.reqacc) {
	case AT_IFAD:
	case AT_IFTCH:
	case AT_IPFTCH:
		rw = S_EXEC;
		break;
	case AT_OWRITE:
		cmn_err(CE_NOTE,"krnxmemflt: reqacc is AT_OWRITE\n");
		/* FALLTHROUGH */
	case AT_WRITE:
	case AT_SPOPWRITE:
		rw = S_WRITE;
		break;
	}

	sp = NULL;
	asp = &kas;
	sp = as_segat(asp, faultadr);
	if (sp == NULL)
		return 1;

	switch (faultcr.ftype) {

	case F_P_N_P:
		if ((sig = as_fault(asp, faultadr, 1, ftype, rw)) != 0)
			sig = 1;
		break;

	case F_PWRITE:
#ifdef DEBUG
		if (watchpt != 0 &&
		    ((u_int)faultadr & ~POFFMASK) == (watchpt & ~POFFMASK)) {
			wp_fault = 1;
			if (((u_int)faultadr & ~0x3) == watchpt)
				wp_hit = 1;
			else
				wp_hit = 0;
			PG_CLRW(wp_pte);
			flushaddr(faultadr);
			return 0;
		}
#endif
		if ((sig = as_fault(asp, faultadr, 1, F_PROT, rw)) != 0)
			sig = 1;
		break;

	default:
		sig = 1;
		break;
	}

	return sig;
}

void
krnlflt(ps)
	psw_t	ps;			/* previous PSW content */
{
	register int	i;
	static	char	*isc_msg[] = {
					"DIVIDE-BY-ZERO",
					"TRACE TRAP",
					"ILLEGAL OPCODE",
					"RESERVED OPCODE",
					"INVALID DESCRIPTOR",
					"EXTERNAL",
					"GATE VECTOR",
					"ILLEGAL LEVEL CHANGE",
					"RESERVED DATA TYPE",
					"INTEGER OVERFLOW",
					"PRIVILEGED OPCODE",
					"(UNKNOWN:11)",
					"(UNKNOWN:12)",
					"(UNKNOWN:13)",
					"BREAKPOINT TRAP",
					"PRIVILEGED REGISTER",
					};

	static	struct {
		int	errno;
		char	*name;
	}			mmu_err[] = {
					F_MPROC,	"F_MPROC",
					F_RMUPDATE,	"F_RMUPDATE",
					F_SDTLEN,	"F_SDTLEN",
					F_PWRITE,	"F_PWRITE",
					F_PTLEN,	"F_PTLEN",
					F_INVALID,	"F_INVALID",
					F_SEG_N_P,	"F_SEG_N_P",
					F_OBJECT,	"F_OBJECT",
					F_PT_N_P,	"F_PT_N_P",
					F_P_N_P,	"F_P_N_P",
					F_INDIRECT,	"F_INDIRECT",
					F_ACCESS,	"F_ACCESS",
					F_OFFSET,	"F_OFFSET",
					F_ACC_OFF,	"F_ACC_OFF",
					F_D_P_HIT,	"F_D_P_HIT",
				};

	splhi();

	if ((Rcsr & CSRALGN) != 0) {
		Wcsr->c_align = 0;
		cmn_err(CE_PANIC, "KERNEL DATA ALIGNMENT ERROR");
	}

	if ((Rcsr & CSRPARE) != 0) {
		Wcsr->c_parity = 0;
		cmn_err(CE_PANIC, "SYSTEM PARITY ERROR INTERRUPT");
	}

	if ((Rcsr & CSRTIMO) != 0) {
		Wcsr->c_sanity = 0;
		cmn_err(CE_PANIC, "KERNEL BUS TIMEOUT");
	}

	if (ps.FT == ON_NORMAL)
		if (ps.ISC == XMEMFLT) {
			for (i = 0;
			  i < sizeof(mmu_err)/sizeof(mmu_err[0]); i++)
				if (mmu_err[i].errno == fltcr_type)
					break;
			if (mmu_err[i].errno == fltcr_type)
				cmn_err(CE_PANIC,
					"KERNEL MMU FAULT (%s)\n",
					mmu_err[i].name);
			else
				cmn_err(CE_PANIC,
					"KERNEL MMU FAULT (%d)\n",
					fltcr_type);
		} else
			cmn_err(CE_PANIC,
				"KERNEL MODE %s FAULT\n", isc_msg[ps.ISC]);
	else
		cmn_err(CE_PANIC,
			"KERNEL MODE FAULT, FT=%d, ISC=%d\n", ps.FT, ps.ISC);
}
