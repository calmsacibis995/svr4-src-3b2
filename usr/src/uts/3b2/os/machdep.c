/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/machdep.c	1.26.1.8"
#include "sys/sysmacros.h"
#include "sys/param.h"
#include "sys/types.h"
#include "sys/psw.h"
#include "sys/immu.h"
#include "sys/buf.h"
#include "sys/cmn_err.h"
#include "sys/csr.h"
#include "sys/systm.h"
#include "sys/sit.h"
#include "sys/pcb.h"
#include "sys/errno.h"
#include "sys/time.h"
#include "sys/proc.h"
#include "sys/signal.h"
#include "sys/siginfo.h"
#include "sys/map.h"
#include "sys/reg.h"
#include "sys/sbd.h"
#include "sys/dma.h"
#include "sys/utsname.h"
#include "sys/acct.h"
#include "sys/cred.h"
#include "sys/vnode.h"
#include "sys/file.h"
#include "sys/fstyp.h"
#include "sys/user.h"
#include "sys/debug.h"
#include "sys/edt.h"
#include "sys/firmware.h"
#include "sys/var.h"
#include "sys/inline.h"
#include "sys/conf.h"
#include "sys/mau.h"
#include "sys/ucontext.h"
#include "sys/prsystm.h"
#include "sys/exec.h"

#include "vm/hat.h"
#include "vm/page.h"
#include "vm/seg.h"
#include "vm/seg_vn.h"

/*
 * The following flags are used to indicate the cause
 * of a level 8 interrupt.
 */

char	iswtchflag;

/*
 * The following flags are used to indicate the cause
 * of a level 9 (PIR) interrupt.
 */

char	timeflag;
char	uartflag;
char	pwrflag;

/*
 * The following access various locations in the firmware
 * vector table or edt.
 *
 * The number of entries in the edt.
 */

#define	VNUM_EDT	(*(((struct vectors *)VBASE)->p_num_edt))

/*
 * A pointer to the start of the edt.
 */

#define	VP_EDT		(((struct vectors *)VBASE)->p_edt)

/*
 * A pointer to an entry in the edt.
 */

#define	V_EDTP(X)	(VP_EDT + (X))

extern int mau_present;

/*
 * Adjust the time to UNIX-based time.
 * This routine must run at IPL15.
 */

void
clkset(oldtime)
	register time_t	oldtime;
{
	hrestime.tv_sec = (unsigned)oldtime;
}

/*
 * Request a PIR9.
 */

void
timepoke()
{
	timeflag = 1;
	Wcsr->s_pir9 = 0x1;
}

/*
 * The following table is indexed by the dma channel
 * number to find the address of the page register.
 */

struct r8	*dmapreg[4] = {
	(struct r8  *)(&dmaid),	/* Integral disk page reg. addr.   */
	(struct r8  *)(&dmaif),	/* Integral floppy page reg. addr. */
	(struct r8  *)(&dmaiuA),/* Integral uart A page reg. addr. */
	(struct r8  *)(&dmaiuB),/* Integral uart B page reg. addr. */
};

/*
 * Initialize the DMA controller.
 */

void
dmainit()
{
	/*
	 * Init the dmac signal level sense.
	 */
	dmac.RSR_CMD = RSTDMA;
}


/*
 * Routine used by the integral disk, integral floppy, and
 * the integral UARTs to access the DMAC.
 */
void
#ifdef __STDC__
dma_access(u_char chan, u_int baddr, u_int numbytes, u_char mode, u_char cmd)
#else
dma_access(chan, baddr, numbytes, mode, cmd)
	register u_char chan, mode, cmd;
	register u_int baddr, numbytes;
