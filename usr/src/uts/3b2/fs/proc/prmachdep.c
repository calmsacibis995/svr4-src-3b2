/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/proc/prmachdep.c	1.7"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/cred.h"
#include "sys/debug.h"
#include "sys/immu.h"
#include "sys/inline.h"
#include "sys/kmem.h"
#include "sys/proc.h"
#include "sys/reg.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/sbd.h"
#include "sys/signal.h"
#include "sys/user.h"

#include "sys/fault.h"
#include "sys/syscall.h"
#include "sys/procfs.h"

#include "vm/as.h"
#include "vm/page.h"

#include "prdata.h"

#define	UADDR	((caddr_t) 0xC0000000)	/* u-area virtual address */

/*
 * Map a target process's u-block in and out.  prumap() makes it addressable
 * (if necessary) and returns a pointer to it.
 */
struct user *
prumap(p)
	register proc_t *p;
{
	/*
	 * With paged u-blocks, this is easy: we just use the address kept in
	 * the proc-table entry.
	 */
	return KUSER(p->p_segu);
}

/* ARGSUSED */
void
prunmap(p)
	proc_t *p;
{
	/*
	 * With paged u-blocks, there's nothing to do in order to unmap.
	 */
}

/*
 * Return general registers.  The u-block must already have been mapped
 * in (via prumap()) by the caller, who supplies its address.
 */
void
prgetregs(up, rp)
	register user_t *up;
	register gregset_t rp;
{
	register greg_t *reg;

	reg = (greg_t *) up->u_ar0;
	reg = (greg_t *) ((caddr_t) up + ((caddr_t) reg - UADDR));
	rp[R_R0] = reg[R0];
	rp[R_R1] = reg[R1];
	rp[R_R2] = reg[R2];
	rp[R_R3] = reg[R3];
	rp[R_R4] = reg[R4];
	rp[R_R5] = reg[R5];
	rp[R_R6] = reg[R6];
	rp[R_R7] = reg[R7];
	rp[R_R8] = reg[R8];
	rp[R_FP] = reg[FP];
	rp[R_AP] = reg[AP];
	rp[R_PS] = reg[PS];
	rp[R_SP] = reg[SP];
	rp[R_PCBP] = (int) up->u_pcbp;
	rp[R_ISP] = 0;
	rp[R_PC] = reg[PC];
}

/*
 * Set general registers.  The u-block must already have been mapped
 * in (via prumap()) by the caller, who supplies its address.
 */
void
prsetregs(up, rp)
	register user_t *up;
	register gregset_t rp;
{
	register greg_t *reg;

	reg = (greg_t *) up->u_ar0;
	reg = (greg_t *) ((caddr_t) up + ((caddr_t) reg - UADDR));
	reg[R0] = rp[R_R0];
	reg[R1] = rp[R_R1];
	reg[R2] = rp[R_R2];
	reg[R3] = rp[R_R3];
	reg[R4] = rp[R_R4];
	reg[R5] = rp[R_R5];
	reg[R6] = rp[R_R6];
	reg[R7] = rp[R_R7];
	reg[R8] = rp[R_R8];
	reg[FP] = rp[R_FP];
	reg[AP] = rp[R_AP];
	reg[SP] = rp[R_SP];
	reg[PC] = rp[R_PC];

	/*
	 * Only the condition-code and trace-enable bits of the PSW
	 * can be modified.
	 */
	((psw_t *)(&reg[PS]))->NZVC = ((psw_t *)&rp[R_PS])->NZVC;
	((psw_t *)(&reg[PS]))->TE = ((psw_t *)&rp[R_PS])->TE;
}

/*
 * Is there floating-point hardware?
 */
int
prhasfp()
{
	extern int mau_present;

	return mau_present;
}

/*
 * Get floating-point registers.  The u-block is mapped in here (not by
 * the caller).
 */
void
prgetfpregs(p, fp)
	proc_t *p;
	register fpregset_t *fp;
{
	user_t *up = prumap(p);
	register struct mau_st *mp = &up->u_mau;
	register int i, j;

	fp->f_asr = mp->asr;
	for (i = 0; i < 3; i++) {
		fp->f_dr[i] = mp->dr[i];
		for (j = 0; j < 4; j++)
			fp->f_fpregs[j][i] = mp->fpregs[j][i];
	}
	prunmap(p);
}

/*
 * Set floating-point registers.  The u-block is mapped in here (not by
 * the caller).
 */
