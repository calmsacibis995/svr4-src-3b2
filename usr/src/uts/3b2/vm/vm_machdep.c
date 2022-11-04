/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:vm/vm_machdep.c	1.18"
/*
 * UNIX machine dependent virtual memory support.
 */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/sysmacros.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/sbd.h"
#include "sys/immu.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/proc.h"
#include "sys/vm.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/cmn_err.h"
#include "sys/debug.h"
#include "sys/immu.h"

#include "sys/vmsystm.h"
#include "vm/hat.h"
#include "vm/as.h"
#include "vm/seg.h"
#include "vm/page.h"
#include "vm/seg_vn.h"
#include "vm/seg_kmem.h"
#include "vm/cpu.h"

/*
 * map_addr() is the routine called when the system is to
 * chose an address for the user.  We will  pick an address
 * range which is just above UVSHM.
 *
 * addrp is a value/result parameter. On input it is a hint from the user
 * to be used in a completely machine dependent fashion. We decide to
 * completely ignore this hint.
 *
 * On output it is NULL if no address can be found in the current
 * processes address space or else an address that is currently
 * not mapped for len bytes with a page of red zone on either side.
 * If align is true, then the selected address will obey the alignment
 * constraints of a vac machine based on the given off value.
 *
 * On the 3b2, we don't have a virtual address cache, so this arg is
 * ignored.
 *
 * We try to map everything into section 3 of the virtual address space.
 * This was done to allow the heap to grow to the section boundary. Also,
 * this makes the segment table for section 2 smaller while section 3 is
 * is the one that grows.
 */
/*ARGSUSED*/
void
map_addr(addrp, len, off, align)
        addr_t *addrp;
        register u_int len;
        off_t off;
        int align;
{
	register struct as *as = u.u_procp->p_as;
	addr_t	base;
	u_int	slen;

	base = (addr_t) UVSHM;
	slen = (addr_t) UVEND - base;
	len = (len + PAGEOFFSET) & PAGEMASK;

	/*
	 * Redzone for each side of the request. This is done to leave
	 * one page unmapped between segments. This is not required, but
	 * it's useful for the user because if their program strays across
	 * a segment boundary, it will catch a fault immediately making
	 * debugging a little easier.
	 */
	len += 2 * PAGESIZE;

	/*
	 * Look for a large enough hole starting above UVSHM.
	 * After finding it, use the lower part.
	 */
	if (as_gap(as, len, &base, &slen, AH_LO, (addr_t)NULL) == 0)
                *addrp = (addr_t)((u_int)base + PAGESIZE);
        else
                *addrp = NULL;	/* no more virtual space */
}

/*
 * Determine whether [base, base+len) contains a mapable range of
 * addresses at least minlen long. base and len are adjusted if
 * required to provide a mapable range.
 */

/* ARGSUSED */
int
valid_va_range(basep, lenp, minlen, dir)
	register addr_t *basep;
	register u_int *lenp;
	register u_int minlen;
	register int dir;
{

	register addr_t hi, lo;

	lo = *basep;
	hi = lo + *lenp;
	if (hi < lo ) 		/* overflow */
		return(0);
	if (hi - lo < minlen)
		return (0);
	return (1);
}

/*
 * Determine whether [addr, addr+len) are vaild user address.
 */
int
valid_usr_range(addr, len)
	register addr_t addr;
	register size_t len;
{
	register int start_scn, end_scn;
	register addr_t eaddr = addr + len;

	if (eaddr <= addr || eaddr > (addr_t)UVEND)
		return(0);

        start_scn = SECNUM(addr);
        end_scn = SECNUM(eaddr);

	if ((start_scn < SCN2) ||	/* kernel section 0,1  or   */
	    (start_scn != end_scn))	/* crosses section boundary */
		return(0);

	if (start_scn == SCN2)		/* SCN2 */
		return(1);

	if (SEGNUM(addr) != 0)		/* SCN3 and doesn't overlap u-area */
		return(1);
		
	return(0);
}
/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLBYTES.
 */

/* ARGSUSED */
void
pagemove(from, to, size)
	register caddr_t from, to;
	int size;
{
}


/*
 * Locate the next free virtual address (above UVSHM) within the given process.
 */
caddr_t
findvaddr(p)
	struct proc *p;
{
	register struct as *as;
	register struct seg *seg;
	register caddr_t vaddr;
	register caddr_t vaddrt;

	as = p->p_as;

	vaddr = (caddr_t) UVSHM;

	seg = as->a_segs;
	if (seg != NULL) {
		/*
		 * segs are sorted by base virtual address,
		 * and are in a circular list, so we can 
		 * easily get to the highest used virtual address.
		 */
		seg = seg->s_prev; 
		vaddrt = seg->s_base + seg->s_size;
		if (vaddr < vaddrt)
			vaddr = vaddrt;
	}

	vaddr = (caddr_t)((uint)(vaddr + NBPS - 1) & ~SOFFMASK);
	return vaddr;
}

/*
 * Fast inline data copy.  Data sizes MUST be an exact multiple 
 * of 64 bytes, and <src> and <dest> addresses must be word aligned.
 */

#if defined(lint)

#define inlinecopy(src, dest, len)	bcopy(src, dest, len)

#else