#endif
{
	register u_char	temp;
	register u_char	*chanselp;
	register		s;

	/*
	 * Raise priority level and save original.
	 */
	s = spltty();

	/*
	 * Load memory base address into dmac.
	 */
	chanselp = &dmac.C0CA;	/* Init channel select pointer.	*/
	dmac.CBPFF = NULL;	/* Init dmac byte ptr flip-flop */

	chanselp[chan*2] = (u_char)(baddr&0xff);   /* load byte3 */
	chanselp[chan*2] = (u_char)((baddr>>8)&0xff); /* load byte2 */
        temp = (u_char)((baddr>>16)&0xff);

	if ((unsigned)cmd >> 3)
		temp |= 0x80;
	else
		temp |= 0x00;
        dmapreg[chan]->data = temp;  	  /* load byte1 */

	/*
	 * Byte 0 is always 2000000 hex so it need not be loaded.
	 *
	 * Load byte count into dmac.
	 */
	dmac.CBPFF = NULL;	/* reinit dmac byte ptr flip-flop */
	numbytes -= 1;
	chanselp[chan*2+1] = (u_char)(numbytes & 0xff); /*load byte3*/
	chanselp[chan*2+1] = (u_char)((numbytes>>8)&0xff);/*load byte2*/

	/*
	 * Load transfer mode, command type, and channel into dmac,
	 * enable the channel, and restore interrupt priority level.
	 */
	dmac.WMODR = (mode | cmd | chan);
	dmac.WMKR = chan;
	splx(s);
}

extern int userstack[];
extern int exec_initialstk;

int
extractarg(args)
	struct uarg	*args;
{
	int error;
	extern int userstrlen();
	int fnsize;
	caddr_t nsp;
	int stgsize;
	int bsize, psize;
	caddr_t ptrstart;
	caddr_t cp;
	int ptrdelta;
	caddr_t argv0;

	/* how big is it? */

	stgsize = args->argsize + args->envsize + args->prefixsize;
	/* Now allow for the fname prefix change to old argv0: */
	if (args->prefixc) {
		fnsize = userstrlen(args->fname) + 1;
		if (fnsize < 0)
			return(EFAULT);
		fnsize++;	/* for the NULL byte */
		stgsize += fnsize;
	}
	stgsize = (stgsize + NBPW-1) & ~(NBPW-1);
	args->stringsize = stgsize;

	/* the 3 represents argc, and the two NULL ptrs terminating lists */
	bsize = (3 + args->argc + args->envc + args->prefixc) * NBPW
		+ stgsize
		+ exec_initialstk;
	psize = btoc(bsize);
	bsize = ctob(psize);

	/* now find a place in the user address space to put it */
	if ((nsp = execstk_addr(bsize, &args->estkhflag)) == NULL) {
		return (ENOMEM);
	}
	ptrdelta = (int)userstack - (int)nsp;
	args->estkstart = nsp;
	args->estksize = bsize;
	args->stacklow = (addr_t) userstack;
	/* and create it */
	if (error = as_map(u.u_procp->p_as, nsp, bsize, segvn_create, zfod_argsp))
		return error;

	/*
	 * now fill the temporary stack frame.
	 * The order is dependent on the direction of stack growth,
	 * an so, is machine-dependent.
	 * For stacks that grow towards larger addresses the order is:
	 * (low address)
	 * argv strings
	 * env strings
	 * prefix strings
	 * (fname if a prefix exists)
	 * argc
	 * prefix ptrs (no NULL ptr)
	 * argv ptrs (with NULL ptr)
	 *	(old argv0 points to fname copy if prefix exits)
	 * env ptrs (with NULL ptr)
	 * postfix values (put here later if they exist)
	 * (high address)
	 *
	 * For stacks that grow towards lower addresses the order is:
	 * (low address)
	 * argc
	 * prefix ptrs (no NULL ptr)
	 * argv ptrs (with NULL ptr)
	 *	(old argv0 points to fname copy if prefix exits)
	 * env ptrs (with NULL ptr)
	 * postfix values (put here later if they exist)
	 * argv strings
	 * env strings
	 * prefix strings
	 * (fname if a prefix exists)
	 * (high address)
	 *
	 * The 3b2 stack grow toward higher addresses.
	 */

