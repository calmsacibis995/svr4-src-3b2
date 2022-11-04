/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/mapin.c	1.2"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/sysmacros.h"
#include "sys/map.h"
#include "sys/immu.h"
#include "sys/mman.h"
#include "sys/proc.h"
#include "sys/buf.h"
#include "sys/debug.h"
#include "sys/cmn_err.h"

#include "vm/as.h"
#include "vm/seg.h"
#include "vm/hat.h"
#include "vm/anon.h"
#include "vm/page.h"
#include "vm/seg_kmem.h"

STATIC long mapin_count = 0;

#ifdef __STDC__
STATIC void bp_map(struct buf *, caddr_t);
#else
STATIC void bp_map();
#endif

/*
 * Map the data referred to by the buffer bp into the kernel
 * at kernel virtual address kaddr.  Used to map in data for
 * DVMA, among other things.
 */

STATIC void
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
			sdeptr = (sde_t *)kvtokstbl(addr);
			ppte = (pte_t *)vatopte(addr, sdeptr);
			ppte->pg_pte = (u_int)mkpte(PG_V, page_pptonum(pp));
			pp = pp->p_next;
			addr += PAGESIZE;
		}
	} else 
		cmn_err(CE_PANIC, "bp_map - non B_PAGEIO\n");
}

/*
 * Called to convert bp for pageio/physio to a kernel addressable location.
 * We allocate virtual space from the kernelmap and then use bp_map to do
 * most of the real work.
 */

void
bp_mapin(bp)
	register struct buf *bp;
{
	int npf, o;
	caddr_t addr;

	mapin_count++;

	if ((bp->b_flags & (B_PAGEIO | B_PHYS)) == 0 ||
	    (bp->b_flags & B_REMAPPED) != 0)
		return;		/* no pageio/physio or already mapped in */

	if ((bp->b_flags & (B_PAGEIO | B_PHYS)) == (B_PAGEIO | B_PHYS))
		cmn_err(CE_PANIC, "bp_mapin");

	o = (int)bp->b_un.b_addr & PAGEOFFSET;
	npf = btoc(bp->b_bcount + o);

	/*
	 * Allocate kernel virtual space for remapping.
	 */

	while ((addr = (caddr_t)malloc(sptmap, npf)) == 0) {
		mapwant(sptmap)++;
		(void) sleep((caddr_t)sptmap, PSWP);
	}
	addr = (caddr_t)((u_long)addr << PAGESHIFT);

	/* map the bp into the virtual space we just allocated */
	bp_map(bp, addr);

	bp->b_flags |= B_REMAPPED;
	bp->b_un.b_addr = addr + o;
}

/*
 * bp_mapout will release all the resources associated with a bp_mapin call.
 * We call hat_unload to release the work done by bp_map which will insure
 * that the reference and modified bits from this mapping are not OR'ed in.
 */

void
bp_mapout(bp)
	register struct buf *bp;
{
	register int npf, saved_npf;
	register pte_t *ppte;
	register sde_t *sdeptr;
	register struct page *pp;
	caddr_t addr;
	u_long saved_addr;

	mapin_count--;

	if (bp->b_flags & B_REMAPPED) {
		pp = bp->b_pages;
		npf = btoc(bp->b_bcount + ((int)bp->b_un.b_addr & PAGEOFFSET));
		saved_npf = npf;
		saved_addr = ((u_long)bp->b_un.b_addr & PAGEMASK);
		addr = (caddr_t)saved_addr;
		while (npf--) {
			ASSERT(pp != NULL);
			sdeptr = (sde_t *)kvtokstbl(addr);
			ppte = (pte_t *)vatopte(addr, sdeptr);
		/* don't want to propagate from this pte to the page struct */
			/* pp->p_ref |= ppte->pgm.pg_ref; */
			/* pp->p_mod |= ppte->pgm.pg_mod; */
			ppte->pg_pte = 0;
			flushaddr(addr);
			addr += PAGESIZE;
			pp = pp->p_next;
		}
		saved_addr = (saved_addr >> PAGESHIFT);
		rmfree(sptmap, saved_npf, saved_addr);
		bp->b_un.b_addr = (caddr_t)((int)bp->b_un.b_addr & PAGEOFFSET);
		bp->b_flags &= ~B_REMAPPED;
	}
}