int	asm
inlinecopy(src, dest, len)
{
%	mem	src, dest, len;	lab	startmove;

	MOVW	src, %r0	
	MOVW	dest, %r1
	MOVW	len, %r2
	ARSW3	&0x06, %r2, %r2

startmove:

	MOVW	(%r0), (%r1)
	MOVW	4(%r0), 4(%r1)
	MOVW	8(%r0), 8(%r1)
	MOVW	12(%r0), 12(%r1)
	MOVW	16(%r0), 16(%r1)
	MOVW	20(%r0), 20(%r1)
	MOVW	24(%r0), 24(%r1)
	MOVW	28(%r0), 28(%r1)
	MOVW	32(%r0), 32(%r1)
	MOVW	36(%r0), 36(%r1)
	MOVW	40(%r0), 40(%r1)
	MOVW	44(%r0), 44(%r1)
	MOVW	48(%r0), 48(%r1)
	MOVW	52(%r0), 52(%r1)
	MOVW	56(%r0), 56(%r1)
	MOVW	60(%r0), 60(%r1)
	ADDW2	&64, %r0
	ADDW2	&64, %r1
	DECW	%r2
	BNEB	startmove
}

#endif

/*
 * Copy the data from the physical page represented by "frompp" to
 * to that represented by "topp".
 *
 * This routine should be made as fast as possible because it is a
 * high runner; e.g. copy on write pages. On the 3b2, function calls
 * are expensive. Also, we really don't have to do all of the boundary
 * checking bcopy() does because we know we have 2 page boundaries. For
 * efficiency we use the fast inlinecopy() routine above.
 */
void
ppcopy(frompp, topp)
	page_t *frompp;
	page_t *topp;
{
	register addr_t fromva, tova;

	ASSERT(frompp != NULL && topp != NULL);
	fromva = (addr_t) ptosv(page_pptonum(frompp));
	tova = (addr_t) ptosv(page_pptonum(topp));
	inlinecopy(fromva, tova, PAGESIZE);
}

/*
 * pagecopy() and pagezero() assume that they will not be called at
 * interrupt level.
 */

/*
 * Copy the data from `addr' to physical page represented by `pp'.
 * `addr' is a (user) virtual address which we might fault on.
 */
void
pagecopy(addr, pp)
	addr_t addr;
	page_t *pp;
{
	register addr_t va;

	ASSERT(pp != NULL);
	va = (addr_t) ptosv(page_pptonum(pp));
	(void) copyin((caddr_t)addr, va, PAGESIZE);
}

/*
 * Zero the physical page from off to off + len given by `pp'
 * without changing the reference and modified bits of page.
 * pagezero uses global CADDR2 and assumes that no one uses this
 * map at interrupt level and no one sleeps with an active mapping there.
 */
void
pagezero(pp, off, len)
	page_t *pp;
	u_int off, len;
{
	register caddr_t va;

	ASSERT((int)len > 0 && (int)off >= 0 && off + len <= PAGESIZE);
	ASSERT(pp != NULL);

	va = (caddr_t) ptosv(page_pptonum(pp));
	(void) bzero(va + off, len);
}

/* find an isolated hole in the user address space to use to
 * build the new stack image.
 * Prefer a hole that is aligned properly for a page table
 * so the page table(s) can be moved later.
 */

caddr_t
execstk_addr(size, hatflagp)
 int size;
 u_int *hatflagp;	/* to tell the hat whether a PT move or PTE copy
			 * is needed */
{
	addr_t hstart, secend;
	int	hsize;
	int sec;
	struct seg *sseg, *seg;
	addr_t eaddr, ptstart;
	addr_t svhstart = NULL;
#define SEGSIZE (1<<SEGNSHFT)

	/* we are looking for a hole of 'size' bytes
	 * at a page table boundary with a free page
	 * both before and aft (so segment concatenation does not occur).
	 * If we cannot find this, then relax the page table boundary
	 * criterion.
	 * An additional criterion would apply on machines that have
	 * stacks that grow to lower addresses.
	 * An overflow off the high address end should not appear
	 * to be a stack growth case.
	 * This allows the copyarglist code to skip string length checks.
	 */
	size += PAGESIZE;	/* for free page at end */
	sseg = seg = u.u_procp->p_as->a_segs;
	*hatflagp = 1;
	if (sseg == NULL)
		return((addr_t) UVBASE);
	for (sec = 2; sec < 4; sec++) {
		switch(sec) {
		case 2:
			hstart = (addr_t) UVBASE;
			secend = (addr_t) 0xbffff800;
			break;
		case 3:
			hstart = (addr_t) UVSTACK;
			secend = (addr_t) 0xfffff800;
			break;
		}
		do {
			eaddr = seg->s_base + seg->s_size;
			if (seg->s_base > hstart) {
				if (seg->s_base > secend)
					hsize = secend - hstart;
				else hsize = seg->s_base - hstart;
				if (hsize >= size && svhstart == NULL)
					svhstart = hstart;
				if (seg->s_base <= secend) {
					ptstart = (addr_t) ((u_int)seg->s_base
						& ~(SEGSIZE-1));
					hsize -= seg->s_base - ptstart;
					if (seg->s_base != ptstart)
						/* the isolation page */
						hsize += PAGESIZE;
				}
				ptstart = (addr_t)roundup((u_int)hstart, SEGSIZE);
				hsize -= ptstart - hstart;
				if (hsize >= size)
					return(ptstart);
			}
			if (eaddr >= hstart)
				/* need free page before hole */
				hstart = eaddr + PAGESIZE;
			if (hstart > secend)
				goto nextsec;
			if (seg->s_next == sseg) {
				/* it is the last segment, so all
				 * the rest is fair game.
				 */
				hsize = secend - hstart;
				if (hsize >= size && svhstart == NULL)
					svhstart = hstart;
				ptstart = (addr_t)roundup((u_int)hstart, SEGSIZE);
				hsize -= ptstart - hstart;
				if (hsize >= size)
					return(ptstart);
				if (sec == 2)
					goto nextsec;
			}
		} while (seg = seg->s_next, seg != sseg);
nextsec:	;
	}
	*hatflagp = 0;
	return(svhstart);
}