	cp = nsp;
	ptrstart = cp + stgsize;
	if (suword((int *)ptrstart, args->argc + args->prefixc)) {
		as_unmap(u.u_procp->p_as, nsp, bsize);
		return EFAULT;
	}
	/* if non-zero fill gets put in, then either
	 * fix coparglist to deal with NULL ptr at end
	 * or add suword of NULL ptr for the two ptr arrays.
	 * Also, add bzero of last partial page and do the
	 * following initial stack page(s) as demand zero.
	 */
	if (copyarglist(args->argc, args->argp, ptrdelta,
	    (caddr_t *)(ptrstart + (1 + args->prefixc) * NBPW), cp, 0)
		!= args->argsize) {
		/* this can happen under heavy load with an swap I/O error */
		as_unmap(u.u_procp->p_as, nsp, bsize);
		return EFAULT;
	}
	if (copyarglist(args->envc, args->envp, ptrdelta,
	    (caddr_t *)(ptrstart + (1 + args->prefixc + args->argc + 1) * NBPW),
	    cp + args->argsize, 0) != args->envsize) {
		/* Besides the heavy load swap I/O error case,
		 * it can also happen if the argument list or strings
		 * are in shared writable pages and another process
		 * zaps them.
		 */
		as_unmap(u.u_procp->p_as, nsp, bsize);
		return EFAULT;
	}
	if (args->prefixc) {
		if (copyarglist(args->prefixc, args->prefixp, ptrdelta,
			(caddr_t *)(ptrstart + NBPW),
			cp + args->argsize + args->envsize, 1)
			!= args->prefixsize) {
			as_unmap(u.u_procp->p_as, nsp, bsize);
			return EFAULT;
		}
		if (copyout(args->fname, (argv0 =
		    cp + args->argsize + args->envsize + args->prefixsize),
		    fnsize)) {
			as_unmap(u.u_procp->p_as, nsp, bsize);
			return EFAULT;
		}
		argv0 += ptrdelta;
		if (suword((int *)(ptrstart + NBPW * (1 + args->prefixc)),
				(int)argv0)) {
			as_unmap(u.u_procp->p_as, nsp, bsize);
			return EFAULT;
		}
	}
	args->stackend =
	    ptrstart + (args->argc + args->envc + args->prefixc + 3) * NBPW
		+ ptrdelta;
	return 0;
}	

/*
 * machine dependent final setup code goes in setregs().
 */
int
setregs(args)
	register struct uarg *args;
{
	register int i;
	register char *sp, *cp;
	register struct proc *p = u.u_procp;


        /*
         * Do psargs.
	 */
	sp = u.u_psargs;
	cp = (char *)userstack;
	i = min(args->argsize, PSARGSZ -1);
	if (copyin(cp, sp, i))
		return(EFAULT);
	while (i--){
		if (*sp == '\0')
			*sp = ' ';
		sp++;
	}
	*sp = '\0';

	p->p_stksize = howmany(args->estksize, sizeof(int *));
	p->p_stkbase = userstack;

	u.u_pcb.sub = (int *)((u_int)userstack + args->estksize);

	ASSERT((u_int)args->stackend <= (u_int)userstack + args->estksize);
	u.u_ar0[SP] = (int)args->stackend;
	u.u_ar0[FP] = 0;
	u.u_ar0[AP] = (int)userstack + args->stringsize;
	u.u_ar0[PC] = (int)u.u_exdata.ux_entloc;

	if (mau_present)
		mau_setup();

	return 0;
}

/*
 * Allocate 'size' units from the given map,
 * returning the base of an area which starts on a "mask" boundary.
 * That is, which has all of the bits in "mask" off in the starting
 * address.  Mask is assummed to be (2**N - 1).
 * Algorithm is first-fit.
 */
int
ptmalloc(mp, size, mask)
	struct map *mp;
	int size;
	int mask;
{
	register int a, b;
	register int gap, tail;
	register struct map *bp;

	ASSERT(size >= 0);
	for (bp = mapstart(mp); bp->m_size; bp++) {
		if (bp->m_size >= size) {
			a = bp->m_addr;
			b = (a+mask) & ~mask;
			gap = b - a;
			if (bp->m_size < (gap + size))
				continue;
			if (gap != 0) {
				tail = bp->m_size - size - gap;
				bp->m_size = gap;
				if (tail) 
					mfree(mp, tail, bp->m_addr+gap+size);
			} else {
				bp->m_addr += size;
				if ((bp->m_size -= size) == 0) {
					do {
						bp++;
						(bp-1)->m_addr = bp->m_addr;
					} while ((bp-1)->m_size = bp->m_size);
					mapsize(mp)++;
				}
			}
			ASSERT(bp->m_size < (unsigned) 0x80000000);
			return b;
		}
	}
	return 0;
}


/*
 * devcheck() is an obsolete routine that is kept in to maintain
 * driver compatibility. The dev_addr array is now set up by lboot
 * at system boot time and can be accessed by an external symbol
 * "<driver name>_addr". The dev_cnt that was returned by devcheck()
 * can be obtained by defining a variable in the /etc/master.d
 * file and initializing it to "#D". The variable should be of the
 * form "<driver name>_cnt" to make all of the drivers consistent.
 */