void
prsetfpregs(p, fp)
	proc_t *p;
	register fpregset_t *fp;
{
	user_t *up = prumap(p);
	register struct mau_st *mp = &up->u_mau;
	register int i, j;

	mp->asr = fp->f_asr;
	for (i = 0; i < 3; i++) {
		mp->dr[i] = fp->f_dr[i];
		for (j = 0; j < 4; j++)
			mp->fpregs[j][i] = fp->f_fpregs[j][i];
	}
	prunmap(p);
}

/*
 * Return the "addr" field for pr_addr in prpsinfo_t.
 */
caddr_t
prgetpsaddr(p)
	register proc_t *p;
{
	return (caddr_t)ubptbl(p);
}

/*
 * Set the PSW to single-step the process.
 */
/* ARGSUSED */
void
prstep(p, up)
	proc_t *p;
	register user_t *up;
{
	register int *reg = up->u_ar0;

	reg = (int *) ((caddr_t) up + ((caddr_t) reg - UADDR));
	((psw_t *)&reg[PS])->TE = 1;
}

/*
 * Set the PC to the specified virtual address.
 */
/* ARGSUSED */
void
prsvaddr(p, up, vaddr)
	proc_t *p;
	register user_t *up;
	caddr_t vaddr;
{
	register int *reg = up->u_ar0;

	reg = (int *) ((caddr_t) up + ((caddr_t) reg - UADDR));
	reg[PC] = (int) vaddr;
}

/*
 * Map address "addr" in process "p" into a kernel virtual address.
 * The memory is guaranteed to be resident and locked down.
 */
/* ARGSUSED */
caddr_t
prmapin(p, addr, writing)
	proc_t *p;
	caddr_t addr;
	int writing;
{
	caddr_t vaddr;
	extern paddr_t vtop();

	/*
	 * On the 3B2 this is easy: physical addresses are mapped into
	 * their equivalent virtual addresses.
	 */
	vaddr = (caddr_t) vtop(addr, p);
	ASSERT(vaddr != NULL);
	return vaddr;
}

/*
 * Unmap address "addr" in process "p"; inverse of prmapin().
 */
/* ARGSUSED */
void
prmapout(p, addr, vaddr, writing)
	proc_t *p;
	caddr_t addr;
	caddr_t vaddr;
	int writing;
{
	/*
	 * Nothing to do on the 3B2.
	 */
}

/*
 * Short-cut fast mapping-in of a process's page: if the page is already
 * resident and copy-on-write processing is not required, return a pointer
 * to the page.  This is a performance hack which may not be meaningful or
 * easy to implement on all systems, in which case this routine should
 * simply return NULL.
 *
 * The 3B2 version is a modified version of vtop() (from vm_hat.c).  It
 * computes the physical address of the page and returns it as a virtual
 * address.
 */
caddr_t
prfastmapin(p, addr, writing)
	register proc_t *p;
	caddr_t addr;
	int writing;
{
	register sde_t *sp, *sde_p;
	register pte_t *pte_p;
	int sid, s, a = (int)addr;

	s = splhi();
	sid = SECNUM(a);
	ASSERT(sid != SCN0 && sid != SCN1);
	sp = (sde_t *)p->p_as->a_hat.hat_srama[sid - SCN2];
	sde_p = &sp[SEGNUM(a)];
	if (SD_INDIRECT(sde_p))
		sde_p = (sde_t *)(sde_p->wd2.address);
	if ((sde_p->seg_flags & SDE_flags) != SDE_flags
	  || (a & MSK_IDXSEG) > (long)motob(sde_p->seg_len)) {
		splx(s);
		return NULL;
	}
	ASSERT(!SD_ISCONTIG(sde_p));
	pte_p = (pte_t *)((int)sde_p->wd2.address & ~0x1F);
	pte_p = &pte_p[(a >> 11) & 0x3F];
	if (!pte_p->pgm.pg_v || (writing && pte_p->pgm.pg_w)) {
		splx(s);
		return NULL;
	}
	PAGE_HOLD(page_numtopp(pte_p->pgm.pg_pfn));
	splx(s);
	return (caddr_t)(pfntophys(pte_p->pgm.pg_pfn) + PAGOFF(a));
}

/*
 * Inverse of prfastmapin().
 */
/* ARGSUSED */
void
prfastmapout(p, addr, vaddr, writing)
	proc_t *p;
	caddr_t addr;
	caddr_t vaddr;
	int writing;
{
	PAGE_RELE(page_numtopp(phystopfn(vaddr)));
}