devcheck(devtype, dev_addr)
	int devtype;
	paddr_t dev_addr[];
{
	int dev_cnt;
	int i;
 
	dev_cnt = 0;
	for (i = 0; i < VNUM_EDT; i++) {
		if (V_EDTP(i)->opt_code == devtype)
			dev_addr[dev_cnt++] =
			   (paddr_t)((17 * V_EDTP(i)->opt_slot - 14) * NBPS);
	}

	return dev_cnt;
}

u_char
getvec(baddr)
	register long baddr;
{
	/*
	 * Simulate the system routine that will supply the
	 * interrupt vector given a peripheral board address.
	 */

	return ((((baddr / 131072) - 3) / 17) + 1) << 4;
}

servicing_interrupt()
{
	int dummy;

	return secnum(&dummy) != 3;
}

/* ARGSUSED */
int
buscheck(bp)
	struct buf *bp;
{
	return 0;
}

/*
 * This function is called to check that a psw is suitable for
 * running in user mode.  If not, it is fixed to make it
 * suitable.  This is necessary when a psw is saved on the user
 * stack where some sneaky devil could set kernel mode or
 * something.
 */

void
fixuserpsw(psp)
	register psw_t	*psp;
{
	extern char	u400;	/* Set non-zero if we are using	*/
				/* the cache.			*/

	/*
	 * We never use quick interrupt so make sure that is
	 * off.  Will crash the system otherwise since there
	 * are no quick interrupt vectors set up.
	 */

	psp->QIE	= 0;

	/*
	 * See if we are using the cache or not and set the
	 * cache flush disable and cache disable bits accordingly.
	 */

	if (u400) {
		psp->CSH_F_D	= 0;
		psp->CSH_D	= 0;
	} else {
		psp->CSH_F_D	= 1;
		psp->CSH_D	= 1;
	}

	/*
	 * The interrupt priority must be zero so that no
	 * interrupts are blocked when in user mode.  The
	 * current and previous modes are both set to user.
	 */

	psp->IPL	= 0;
	psp->CM		= PS_USER;
	psp->PM		= PS_USER;

	/*
	 * Turn off the register save/restore and initial psw
	 * bits.  The caller can turn them back on if they're
	 * wanted.
	 */

	psp->R		= 0;
	psp->I		= 0;
}

void
restorecontext(ucp)
	register ucontext_t *ucp;
{
	register proc_t *pp = u.u_procp;

	u.u_oldcontext = ucp->uc_link;

	if (ucp->uc_flags & UC_STACK) {
		if (pp->p_stkbase != ucp->uc_stack.ss_sp) {
			pp->p_stkbase = ucp->uc_stack.ss_sp;
			u.u_pcb.slb = ucp->uc_stack.ss_sp;
			pp->p_stksize = ucp->uc_stack.ss_size;
			u.u_pcb.sub = pp->p_stkbase + pp->p_stksize;
			if (ucp->uc_stack.ss_sp == u.u_altsp)
				u.u_altflags |= SS_ONSTACK;
			else
				u.u_altflags &= ~SS_ONSTACK;
		}
	}

	if (ucp->uc_flags & UC_CPU)
		prsetregs(PTOU(pp), ucp->uc_mcontext.gregs);

	if ((ucp->uc_flags & UC_MAU) && prhasfp())
		prsetfpregs(pp, &ucp->uc_mcontext.fpregs);

	if (ucp->uc_flags & UC_SIGMASK) {
		sigutok(&ucp->uc_sigmask,&u.u_procp->p_hold);
		sigdiffset(&u.u_procp->p_hold,&cantmask);
	}
}

void
savecontext(ucp, mask)
	register ucontext_t *ucp;
	k_sigset_t mask;
{
	register proc_t *pp = u.u_procp;

	ucp->uc_flags = UC_ALL;
	ucp->uc_link = u.u_oldcontext;

	/*
	 * Save current stack state.
	 */
        ucp->uc_stack.ss_sp = pp->p_stkbase;
        ucp->uc_stack.ss_size = pp->p_stksize;
        ucp->uc_stack.ss_flags = 0;

	/*
	 * Save machine context.
	 */
	prgetregs(PTOU(pp), ucp->uc_mcontext.gregs);
	if (prhasfp())
		prgetfpregs(pp, &ucp->uc_mcontext.fpregs);
	else
		ucp->uc_flags &= ~UC_MAU;

	/*
	 * Save signal mask.
	 */
	sigktou(&mask,&ucp->uc_sigmask);
}

/*
 * Dispatch signal handler.
 */
psw_t	sendsig_psw = SIGPSW;

int
sendsig(sig, hdlr)
	int sig;
	register void (*hdlr)();
{
	ucontext_t uc;
	psw_t ill_psw;
	siginfo_t si;
	int newstack;		/* if true, switching to alternate stack */
	int stacksz;		/* size of stack required to catch signal */
 	register int *sp, *ap;
	proc_t *p = u.u_procp;
	struct {
		int signo;
		siginfo_t *sip;
		ucontext_t *ucp;
	} argpframe;
	struct {
		void	(*pc)();
		psw_t	psw;
	} retgframe;

	argpframe.signo = sig;

 	stacksz = 
	  sizeof(ucontext_t) + 	/* user context structure */
	  sizeof(argpframe) + 	/* current signal */
	  sizeof(retgframe); 	/* for binary compatibility */

	bzero((caddr_t)&si, sizeof(si));
	if (p->p_curinfo && sigismember(&p->p_siginfo, sig)) {
		bcopy((caddr_t)&p->p_curinfo->sq_info,
		  (caddr_t)&si, sizeof(k_siginfo_t));
		stacksz += sizeof(siginfo_t);
	}
 
 	newstack = (sigismember(&u.u_sigonstack, sig)
	   && !(u.u_altflags & (SS_ONSTACK|SS_DISABLE)));

 	if (newstack != 0) {
 		if (stacksz >= u.u_altsize)
			return 0;
 		sp = (int *)u.u_altsp;
 	} else {
 		register int *sub;
 		sub = u.u_pcb.sp + stacksz / NBPW;
 		if (sub >= u.u_pcb.sub && !grow(sub))
			return 0;
 		sp = u.u_pcb.sp;
	}

	if (p->p_curinfo && sigismember(&p->p_siginfo, sig)) {
		if (copyout((caddr_t)&si, (caddr_t)sp, sizeof(siginfo_t)) < 0)
			return 0;
		argpframe.sip = (siginfo_t *)sp;
		sp += sizeof(siginfo_t) / NBPW;
	} else
		argpframe.sip = NULL;

	savecontext(&uc, u.u_sigoldmask);
	if (copyout((caddr_t)&uc, (caddr_t)sp, sizeof(ucontext_t)) < 0)
		return 0;

	argpframe.ucp = (ucontext_t *)sp;
	sp += sizeof(ucontext_t) / NBPW;
	if (copyout((caddr_t)&argpframe, (caddr_t)sp, sizeof(argpframe)) < 0)
		return 0;

	ap = sp;
	sp += sizeof(argpframe) / NBPW;

	/*
	 * Force an illegal change, if handler should return via RETG.
	 * This should only be used by old code: upon return from the
	 * signal handler, the copy of the pc and psw in the ucontext
	 * structure will be restored, not this one.
	 */

	ill_psw = u.u_pcb.psw;
	ill_psw.CM = 01;		/* Force an illegal change. */
	retgframe.psw = ill_psw;
	retgframe.pc = u.u_pcb.pc;
	if (copyout((caddr_t)&retgframe, (caddr_t)sp, sizeof(retgframe)) < 0)
		return 0;
	sp += sizeof(retgframe) / NBPW;

	/*
	 * Now that we can no longer fault, update the u-block.
	 */

	/*
	 * Push context.
	 */
	u.u_oldcontext = argpframe.ucp;

	u.u_pcb.pc = hdlr;
	u.u_pcb.sp = (int *)sp;
	u.u_pcb.regsave[K_AP] = (int)ap;
	u.u_pcb.psw = sendsig_psw;
	u.u_pcb.psw.OE = ill_psw.OE;

	if (newstack) {
		u.u_altflags |= SS_ONSTACK;
 		u.u_pcb.slb = (int *)u.u_altsp;
		u.u_pcb.sub = u.u_altsp + u.u_altsize;
		u.u_procp->p_stkbase = u.u_altsp;
		u.u_procp->p_stksize = howmany(u.u_altsize, sizeof(int *));
	}

	return 1;
}

/*
 * Fixup after return from signal handler via RETG instruction.
 * Returns the value of ap at the time the signal was delivered,
 * so that it may be restored in ttrap.s.
 */
int *
retsig(ucp)
	register ucontext_t *ucp;
{
	u.u_oldcontext = ucp->uc_link;

	u.u_ipcb.sp = (int *)ucp;
	u.u_ipcb.pc = (void(*)())ucp->uc_mcontext.gregs[R_PC];
	u.u_ipcb.psw = *(psw_t *)&(ucp->uc_mcontext.gregs[R_PS]);

	fixuserpsw(&u.u_ipcb.psw);

	return (int *)(ucp->uc_mcontext.gregs[R_AP]);
}

long mapin_count = 0;

/*
 * Map the data referred to by the buffer bp into the kernel
 * at kernel virtual address addr.  
 */

void
bp_map(bp, addr)
	register struct buf *bp;
	caddr_t addr;
{
	register struct page *pp;
	register int npf;
	register pte_t	*ppte;
	register sde_t	*sdeptr;

	npf = btoc(bp->b_bcount + ((int)bp->b_un.b_addr & PAGEOFFSET));

	if (bp->b_flags & B_PAGEIO) {
		pp = bp->b_pages;
		ASSERT(pp != NULL);
		while (npf--) {
			flushaddr(addr);
			sdeptr = kvtokstbl(addr);
			ppte = vatopte((caddr_t)addr, sdeptr);
			ppte->pg_pte = (u_int)mkpte(PG_V, page_pptonum(pp));
			pp = pp->p_next;
			addr += PAGESIZE;
		}
	} else 
		cmn_err(CE_PANIC, "bp_map - non B_PAGEIO\n");
}

/*
 * Called to convert bp for pageio to a kernel addressable location.
 * We allocate virtual space from the sptmap and then use bp_map to do
 * most of the real work.
 */

void
bp_mapin(bp)
	register struct buf *bp;
{
	int npf, o;
	caddr_t kaddr;

	mapin_count++;

	if ((bp->b_flags & (B_PAGEIO | B_PHYS)) == 0 ||
	    (bp->b_flags & B_REMAPPED) != 0)
		return;		/* no pageio or already mapped in */

	if ((bp->b_flags & (B_PAGEIO | B_PHYS)) == (B_PAGEIO | B_PHYS))
		cmn_err(CE_PANIC, "bp_mapin");

	o = (int)bp->b_un.b_addr & PAGEOFFSET;
	npf = btoc(bp->b_bcount + o);

	/*
	 * Allocate kernel virtual space for remapping.
	 */
	while ((kaddr = (caddr_t)malloc(sptmap, npf)) == 0) {
		mapwant(sptmap)++;
		(void) sleep((caddr_t)sptmap, PSWP);
	}
	kaddr = (caddr_t)((int)kaddr << PAGESHIFT);

	/* map the bp into the virtual space we just allocated */
	bp_map(bp, kaddr);

	bp->b_flags |= B_REMAPPED;
	bp->b_un.b_addr = kaddr + o;
}

/*
 * bp_mapout will release all the resources associated with a bp_mapin call.
 */
void
bp_mapout(bp)
	register struct buf *bp;
{
	register int npf, saved_npf;
	register pte_t *ppte;
	register sde_t *sdeptr;
	register struct page *pp;
	caddr_t addr, saved_addr;

	mapin_count--;

	if (bp->b_flags & B_REMAPPED) {
		pp = bp->b_pages;
		npf = btoc(bp->b_bcount + ((int)bp->b_un.b_addr & PAGEOFFSET));
		saved_npf = npf;
		saved_addr = addr = (caddr_t)((int)bp->b_un.b_addr & PAGEMASK);
		while (npf--) {
			ASSERT(pp != NULL);
			sdeptr = kvtokstbl(addr);
			ppte = vatopte(addr, sdeptr);
			ppte->pg_pte = 0;
			flushaddr(addr);
			addr += PAGESIZE;
			pp = pp->p_next;
		}
		saved_addr = (caddr_t)((int)saved_addr >> PAGESHIFT);
		rmfree(sptmap, saved_npf, (u_long)saved_addr);
		bp->b_un.b_addr = (caddr_t)((int)bp->b_un.b_addr & PAGEOFFSET);
		bp->b_flags &= ~B_REMAPPED;
	}
}
