/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:vm/vm_hat.c	1.49"

/*
 * VM - Hardware Address Translation management.
 *
 * The hat layer manages the address translation hardware as a cache
 * driven by calls from the higher levels in the VM system.  Nearly
 * all the details of how the hardware is managed shound not be visible
 * above this layer except for miscellaneous machine specific functions
 * (e.g. mapin/mapout) that work in conjunction with this code.  Other
 * than a small number of machine specific places, the hat data
 * structures seen by the higher levels in the VM system are opaque
 * and are only operated on by the hat routines.  Each address space
 * contains a struct hat and a page contains an opaque pointer which
 * is used by the hat code to hold a list of active translations to
 * that page.
 *
 * Notes:
 *
 *	The p_mapping list hanging off the page structure is protected
 *	by spl's in the Sun code. Currently, we do not have to do this
 *	because our mappings are not interrupt replaceable.
 *
 *	It is assumed by this code:
 *
 *		- no load/unload requests will span section boundaries.
 *
 *		-
 *
 */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/immu.h"
#include "sys/vnode.h"
#include "sys/mman.h"
#include "sys/bitmasks.h"
#include "sys/tuneable.h"
#include "sys/cmn_err.h"
#include "sys/debug.h"
#include "sys/sysmacros.h"
#include "sys/inline.h"
#include "sys/errno.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/proc.h"
#include "sys/signal.h"
#include "sys/fs/s5dir.h"
#include "sys/user.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "vm/vm_hat.h"
#include "vm/hat.h"
#include "vm/seg.h"
#include "vm/as.h"
#include "vm/page.h"
#include "vm/anon.h"
#include "vm/seg_vn.h"

#ifdef DEBUG

/*
 * Single thread locking on mappings; not necessarily true but useful
 * for initial debugging.
 */
#define	LOCK_THREAD	1

u_int hat_debug = 0x0;
u_int hat_duppts = 1;
u_int hat_mappts = 1;
u_int hat_ptpgs = 0xffffffff;
u_int hat_pgcnt = 0;
u_int hat_pgfail = 0;
u_int hat_ptstolen = 0;

#define RET_ADDR(last_arg)	(*((long *)&last_arg + 1))

#define	HAT_MAX_LOG	200

long hat_log_ptr = 0;
long hat_log_ovr = 0;

struct hat_log_entry {
	char  *hl_str;
	u_long hl_x;
	u_long hl_y;
	u_long hl_z;
} hat_log[HAT_MAX_LOG];

#define	HAT_LOG(cond, str, x, y, z) \
	{ \
		if (cond) { \
			if (hat_log_ptr == HAT_MAX_LOG) { \
				hat_log_ovr = 1; \
				hat_log_ptr = 0; \
			} \
			hat_log[hat_log_ptr].hl_str = str; \
			hat_log[hat_log_ptr].hl_x = (u_long)x; \
			hat_log[hat_log_ptr].hl_y = (u_long)y; \
			hat_log[hat_log_ptr].hl_z = (u_long)z; \
			++hat_log_ptr; \
		} \
	}

printhatlog()
{
	register int i;

	if (hat_log_ovr != 0) {
		for(i = hat_log_ptr; i < HAT_MAX_LOG; i++) {
			cmn_err(CE_CONT, "%s: %x %x %x\n",
				hat_log[i].hl_str,
				hat_log[i].hl_x,
				hat_log[i].hl_y,
				hat_log[i].hl_z);
			dodmddelay();
		}
	}

	for(i = 0; i < hat_log_ptr; i++) {
		cmn_err(CE_CONT, "%s: %x %x %x\n",
			hat_log[i].hl_str,
			hat_log[i].hl_x,
			hat_log[i].hl_y,
			hat_log[i].hl_z);
		dodmddelay();
	}
}

#define	CHECK_PTE(seg, pte) \
	{ \
		if (seg == segkmap) \
			ASSERT(pte >= ksegmappt && pte < eksegmappt); \
		else if (seg == segu) \
			ASSERT(pte >= ksegupt && pte < eksegupt); \
		else \
			ASSERT((pte < ksegmappt || pte >= eksegmappt) && \
			       (pte < ksegupt   || pte >= eksegupt)   ); \
	}
#else
#define	HAT_LOG(cond, str, x, y, z)
#define	CHECK_PTE(seg, pte)
#endif

struct as kas;

extern struct	seg *segkmap;	/* kernel generic mapping segment */
extern int	kvsegmap[];
extern pte_t	*ksegmappt;
extern pte_t	*eksegmappt;
extern struct	seg *segu;	/* kernel u-area mapping segment */
extern int	kvsegu[];
extern pte_t	*ksegupt;
extern pte_t	*eksegupt;

STATIC void	hat_pteload();
STATIC u_int	hat_growsdt();
STATIC u_int	hat_sdtalloc();
STATIC void	hat_sdtfree();
STATIC pte_t	*hat_ptalloc();
STATIC void	hat_ptfree();
STATIC ptdat_t	*hat_pt2ptdat();

STATIC u_int	pt_waiting;	/* Number of procs waiting for a pdt.   */
STATIC ptdat_t	free_pts;  	/* List of free page tables.            */
STATIC ptdat_t	active_pts;	/* List of active page tables.          */
STATIC page_t	sdtfreelist;	/* List of pgs that contain free sdt's. */

STATIC int	sde_invalid[9];	/* invalid sde used for initialization. */
SRAMA		mmu_invalid;

/* XXX - ublock kludge, default segment table for section 3. */

STATIC int	dflt_sdt[9];
SRAMA		dflt_sdt_p;

/*
 * Initialize the hardware address translation structures.
 * Called by startup() after the vm structures have been allocated
 * and mapped in.
 */
void
hat_init()
{
	/*
	 * Initialize the segment table and page table lists.
	 */
	sdtfreelist.p_prev = sdtfreelist.p_next = &sdtfreelist;
	free_pts.pt_prev = free_pts.pt_next = &free_pts;
	active_pts.pt_prev = active_pts.pt_next = &active_pts;

	/*
	 * We don't have to do anything to initialize the invalid SDT because
	 * it's guaranteed to be all zeroes. Hence, the valid bit won't be on.
	 *
	 * Note, the address of an SDT must be on a 32 byte boundary.
	 */
	mmu_invalid = ((kvtophys((caddr_t)sde_invalid) + 0x1F) & ~0x1F);

	/*
	 * XXX - ublock kludge, set up a default segment table that is
	 * to be used when a process has no as, hence no hat, structure.
	 *
	 * Note, the address of an SDT must be on a 32 byte boundary.
	 */
	dflt_sdt_p = ((kvtophys((caddr_t)dflt_sdt) + 0x1F) & ~0x1F);
	((sde_t *)dflt_sdt_p)->seg_prot = KRWE;
	((sde_t *)dflt_sdt_p)->seg_len = ctomo(USIZE);
	((sde_t *)dflt_sdt_p)->seg_flags = SDE_flags;
}

/*
 * Allocate hat structure for address space (as).
 * Called from as_alloc() when address space is being set up.
 */
void
hat_alloc(as)
	register struct as *as;
{
	register hat_t *hatp = &as->a_hat;

	/*
	 * Initialize srama's to mmu_invalid  and dflt_sdt to
	 * indicate segment tables for sections 2 and 3 have not
	 * yet been allocated.
	 */
	hatp->hat_srama[HAT_SCN2] = mmu_invalid;
	hatp->hat_srama[HAT_SCN3] = dflt_sdt_p;
	*((int *)&hatp->hat_sramb[HAT_SCN2]) = 0;
	*((int *)&hatp->hat_sramb[HAT_SCN3]) = 0;
}

/*
 * Free all of the translation resources for the specified address space.
 * Called from as_free when the address space is being destroyed.
 */
void
hat_free(as)
	struct as *as;
{
	register sde_t *sde, *lastsde;
	register pte_t *pte, *lastpte;
	register pte_t **nextpte;
	register hat_t *hatp = &as->a_hat;
	pte_t	*pt;
	page_t	*pp;
	int	section;
	int	s;

	/*
	 * XXX - ublock kludge, the hat structure is about to be blown away.
	 * If it is the current process, we must switch segment tables to the
	 * default segment table so the ublock will still be accessible.
	 *
	 * Note, this also takes care of entirely flushing the mmu cache for
	 * sections 2 and 3.
	 */

	if (as == u.u_procp->p_as) {
		s = splhi();
		srama[SCN2] = mmu_invalid;
		((int *)sramb)[SCN2] = 0;
		((sde_t *)dflt_sdt_p)->wd2.address =
			phys_ubptbl(u.u_procp->p_ubptbl);
		srama[SCN3] = dflt_sdt_p;
		((int *)sramb)[SCN3] = 0;
		splx(s);
	}

	if (hatp->hat_srama[HAT_SCN2] != mmu_invalid) {
		section = SCN2;
		sde = (sde_t *)hatp->hat_srama[HAT_SCN2];
		lastsde = sde + hatp->hat_sramb[HAT_SCN2].SDTlen;
	} else {
		section = SCN3;
		sde = (sde_t *)hatp->hat_srama[HAT_SCN3];
		lastsde = sde + hatp->hat_sramb[HAT_SCN3].SDTlen;

		/*
		 * XXX - ublock kludge, skip over ublock.
		 */
		if (++sde > lastsde)
			return;
	}

next_scn:

	do {
		ASSERT(!SD_ISCONTIG(sde));

		if (!SD_ISVALID(sde))
			continue;

		ASSERT(sde->wd2.address != NULL);
		pt = (pte_t *)sde->wd2.address;
		lastpte = pt + NPGPT;
		pte = pt;
		do {
			if (pte->pg_pte == NULL)
				continue;

#if DEBUG && LOCK_THREAD == 1
			ASSERT(!PG_ISLOCKED(pte));
#endif

			as->a_rss--;

			pp = page_numtopp(pte->pgm.pg_pfn);
			if (pp == NULL)
				continue;
			pp->p_ref |= pte->pgm.pg_ref;
			pp->p_mod |= pte->pgm.pg_mod;

			/*
			 * Must remove pte from its
			 * p_mapping list.
			 */

			nextpte = (pte_t **)(&(pp->p_mapping));
			while (*nextpte != pte) {
				ASSERT(*nextpte != (pte_t *)NULL);
				nextpte =
				  (pte_t **)(*nextpte + NPGPT);
			}
			*nextpte = *((pte_t **)(pte + NPGPT));

			/*
			 * Don't have to perform the regular
			 * page table accounting here (i.e.
			 * next_pte ptrs, NULL out the pte,
			 * decrement the pte_in_use count, ...)
			 * because we are going to hat_ptfree()
			 * the whole page table.  hat_ptalloc()
			 * will initialize everything before it
			 * is reallocated.
			 */

		} while (++pte < lastpte);

		/*
		 * Free the page tbl and the (p_mapping list)
		 * next_pte ptrs.
		 */
		hat_ptfree(pt);

	} while (++sde <= lastsde);

	/* "grow" the segment tbls to 0 length. */
	hat_growsdt(hatp, section, 0);

	if (++section == SCN3) {
		sde = (sde_t *)hatp->hat_srama[HAT_SCN3];
		lastsde = sde + hatp->hat_sramb[HAT_SCN3].SDTlen;

		/*
		 * XXX - ublock kludge, skip over ublock.
		 */
		if (++sde <= lastsde)
			goto next_scn;
	}
}

/*
 * Set up any translation structures, for the specified address space,
 * that are needed or preferred when the process is being swapped in.
 *
 * For the 3b2, since we don't free the segment tables on swap out, we
 * don't do anything here. The page tables will be allocated by hat_memload.
 */
/* ARGSUSED */
void
hat_swapin(as)
	struct as *as;
{
}

/*
 * Free all of the translation resources, for the specified address space,
 * that can be freed while the process is swapped out. Called from as_swapout.
 *
 * For the 3b2, all of the page tables are freed. We prefer to leave the
 * segment tables intact because they have to be physically contiguous
 * and it may be extremely difficult to regain the contiguous space on swapin.
 * We simply see if the segment tables can be shrunk.
 */
void
hat_swapout(as)
	register struct as *as;
{
	hat_t	*hatp = &as->a_hat;
	ptdat_t	*ptd;
	ptdat_t	ptdat_tmp;
	page_t	*pp;
	int	section;

	ASSERT(as != u.u_procp->p_as);

	/*
	 * We split this routine into two seperate blocks to get better
	 * register allocation for heavily used variables.
	 */

	/*
	 * Check to see if we can shrink the segment tables in this
	 * block of code.
	 */

	{

	register struct seg *seg;
	register struct seg *sseg;
	register struct seg *pseg;
	register u_int maxseg;
	register u_int section;
	register u_int i;

	sseg = seg = as->a_segs;

	ASSERT(sseg != NULL);

	i = SCN2;

	do {
		section = SECNUM(seg->s_base);
		ASSERT(section == SCN2 || section == SCN3);

		if (section != i) {
			hat_growsdt(hatp, i, 0);
			continue;
		}

		do {
			pseg = seg;
			seg = seg->s_next;
		} while ((section == SECNUM(seg->s_base)) && (seg != sseg));

		maxseg = SEGNUM(pseg->s_base + pseg->s_size - 1);

		if (hatp->hat_sramb[section - SCN2].SDTlen  > maxseg) {
			hat_growsdt(hatp, section, maxseg+1);
		}

	} while (++i == SCN3);

	}	/* end of segment table block */

	/*
	 * Free up page tables in this block of code.
	 */

	{

	register sde_t *sde, *lastsde;
	register pte_t *pte, *lastpte;
	register pte_t *pt, **nextpte;

	if (hatp->hat_srama[HAT_SCN2] != mmu_invalid) {
		section = SCN2;
		sde = (sde_t *)hatp->hat_srama[HAT_SCN2];
		lastsde = sde + hatp->hat_sramb[HAT_SCN2].SDTlen;
	} else {
		section = SCN3;
		sde = (sde_t *)hatp->hat_srama[HAT_SCN3];
		lastsde = sde + hatp->hat_sramb[HAT_SCN3].SDTlen;

		/*
		 * XXX - ublock kludge, skip over ublock.
		 */
		 
		if (++sde > lastsde)
			return;
	}

next_scn:

	do {
		ASSERT(!SD_ISCONTIG(sde));

		if (!SD_ISVALID(sde))
			continue;

		ASSERT(sde->wd2.address != NULL);
		pt = (pte_t *)sde->wd2.address;
		ptd = hat_pt2ptdat(pt, &ptdat_tmp);
		if (ptd->pt_keepcnt != 0)
			continue;

		lastpte = pt + NPGPT;
		pte = pt;
		do {
			if (pte->pg_pte == NULL)
				continue;

#if DEBUG && LOCK_THREAD == 1
			ASSERT(!PG_ISLOCKED(pte) );
#endif

			as->a_rss--;

			pp = page_numtopp(pte->pgm.pg_pfn);
			if (pp == NULL)
				continue;
			pp->p_ref |= pte->pgm.pg_ref;
			pp->p_mod |= pte->pgm.pg_mod;

			/*
			 * Must remove pte from its
			 * p_mapping list.
			 */
			nextpte = (pte_t **)(&(pp->p_mapping));
			while (*nextpte != pte) {
				ASSERT(*nextpte != (pte_t *)NULL);
				nextpte =
				  (pte_t **)(*nextpte + NPGPT);
			}
			*nextpte = *((pte_t **)(pte + NPGPT));

			/*
			 * Don't have to perform the regular
			 * page table accounting here (i.e.
			 * next_pte ptrs, NULL out the pte,
			 * decrement the pte_in_use count, ...)
			 * because we are going to hat_ptfree()
			 * the whole page table.  hat_ptalloc()
			 * will initialize everything before it
			 * is reallocated.
			 */

		} while (++pte < lastpte);

		/*
		 * Free the page tbl and the (p_mapping list)
		 * nextpte ptrs.
		 */
		hat_ptfree(pt);
		SD_CLRVALID(sde);

	} while (++sde <= lastsde);

	if (++section == SCN3) {
		sde = (sde_t *)hatp->hat_srama[HAT_SCN3];
		lastsde = sde + hatp->hat_sramb[HAT_SCN3].SDTlen;

		/*
		 * XXX - ublock kludge, skip over ublock.
		 */
		if (++sde <= lastsde)
			goto next_scn;
	}

	} /* end of page table block */

}

/*
 * Unload all of the hardware translations that map page pp.
 */
void
hat_pageunload(pp)
	register page_t *pp;
{
	register pte_t *pte;
	register struct as *as;
	register ptdat_t *ptdat;
	ptdat_t	ptdat_tmp;
	struct	as *cur_as = u.u_procp->p_as;

	ASSERT(pp != NULL);

	pte = (pte_t *)(pp->p_mapping);

	while (pte != NULL) {

		/*
		 * Sync ref and mod bits back to page structure
		 * before we invalidate the pte.
		 */
		pp->p_ref |= pte->pgm.pg_ref;
		pp->p_mod |= pte->pgm.pg_mod;

		ptdat = hat_pt2ptdat(pte, &ptdat_tmp);
		as = ptdat->pt_as;
		as->a_rss--;

		/*
		 * If the pte belongs to the current process or the kernel,
		 * we must flush the virtual address from the mmu cache.
		 */

		if (as == cur_as || as == &kas) {
			u_int page_num = ((u_int)pte - ((u_int)pte & ~255)) >> 2;
			HAT_LOG((hat_debug & 2),
				"hat_pageunload: flushing addr = ",
				((ptdat->pt_secseg << 16) |
				 (page_num << PNUMSHFT)),
				as, 0);

			flushaddr( ((ptdat->pt_secseg << 16) |
			            (page_num << PNUMSHFT)) );
		}

#if DEBUG && LOCK_THREAD == 1
		ASSERT(!PG_ISLOCKED(pte));
#endif

		/*
		 * If the number of active pte's in the page table drops
		 * to zero, we can free the page table. Otherwise, just
		 * invalidate the pte.
		 *
		 * Note, we don't have to NULL out the next_pte pointer
		 * of the p_mapping list, because we NULL'd the head ptr
		 * to the list. Hence, there's currently no way to get
		 * to this pte; it's not on any list. The next time we put
		 * the pte on a list, we'll automatically set the next_pte
		 * ptr.
		 */

		ASSERT(ptdat->pt_inuse > 0);

		if (--ptdat->pt_inuse == 0) {
			int section;
			sde_t *sde;
			pte_t *nextpte;

			section = ptdat->pt_secseg >> 14;

			/* Must be a user address */
			ASSERT(ptdat->pt_keepcnt == 0);
			ASSERT(as != &kas);
			ASSERT(section == SCN2 || section == SCN3);

			/*
			 * Invalidate the sde pointing to this page tbl.
			 * It's OK to use the physical address because
			 * phys/virt are 1-1 .
			 */
			sde = (sde_t *)(as->a_hat.hat_srama[section - SCN2]) +
			  ((ptdat->pt_secseg >> 1) & SEGNMASK);

			ASSERT(sde->wd2.address == ((u_int)pte & (~255)));

			SD_CLRVALID(sde);

			nextpte = *((pte_t **)(pte + NPGPT));
			hat_ptfree((u_int)pte & (~255));
			pte = nextpte;
		} else {
			pte->pg_pte = 0;
			pte = *((pte_t **)(pte + NPGPT));
		}
	}

	/*
	 * Do this here to make debugging this routine easier.
	 */
	pp->p_mapping = NULL;
}

/*
 * Unload all of the mappings in the range [addr,addr+len) .
 */
void
hat_unload(seg, addr, len, flags)
	struct seg *seg;
	register addr_t addr;
	u_int len;
	u_int flags;
{
	register addr_t	endaddr;
	register pte_t	*pte;
	register pte_t	**nextpte;
	register page_t	*pp;
	register u_int	npgs;
	struct	as *as = seg->s_as;
	hat_t	*hatp = &seg->s_as->a_hat;
	sde_t	*sde;
	sde_t	*lastsde;
	pte_t	*pt;
	ptdat_t *ptdat;
	ptdat_t	ptdat_tmp;
	int	section;
	u_int	free_page = flags & HAT_FREEPP;
	u_int	rele_page = flags & HAT_RELEPP;

	HAT_LOG(((hat_debug & 8) && ((seg == segu) || (seg == segkmap))),
		"hat_unload", addr, len, RET_ADDR(len));

	endaddr = addr + len - 1;
	section = SECNUM(addr);

	/* addr is page aligned */
	ASSERT(((u_long)addr & POFFMASK) == 0);

	/* doesn't cross section boundary */
	ASSERT(section == SECNUM(endaddr));

	/* valid section */
	ASSERT(section != 0);

	if (section == SCN1) {
		sde = (sde_t *)srama[SCN1];

		ASSERT(seg == segkmap || seg == segu);
		ASSERT(sramb[SCN1].SDTlen >= SEGNUM(endaddr));

	} else { /* SCN2 or SCN3 */

		sde = (sde_t *)hatp->hat_srama[section - SCN2];

		/*
		 * This test is needed because relvm() calls hat_free()
		 * and then the segment driver free routines. The segment
		 * driver free routines call hat_unload() on the segments
		 * address range, but hat_free() already destroyed all of
		 * the mappings. This is the only case where the SDT's
		 * don't exist when this routine gets called. All other
		 * times the segment has been hat_map()'d, hence the SDT
		 * for it exists.
		 */
		if (sde == (sde_t *)mmu_invalid || sde == (sde_t *)dflt_sdt_p)
			return;

		ASSERT(hatp->hat_sramb[section-SCN2].SDTlen >= SEGNUM(endaddr));
	}

	lastsde = sde + SEGNUM(endaddr);
	sde += SEGNUM(addr);

	do {
		ASSERT(!SD_ISCONTIG(sde));

		if (!SD_ISVALID(sde)) {
			addr = (addr_t)(((u_int)addr + NBPS) & (~(NBPS - 1)));
			continue;
		}

		ASSERT(sde->wd2.address != NULL);

		pt = (pte_t *)(sde->wd2.address);
		pte = pt + PAGNUM(addr);
		ptdat = hat_pt2ptdat(pt, &ptdat_tmp);
		npgs = 0;
		do {
			if (pte->pg_pte == 0) {
				++pte;
				addr += NBPP;
				continue;
			}

			CHECK_PTE(seg, pte);

#if DEBUG && LOCK_THREAD == 1
			if (flags & HAT_UNLOCK) {
				ASSERT(PG_ISLOCKED(pte));
				PG_CLRLOCK(pte);
			} else {
				ASSERT(!PG_ISLOCKED(pte));
			}
#endif

			++npgs;
			flushaddr(addr);

			pp = page_numtopp(pte->pgm.pg_pfn);
			if (pp != NULL) {
				pp->p_ref |= pte->pgm.pg_ref;
				pp->p_mod |= pte->pgm.pg_mod;

				/* Must remove from p_mapping list.
				 */
				nextpte = (pte_t **)(&(pp->p_mapping));
				while (*nextpte != pte) {
					ASSERT(*nextpte != NULL);
					nextpte = (pte_t **)(*nextpte + NPGPT);
				}
				*nextpte = *((pte_t **)(pte + NPGPT));

				if (rele_page) {
					PAGE_RELE(pp);
				}

				/* Avoid orphaned pages; code taken from
				 * checkpage() in os/vm_pageout.c.
				 */
				if (free_page &&
				  (pp->p_keepcnt == 0) && /* not kept */
				  (pp->p_mapping == 0) && /* no mappings */
				  (pp->p_lock == 0) &&    /* nothing funny */
				  (pp->p_free == 0) &&
				  (pp->p_intrans == 0) &&
				  (pp->p_lckcnt == 0) &&
				  (pp->p_cowcnt == 0) ) {
					if (pp->p_mod && pp->p_vnode) {
						(void) VOP_PUTPAGE(pp->p_vnode,
							pp->p_offset,
							PAGESIZE,
							B_ASYNC | B_FREE,
							(struct cred *) 0);
					} else {
						page_lock(pp);
						page_free(pp, 0);
					}
				}
			}

			pte->pg_pte = 0;
			++pte;
			addr += NBPP;

		} while (((u_long)addr & (PNUMMASK << PNUMSHFT))
			&& addr < endaddr);

		ASSERT(as->a_rss >= npgs);
		as->a_rss -= npgs;

		if (flags & HAT_UNLOCK) {
			ASSERT(ptdat->pt_keepcnt >= npgs);
			ptdat->pt_keepcnt -= npgs;
		}

		ASSERT(ptdat->pt_inuse >= npgs);
		ptdat->pt_inuse -= npgs;

		if (ptdat->pt_inuse == 0) {
			hat_ptfree(pt);
			SD_CLRVALID(sde);
		}

	} while (++sde <= lastsde);
}

/*
 * Change the protections for the virtual address range [addr,addr+len)
 * to the protection prot.
 *
 * For the 3b2:
 *
 * (~)PROT_USER will be handled at the segment table level,
 * since the user/kernel access bits are in the segment descriptor entry.
 * This is not a problem because user/kernel pages don't reside in the
 * same segments.
 *
 * The only catcheable access violation at the page level is write access.
 * Therefore, the only pte bit that needs to be set in this routine, except
 * in the PROT_NONE case, is the fault-on-write bit.
 * 
 * PROT_NONE will be handled by using the valid bit in the page table
 * entry. If a user does not have access to a page, the valid bit will
 * be cleared.
 */
void
hat_chgprot(seg, addr, len, prot)
	struct seg *seg;
	register addr_t addr;
	u_int len;
	u_int prot;
{
	register addr_t endaddr;
	register pte_t	*pte;
	register sde_t	*sde;
	register sde_t	*lastsde;
	struct	hat *hatp = &seg->s_as->a_hat;
	int	section;

	endaddr = addr + len - 1;
	section = SECNUM(addr);

	/* addr is page aligned */
	ASSERT(((u_long)addr & POFFMASK) == 0);

	/* doesn't cross section boundary */
	ASSERT(section == SECNUM(endaddr));

	/* valid section */
	ASSERT(section != 0);

	if (section == SCN1) {
		sde = (sde_t *)srama[SCN1];

		ASSERT(seg == segkmap || seg == segu);
		ASSERT(sramb[SCN1].SDTlen >= SEGNUM(endaddr));

	} else { /* SCN2 or SCN3 */

		sde = (sde_t *)hatp->hat_srama[section - SCN2];

		ASSERT(sde != (sde_t *)mmu_invalid);
		ASSERT(sde != (sde_t *)dflt_sdt_p);
		ASSERT(hatp->hat_sramb[section-SCN2].SDTlen >= SEGNUM(endaddr));
	}

	lastsde = sde + SEGNUM(endaddr);
	sde += SEGNUM(addr);

	/*
	 * (~)PROT_USER handled at the segment descriptor table level,
	 */
	prot &= (PROT_READ | PROT_WRITE | PROT_EXEC);

	if (prot & PROT_WRITE) {
		do {
			ASSERT(!SD_ISCONTIG(sde));

			if (!SD_ISVALID(sde)) {
				addr = (addr_t)
				  (((u_int)addr + NBPS) & (~(NBPS - 1)));
				continue;
			}

			ASSERT(sde->wd2.address != NULL);
			pte = (pte_t *)sde->wd2.address + PAGNUM(addr);
			do {
				/*
				 * No need to check for valid pte
				 * here because we're just CLEARING
				 * the fault on write bit.
				 */
				PG_CLRW(pte);
				++pte;
				flushaddr(addr);
				addr += NBPP;
			} while (((u_long)addr & (PNUMMASK << PNUMSHFT))
				&& addr < endaddr);
		} while (++sde <= lastsde);
	} else if (prot == PROT_NONE) {
		do {
			ASSERT(!SD_ISCONTIG(sde));

			if (!SD_ISVALID(sde)) {
				addr = (addr_t)
				  (((u_int)addr + NBPS) & (~(NBPS - 1)));
				continue;
			}

			ASSERT(sde->wd2.address != NULL);
			pte = (pte_t *)sde->wd2.address + PAGNUM(addr);
			do {
				/*
				 * No need to check for valid pte
				 * here because we're just CLEARING
				 * the valid bit.
				 */
				PG_CLRVALID(pte);
				++pte;
				flushaddr(addr);
				addr += NBPP;
			} while (((u_long)addr & (PNUMMASK << PNUMSHFT))
				&& addr < endaddr);
		} while (++sde <= lastsde);
	} else {
		do {
			ASSERT(!SD_ISCONTIG(sde));

			if (!SD_ISVALID(sde)) {
				addr = (addr_t)
				  (((u_int)addr + NBPS) & (~(NBPS - 1)));
				continue;
			}

			ASSERT(sde->wd2.address != NULL);
			pte = (pte_t *)sde->wd2.address + PAGNUM(addr);
			do {
				/*
				 * Don't set fault on write bit
				 * if pte is 0.  It will be set when
				 * it is hat_memload()'d.
				 */
				if (pte->pg_pte != 0)
					PG_SETW(pte);
				++pte;
				flushaddr(addr);
				addr += NBPP;
			} while (((u_long)addr & (PNUMMASK << PNUMSHFT))
					&& addr < endaddr);
		} while (++sde <= lastsde);
	}
}

/*
 * Get all the hardware dependent attributes for a page struct
 */
void
hat_pagesync(pp)
	register page_t *pp;
{
	register pte_t *pte;
	register struct as *as;
	register struct as *cur_as = u.u_procp->p_as;
	register ptdat_t *ptdat;
	ptdat_t	ptdat_tmp;
	u_int	page_num;

	pte = (pte_t *)pp->p_mapping;

	while (pte != NULL) {
		pp->p_ref |= pte->pgm.pg_ref;
		PG_CLRREF(pte);
		pp->p_mod |= pte->pgm.pg_mod;
		PG_CLRMOD(pte);

		/*
		 * If the pte belongs to the current process or the kernel,
		 * we must flush the virtual address from the mmu cache so
		 * the ref and mod bits in the pte will get updated by the
		 * mmu. Otherwise the mmu would find the bits set in the
		 * cache and think they were already set in the pte.
		 */
		ptdat = hat_pt2ptdat(pte, &ptdat_tmp);

		if ((as = ptdat->pt_as) == cur_as || as == &kas) {
			page_num = ((u_int)pte - ((u_int)pte & ~255)) >> 2;

			HAT_LOG((hat_debug & 2),
				"hat_pagesync: flushing addr = ",
				((ptdat->pt_secseg << 16) |
				 (page_num << PNUMSHFT)),
				as, 0);

			flushaddr( ((ptdat->pt_secseg << 16) |
			            (page_num << PNUMSHFT)) );
		}

		pte = *((pte_t **)(pte + NPGPT));
	}
}

/*
 * Set up addr to map to page pp with protection prot.
 */
void
hat_memload(seg, addr, pp, prot, lock)
	struct seg *seg;
	addr_t addr;
	page_t *pp;
	u_int  prot;
	u_int  lock;
{
	HAT_LOG(((hat_debug & 0x4) && lock),
		"hat_memload-l:", addr, pp, RET_ADDR(lock));
	HAT_LOG(((hat_debug & 0x8) && ((seg == segkmap) || (seg == segu))),
		"hat_memload-k:", addr, pp, RET_ADDR(lock));

	hat_pteload(seg, addr, pp, page_pptonum(pp), prot, lock);
}

/*
 * Set up addr to map to cookie pf with protection prot.
 */
void
hat_devload(seg, addr, pf, prot, lock)
	struct seg *seg;
	addr_t addr;
	u_int pf;
	u_int prot;
	int   lock;
{
	HAT_LOG(((hat_debug & 0x4) && lock),
		"hat_devload-l:", addr, pf, RET_ADDR(lock));
	HAT_LOG(((hat_debug & 0x8) && ((seg == segkmap) || (seg == segu))),
		"hat_devload-k:", addr, pf, RET_ADDR(lock));

	hat_pteload(seg, addr, NULL, pf, prot, lock);
}

/*
 * Set up addr to map pfn with protection prot. May, or may not,
 * have an associated pp structure.
 */
STATIC void
hat_pteload(seg, addr, pp, pfn, prot, lock)
	struct seg *seg;
	register addr_t addr;
	register page_t *pp;
	u_int  pfn;
	u_int  prot;
	u_int  lock;
{
	register sde_t *sde;
	register pte_t *pte;
	ptdat_t *ptdat;
	ptdat_t ptdat_tmp;
	u_int section;
	u_int addr_seg;
	u_int mode;
	SRAMA sra;
	SRAMB srb;

	section = SECNUM(addr);
	addr_seg = SEGNUM(addr);

	ASSERT((pp->p_mapping == NULL) ||
	       ((pp->p_mapping >= (caddr_t)0x2000000) &&
	        (pp->p_mapping <  (caddr_t)0x3000000)));

	/* addr is page aligned */
	ASSERT(((int)addr & POFFMASK) == 0);

	/* kernel address, but not segkmap or segu */
	ASSERT(!(section == SCN1 && seg != segkmap && seg != segu));

	/* overlap with u-block */
	ASSERT(!(section == SCN3 && addr_seg == 0));

	/* valid section */
	ASSERT(section != 0);

	if (section == SCN1) {
		sra = srama[section];
		srb = sramb[section];
	} else { /* SCN2 or SCN3 */
		sra = seg->s_as->a_hat.hat_srama[section-SCN2];
		srb = seg->s_as->a_hat.hat_sramb[section-SCN2];
	}

	if ((sde = (sde_t *)sra) == (sde_t *)mmu_invalid ||
	    addr_seg > srb.SDTlen) {
		cmn_err(CE_PANIC, "hat_pteload: SDT not allocated.");
		/* NOTREACHED */
	}

	sde += addr_seg;

	ASSERT(!SD_ISCONTIG(sde));

	if (!SD_ISVALID(sde) ) {
		ASSERT(section == SCN2 || section == SCN3);

		/*
		 * If the thing we are mapping is associated with a pp,
		 * we must bump the keepcnt to prevent it from being
		 * stolen because may sleep in hat_ptalloc().
		 */
		if (pp) {
			++pp->p_keepcnt;
			pte = hat_ptalloc(&ptdat, HAT_CANWAIT);
			--pp->p_keepcnt;
		} else {
			pte = hat_ptalloc(&ptdat, HAT_CANWAIT);
		}

		/* Shouldn't have failed. */
		ASSERT(pte != (pte_t *)NULL);

		/*
		 * Update the page table data (ptdat) to which the
		 * pte will belong.
		 */
		ptdat->pt_as = seg->s_as;
		ptdat->pt_secseg = ((u_int)addr >> SEGNSHFT) << 1;
		ptdat->pt_inuse = 1;
		ptdat->pt_keepcnt = 0;

		sde->wd2.address = (paddr_t)pte;
		sde->seg_prot = (prot & PROT_USER) ? KRWE|URWE : KRWE ;
		sde->seg_flags = SDE_flags;
		sde->seg_len = (stob(1)>>3) - 1;

		pte += PAGNUM(addr);
		ASSERT(pte->pg_pte == 0);
	} else {
		ASSERT(sde->wd2.address != NULL);

		pte = (pte_t *)sde->wd2.address + PAGNUM(addr);

		CHECK_PTE(seg, pte);

		if (pte->pg_pte != 0) {
			if ((u_int)pte->pgm.pg_pfn != pfn) {
				cmn_err(CE_PANIC, "hat_pteload - trying to change existing mapping.");
				/* NOTREACHED */
			}

			/*
			 * (~)PROT_USER is handled at the SDT level.
			 */
			prot &= (PROT_READ | PROT_WRITE | PROT_EXEC);

			if (prot & PROT_WRITE)
				mode = PG_V;
			else if (prot == PROT_NONE)
				mode = 0;
			else
				mode = PG_V | PG_W;
			/*
			 * Sync up sofware copy of ref & mod bits
			 * before we clear them in the pte.  We're
			 * also going to flush the mmu cache because
			 * the mmu wouldn't bother resetting them, in
			 * the pte, if it thinks they're already set.
			 * We also have to flush because we may have
			 * changed the permissions of the translation.
			 */
			if (pp) {
				pp->p_ref |= pte->pgm.pg_ref;
				pp->p_mod |= pte->pgm.pg_mod;
			}

			if (lock & HAT_LOCK) {
				ptdat = hat_pt2ptdat(pte, &ptdat_tmp);
				++ptdat->pt_keepcnt;
#if DEBUG && LOCK_THREAD == 1
				mode |= PG_LOCK;
			} else {
				/*
				 * If we're remapping an already existing
				 * locked mapping, we must preserve the
				 * lock bit in the pte; i.e. even though
				 * lock was not specified, this call does
				 * not unlock the mapping. Only hat_unlock
				 * or hat_unload can unlock a mapping.
				 */
				if (PG_ISLOCKED(pte))
					mode |= PG_LOCK;
			}
#else
			}
#endif
			pte->pg_pte = (u_int)mkpte(mode, pfn);
			flushaddr(addr);
			return; /* success */
		}
		ptdat = hat_pt2ptdat(pte, &ptdat_tmp);
		/* XXX - ASSERT(ptdat->pt_inuse < NPGPT); */
		ptdat->pt_inuse++;
	}

	/*
	 * (~)PROT_USER is handled at the SDT level.
	 */
	prot &= (PROT_READ | PROT_WRITE | PROT_EXEC);

	if (prot & PROT_WRITE)
		mode = PG_V;
	else if (prot == PROT_NONE)
		mode = 0;
	else
		mode = PG_V | PG_W;

	if (lock & HAT_LOCK) {
		++ptdat->pt_keepcnt;
#if DEBUG && LOCK_THREAD == 1
		mode |= PG_LOCK;
#endif
	}

	pte->pg_pte = (u_int)mkpte(mode, pfn);

	/* add to p_mapping list */

	if (pp) {
		*((pte_t **)(pte + NPGPT)) = (pte_t *)pp->p_mapping;
		pp->p_mapping = (caddr_t)pte;
	}

	seg->s_as->a_rss++;
}

int
hat_dup(as, new_as)
	struct as *as;
	struct as *new_as;
{
	register addr_t addr;
	register addr_t endaddr;
	u_int	addr_seg;
	u_int	addr_pag;
	u_int	hat_scn;
	u_int	sra;
	u_int	error;
	sde_t	*last_sde;
	sde_t	*sde;
	sde_t	*new_sde;
	pte_t	*pte;
	pte_t	*new_pte;
	ptdat_t *ptdat;
	ptdat_t *ptdat_tmp;
	u_int	copyflag;
	page_t	*pp;
	page_t	*opp;
	struct	seg *sseg;
	struct	seg *seg;		/* parent seg ptr. */
	struct	seg *new_seg;		/* child  seg ptr. */
	struct	segvn_data *svd;	/* parent seg private data */
	struct	segvn_data *new_svd;	/* child  seg private data */
	struct	anon_map *amp;		/* parent anon map ptr. */
	struct	anon_map *new_amp;	/* child  anon map ptr. */
	struct	anon **anon;		/* parent anon entry ptr. */
	struct	anon **new_anon;	/* child  anon entry ptr. */
	u_int	index;
	struct	vnode *svp;
	u_int	soff;

	/*
	 * Grow the child's SDT to the length of the parent's.
	 */
	hat_scn = HAT_SCN2;
	do {
		ASSERT(new_as->a_hat.hat_srama[hat_scn] == mmu_invalid ||
		       new_as->a_hat.hat_srama[hat_scn] == dflt_sdt_p);
		ASSERT(new_as->a_hat.hat_sramb[hat_scn].SDTlen == 0);

		sra = as->a_hat.hat_srama[hat_scn];

		if (sra == dflt_sdt_p || sra == mmu_invalid)
			continue;

		error = hat_growsdt(&new_as->a_hat, hat_scn+SCN2,
			as->a_hat.hat_sramb[hat_scn].SDTlen + 1);

		if (error != 0)
			return (error);

	} while (++hat_scn == HAT_SCN3);

#ifdef DEBUG
	if (hat_duppts == 0)
		return(0);
#endif

	/* 
	 * Optimize behavior for seg_vn type segments.
	 *
	 * Initialize the page tables of the new address space <new_as>
	 * by copying the valid seg_vn mappings from the original address
	 * space <as>.
	 *
	 * As a heuristic, if a MAP_PRIVATE page has already been
	 * copy-on-write'd we will give a private copy of the page
	 * to the child. In general, we believe the child will write
	 * the same pages that the parent has already written. Our
	 * performance studies show that this RADICALLY reduces the
	 * number of c-o-w faults caused by the shell.
	 *
	 * NOTES:
	 *
	 *	We do not inform any file system that might manage a
	 *	particular page that a new reference has been made to
	 *	it.
	 *
	 *	We're only guessing that the process will need the
	 *	copied mappings. We don't bother sleeping for any
	 *	mapping resources.
	 *
	 *	In the future, we might try some other heuristics
	 *	to avoid down on unnecessary copying (for example,
	 *	instead of copying all stack pages, only copy the
	 *	current one which we find based on the current
	 *	(parent) stack pointer).
	 */

	sseg = seg = as->a_segs;
	new_seg = new_as->a_segs;

	ASSERT(seg != NULL && new_seg != NULL);

	do {
		/*
		 * Make sure the seg lists are 1-1.
		 */
		ASSERT(	(seg->s_ops  == new_seg->s_ops)  &&
			(seg->s_base == new_seg->s_base) &&
			(seg->s_size == new_seg->s_size) );

		/* 
		 * Only copy the mappings if this is a vnode-type segment.
		 * For now, in order to tell whether a segment is a
		 * vnode-type segment, we check what functions are used
		 * to manipulate it.  Ultimately, we would like a type field 
		 * to let us know what kind of segment it is.  Or we can
		 * take the object-oriented approach and add a function
		 * into the seg_ops that will do this duplication for us.
		 * There may eventually be other types of segments that
		 * we can 'duplicate' on fork.
		 */
		if (seg->s_ops != &segvn_ops) {
			new_seg = new_seg->s_next;
			continue;
		}

		svd = (struct segvn_data *)(seg->s_data);

		/*
		 * If the original segment has per page protections
		 * (a vpage array), play safe and do not copy the mappings.
		 */
		if (svd->pageprot) {
			new_seg = new_seg->s_next;
			continue;
		}

		/*
		 * If the original segment is MAP_PRIVATE, has write
		 * permission and has the anon map set up, then we will
		 * anticipate the c-o-w faults and make a copy of the
		 * pages.
		 */
		amp = svd->amp;
		if (amp) {
			/*
			 * The child's anon structures should have been set
			 * up in segvn_dup().
			 */
			new_svd = (struct segvn_data *)(new_seg->s_data);
			ASSERT(new_svd != NULL);
			new_amp = new_svd->amp;
			ASSERT(new_amp != NULL);
			anon = amp->anon;
			ASSERT(anon != NULL);
			new_anon = new_amp->anon;
			ASSERT(new_anon != NULL);

			if ((svd->type == MAP_PRIVATE) &&
			    (svd->prot & PROT_WRITE))  {
				copyflag = 1;
			} else {
				copyflag = 0;
			}
		} else {
			copyflag = 0;
		}

		/*
		 * Calculate parameters and find data structures of the
		 * mappings in the original segment.
		 */
		addr = seg->s_base;
		hat_scn = SECNUM(addr) - SCN2;
		addr_seg = SEGNUM(addr);
		endaddr = addr + seg->s_size - 1;

		/* page aligned */
		ASSERT(((u_long)addr & POFFMASK) == 0);

		/* user mapping */
		ASSERT(hat_scn == HAT_SCN2 || hat_scn == HAT_SCN3);

		/* u-area overlap */
		ASSERT(!(hat_scn == HAT_SCN3 && addr_seg == 0));

		/* cross section boundary */
		ASSERT(hat_scn == (SECNUM(endaddr) - SCN2));

		sde = (sde_t *)as->a_hat.hat_srama[hat_scn];
		last_sde = sde + SEGNUM(endaddr);
		sde += addr_seg;
		new_sde = (sde_t *)new_as->a_hat.hat_srama[hat_scn] + addr_seg;

		/*
		 * For each sde associated with the original segment,
		 * if it has valid page tables present, copy the entries
		 * to the sde for the new segment.  We may have to allocate
		 * page tables for the new segment if they are not present.
		 */

		do {
			ASSERT(!SD_ISCONTIG(sde));

			if (!SD_ISVALID(sde)) {
				addr = (addr_t)
				  (((u_int)addr + NBPS) & (~(NBPS - 1)));
				if (copyflag) {
					index = (addr - seg->s_base) >> PNUMSHFT;
					anon = amp->anon + index;
					new_anon = new_amp->anon + index;
				}
				++new_sde;
				continue;
			}

			ASSERT(sde->wd2.address != NULL);

			addr_pag = PAGNUM(addr);
			pte = (pte_t *)sde->wd2.address + addr_pag;

			/*
			 * If sde for new segment does not have a page table,
			 * allocate one.  New_pte will point to the base of
			 * the page table.
			 */
			if (!SD_ISVALID(new_sde)) {
				new_pte = hat_ptalloc(&ptdat, HAT_NOSLEEP|HAT_NOSTEAL);
				if (new_pte == NULL)
					return(0);

				ptdat->pt_as = new_as;
				ptdat->pt_secseg = ((u_int)addr >> SEGNSHFT)<<1;
				ptdat->pt_inuse = 0;
				ptdat->pt_keepcnt = 0;

				*new_sde = *sde; /* will mark it valid */
				new_sde->wd2.address = (paddr_t) new_pte;
			} else {
				new_pte = (pte_t *)new_sde->wd2.address;
				ptdat = hat_pt2ptdat(new_pte, &ptdat_tmp);
			}

			new_pte += addr_pag;

			/*
			 * Walk through page table, copying valid entries.
			 */
			do {
				ASSERT(new_pte->pg_pte == 0);

				if (PG_ISVALID(pte)) {
					if (copyflag && *anon) {
						/*
						 * Allocate the new page,
						 * a new anon structure,
						 * copy the page contents, 
						 * and tie everything together.
						 *
						 * XXX - I should be calling
						 * rm_allocpage() here, but
						 * for now I want to save the
						 * extra function call.
						 */
						pp = page_get(PAGESIZE, P_NOSLEEP);
						if (pp == NULL)
							return(0);

						*new_anon = anon_alloc();
						swap_xlate(*new_anon, &svp, &soff);
						page_enter(pp, svp, soff);
						(*new_anon)->un.an_page = pp;
						pp->p_intrans = 1;
						pp->p_pagein = 1;
						opp =
						  page_numtopp(pte->pgm.pg_pfn);
						ppcopy(opp, pp);
						pp->p_mod = 1;
						pp->p_intrans = 0;
						pp->p_pagein = 0;
						/*
						 * The child must have write
						 * permission to get here. So,
						 * the valid bit should be on
						 * and the f-o-w bit should be
						 * off.
						 */
						new_pte->pg_pte =
						  (u_int)mkpte(PG_V, page_pptonum(pp));

						/*
						 * Add to p_mapping list.
						 */
						*((pte_t **)(new_pte + NPGPT)) =
						  (pte_t *)pp->p_mapping;
						pp->p_mapping =
						  (caddr_t)new_pte;

						page_unlock(pp);
						PAGE_RELE(pp);

						/* 
						 * If the parent refcount is 1,
						 * cancel the copy on write.
						 *
						 * Note: We don't have to flush
						 * the ATB because segvn_dup
						 * called hat_chgprot, to set
						 * the copy-on-write, and that
						 * flushes the ATB.
						 */
						if (((*anon)->an_refcnt--) == 1)
							PG_CLRW(pte);
					} else {
						/*
						 * The fault-on-write bit
						 * has already been set in
						 * the parent's pte, if it's
						 * needed, by segvn_dup().
						 */
						new_pte->pg_pte = pte->pg_pte;

						/*
						 * Add to p_mapping list.
						 */
						*((pte_t **)(new_pte + NPGPT)) =
						  *((pte_t **)(pte + NPGPT));
						*((pte_t **)(pte + NPGPT)) =
						  new_pte;
					}
#if DEBUG && LOCK_THREAD == 1
					/*
					 * Child should not inherit lock bit.
					 */
					PG_CLRLOCK(new_pte);
#endif
					++new_as->a_rss;
					++ptdat->pt_inuse;
				}

				++pte;
				++new_pte;
				++anon;
				++new_anon;
				addr += NBPP;

			} while (((u_long)addr & (PNUMMASK << PNUMSHFT))
				&& addr < endaddr);

			++new_sde;

		} while (++sde <= last_sde);

		new_seg = new_seg->s_next;

	} while ((seg = seg->s_next) != sseg);

	return (0);
}

/*
 * hat_map(seg, ppl, base, prot, flags)
 *
 * Allocate any hat resources needed for a new segment.
 *
 * This routine is invoked by the seg_create() routines in the segment
 * drivers.
 *
 * On the 3b2, we grow the SDT to the appropriate size. Also, for vnode
 * type segments, we load up translations to any pages of the segment
 * that are already in core.
 */
u_int
hat_map(seg, ppl, file_base, prot, flags)
	register struct seg *seg;
	page_t *ppl;
	off_t	file_base;
	u_int	prot;
	u_int	flags;
{
	register addr_t	addr;
	register page_t	*pp;
	register pte_t	*pte;
	register long	file_off;	/* offset of page in file */
	hat_t	*hatp;
	struct	as *as;
	addr_t	endaddr;
	SRAMA	sra;
	int	newnseg;
	int	curnseg;	/* segment id of current last HW seg */
	int	section;
	sde_t	*sde;
	long	file_end;	/* end of VM segment in file */
	long	addr_off;	/* loop invariant; see comments below. */
	ptdat_t	*ptdat;
	ptdat_t	ptdat_tmp;
	int	mode;
	int	error;

	addr = seg->s_base;
	endaddr = addr + seg->s_size - 1;
	newnseg = SEGNUM(endaddr);
	section = SECNUM(addr);

	ASSERT(((u_long)addr & POFFMASK) == 0);	/* page aligned */
	ASSERT(section == SECNUM(endaddr));	/* cross section boundary */

	if (section == SCN0 || section == SCN1) { /* valid for hat_map */
		/*
		 * curnseg = sramb[section].SDTlen;
		 *
		 * XXX - need some validity checks here, but everything
		 *       I think should be true isn't.
		 *
		 * ASSERT(newnseg <= curnseg);
		 */

		return (0);
	}

	as = seg->s_as;
	hatp = &(as->a_hat);
	sra = hatp->hat_srama[section - SCN2];
	if (sra == mmu_invalid || sra == dflt_sdt_p)
		curnseg = -1;
	else
		curnseg = hatp->hat_sramb[section - SCN2].SDTlen;

	if (newnseg > curnseg) {
		/*
		 * hat_growsdt() is called with newnseg+1 because it takes
		 * the number of SDE entries needed for the section. We
		 * have to add 1 since segment id numbering starts at 0.
		 */
		if ((error = hat_growsdt(hatp, section, newnseg+1)) != 0) {
			return (error);
		}
		ASSERT(as == u.u_procp->p_as);
		loadmmu(hatp, section);
		sra = hatp->hat_srama[section - SCN2];
	}

#ifdef DEBUG
	if (hat_mappts == 0)
		return(0);
#endif

	if ((flags & HAT_PRELOAD) == 0)
		return(0);

	/*
	 * XXX - Some day we may pre-load other segment types.
	 */
	ASSERT((flags & HAT_VNLIST) != 0);

	/*
	 * Set up as many of the hardware address translations as we can.
	 * If the segment is a vnode-type segment and it has a non-NULL vnode, 
	 * we walk down the list of incore pages associated with the vnode.
	 * For each page on that list, if the page maps into the range managed
	 * by the segment we calculate the corresponding virtual address and
	 * set up the hardware translation tables to point to the page.
	 *
	 * Note: the file system that owns the vnode is not informed about the
	 * new references we have made to the page.
	 */
	
	/*
	 * Walk down the list of pages associated with the
	 * vnode, setting up the translation tables if the 
	 * page maps into addresses in this segment.
	 */
	if ((pp = ppl) == NULL)	/* page list is empty */
		return(0);

	/*
	 * Calculate the protections we need for the pages
	 * (i.e. whether to set fault on write bit or not).
	 */
	if (prot & PROT_WRITE)
		mode = PG_V;
	else if (prot == PROT_NONE)
		return(0);		/* don't bother */
	else
		mode = PG_V | PG_W;

	/*
	 * Remove invariant from the loop. To compute the virtual
	 * address of the page, we use the following computation:
	 *
	 *	addr = seg->s_base + (file_off - file_base)
	 *
	 * Since seg->s_base and file_base do not vary for this segment:
	 *
	 *	addr = file_off + (addr_off = (seg->s_base - file_base))
	 *
	 * Note: currently addr = seg->s_base;
	 */
	addr_off = (u_int)addr - file_base;
	file_end = file_base + seg->s_size;

	do {
		/* 
		 * If page is intrans, ignore it.  (There may be 
		 * races if we try to use it.)
		 */
		if (pp->p_intrans) {
			continue;
		}

		/*
		 * See if the file offset is within the range mapped
		 * by the segment.
		 */
		file_off = pp->p_offset;
		ASSERT((file_off & POFFMASK) == 0);
		if ((file_off < (u_int)file_base) || (file_off >= file_end)) {
			continue;
		}

		/* Reclaim the page because we're committed to loading up a
		 * translation to it.
		 */
		if (pp->p_free) {
			page_reclaim(pp);
		}

		addr = (addr_t) (addr_off + file_off);
		ASSERT(((u_int)addr & POFFMASK) == 0);
		ASSERT(section == SECNUM(addr));

		/*
		 * Get the segment descriptor entry for the page.
		 * If it is not yet valid (page tables not present),
		 * allocate and set up the page tables.  Then get 
		 * the page table entry for the page.
		 */

		sde = (sde_t *)sra + SEGNUM(addr);

		ASSERT(SEGNUM(addr) <= (hatp->hat_sramb[section-SCN2]).SDTlen);

		if (!SD_ISVALID(sde)) {
			/*
			 * We're only guessing that the process will need
			 * these mappings. Don't bother sleeping for a PDT
			 * or stealing one from another process.
			 */
			pte = hat_ptalloc(&ptdat, HAT_NOSLEEP|HAT_NOSTEAL);
			if (pte == NULL)
				return(0);

			ptdat->pt_as = as;
			ptdat->pt_secseg = ((u_int)addr >> SEGNSHFT) << 1;
			ptdat->pt_inuse = 0;
			ptdat->pt_keepcnt = 0;

			sde->wd2.address = kvtophys((caddr_t)pte);
			sde->seg_prot = KRWE|URWE;
			sde->seg_flags = SDE_flags;
			sde->seg_len = (stob(1) >> 3) - 1;

			pte += PAGNUM(addr);

		} else {	/* page table is present */

			ASSERT(sde->wd2.address != NULL);
			pte = (pte_t *)sde->wd2.address + PAGNUM(addr);
			ASSERT(pte->pg_pte == 0);
			ptdat = hat_pt2ptdat(pte, &ptdat_tmp);
		}

		pte->pg_pte = (u_int)mkpte(mode, page_pptonum(pp));
		ptdat->pt_inuse++;
		as->a_rss++;

		/*
		 * Finally, add this reference to the p_mapping list.
		 * If page is on the freelist and we mapped it into
		 * the process address space, take it off the freelist.
		 * Then get the next page on the vnode page list.
		 */
		*((pte_t **)(pte + NPGPT)) = (pte_t *)pp->p_mapping;
		pp->p_mapping = (caddr_t)pte;

	} while ((pp = pp->p_vpnext) != ppl);

	return (0);
}

/*
 * Unlock translation at addr.
 *
 * Under the 3b2 implementation, this just means checking that the
 * translation was locked and decrementing the page table keepcnt
 * in the ptdat structure.
 */
void
hat_unlock(seg, addr)
	struct seg *seg;
	register addr_t addr;
{
	register sde_t *sde;
	register pte_t *pte;
	register hat_t *hatp = &seg->s_as->a_hat;
	register ptdat_t *ptd;
	int section;
	ptdat_t ptdat_tmp;

	HAT_LOG((hat_debug & 4), "hat_unlock", addr, seg, 0);

	section = SECNUM(addr);

	/* addr is page aligned */
	ASSERT(((u_long)addr & POFFMASK) == 0);

	/* valid section */
	ASSERT(section != 0);

	if (section == SCN1) {
		sde = (sde_t *)srama[SCN1];

		ASSERT(seg == segkmap || seg == segu);
		ASSERT(sramb[SCN1].SDTlen >= SEGNUM(addr));

	} else { /* SCN2 or SCN3 */

		sde = (sde_t *)hatp->hat_srama[section - SCN2];

		ASSERT(sde != (sde_t *)mmu_invalid);
		ASSERT(sde != (sde_t *)dflt_sdt_p);
		ASSERT(hatp->hat_sramb[section-SCN2].SDTlen >= SEGNUM(addr));
	}

	sde += SEGNUM(addr);

	if (SD_ISVALID(sde)) {
		ASSERT(sde->wd2.address != NULL);
		pte = (pte_t *)sde->wd2.address + PAGNUM(addr);
#if DEBUG && LOCK_THREAD == 1
		ASSERT(PG_ISLOCKED(pte));
		PG_CLRLOCK(pte);
#endif
		ptd = hat_pt2ptdat(pte, &ptdat_tmp);
		ASSERT(ptd->pt_keepcnt != 0);
		if ((--ptd->pt_keepcnt) == 0 && pt_waiting != 0) {
			wakeup((caddr_t)&free_pts);
			pt_waiting = 0;
		}
	} else {
		cmn_err(CE_PANIC, "hat_unlock: invalid sde.");
		/* NOTREACHED */
	}
}

/*
 * Associate all the mappings in the range [addr..addr+len) with
 * segment seg. Since we don't cache segments in this hat implementation,
 * this routine is a noop.
 */
/* ARGSUSED */
void
hat_newseg(seg, addr, len, nseg)
	struct seg *seg;
	addr_t addr;
	u_int len;
	struct seg *nseg;
{
}

STATIC ptdat_t *
hat_pt2ptdat(pt, ptd)
	register paddr_t pt;
	register ptdat_t *ptd;
{
	register page_t *pp;
	register u_int pt_pfn;

	pt_pfn = phystopfn(pt);

	if ((pp = page_numtopp(pt_pfn)) == NULL) {
		/*
		 * Must be a kernel page table.
		 */
		ptd->pt_as = &kas;

		if ((pt >= (u_int)ksegmappt) && (pt < (u_int)eksegmappt)) {
			ptd->pt_secseg = (SCN1 << 14) |
				((SEGNUM(kvsegmap) +
				 (((pt - (u_int)ksegmappt) >> 9))) << 1);
		} else if ((pt >= (u_int)ksegupt) && (pt < (u_int)eksegupt)) {
			ptd->pt_secseg = (SCN1 << 14) |
				((SEGNUM(kvsegu) +
				 (((pt - (u_int)ksegupt) >> 9))) << 1);
		} else {
			cmn_err(CE_PANIC, "hat_pt2ptdat: invalid pte ptr");
		}

		/*
		 * We add 1 so the hat will never try to free a kernel
		 * page table.
		 */
		ptd->pt_inuse = NPGPT+1;
		ptd->pt_keepcnt = NPGPT+1;
	} else {
		ASSERT(pp->p_ptdats != NULL);

		ptd = (ptdat_t *)(pp->p_ptdats) +
		  (((u_int)pt >> 9) & 3);
	}

	return (ptd);
}

/*
 * Return the page frame number corresponding to the virtual address vaddr.
 *
 * Required interface defined by the Driver-Kernel Interface (DKI).
 *
 */
u_int
hat_getkpfnum(vaddr)
	caddr_t vaddr;
{
	return( kvtopfn(vaddr) );
}

/*
 * hat_vtokp_prot(vprot):
 * used for kernel segments to check the requested virtual page
 * protections and to convert them to the physical protections.
 * Only ro/rw permissions are available in the page table entry
 * (using the copy-on-write bit).
 * So, only kernel level permissions are permitted.
 * The value returned is the value to be placed in the copy-on-write bit.
 */
u_int
hat_vtokp_prot(vprot)
	u_int	vprot;
{
	if (vprot & PROT_USER) {
		cmn_err(CE_PANIC, "hat_vtokp_prot: user addr in kernel space");
		/* NOTREACHED */
	}

	switch (vprot) {
	case 0:
		cmn_err(CE_PANIC, "hat_vtokp_prot: null permission");
		/* NOTREACHED */
	case PROT_READ:
	case PROT_EXEC:
	case PROT_READ | PROT_EXEC:
		return (1);
	case PROT_WRITE:
	case PROT_READ | PROT_WRITE:
	case PROT_READ | PROT_WRITE | PROT_EXEC:
		return (0);
	default:
		cmn_err(CE_PANIC, "hat_vtokp_prot: bad prot");
		/* NOTREACHED */
	}
}

/*
 * hat_growsdt(hatp, section, nentries)
 *
 * "grow" the specified section's segment table to the specified number of
 * entires.
 *
 * hatp     	-> pointer to the hat the section belongs to.
 * section	-> section containing the hardware segment.
 * nentries	-> number of entries in the new SDT[section] table.
 */

STATIC u_int
hat_growsdt(hatp, section, nentries)
	hat_t *hatp;
	int	section;
	register int	nentries;
{
	register oentries, nlastseg;
	register osize, nsize;
	SRAMA obase, nbase;
	register int i;
	int error;

	ASSERT(section == SCN2 || section == SCN3);

	if ((nentries > MAXSDTSEG) || (nentries < 0))
		return (ENOMEM);

	obase = hatp->hat_srama[section - SCN2];
	oentries = hatp->hat_sramb[section - SCN2].SDTlen + 1;

	/*
	 * (o|n)size = size of the SDT tables in units of minimum size
	 * segment tables.
	 */

	if (obase == mmu_invalid || obase == dflt_sdt_p)
		osize = 0;
	else
		osize = btosdt(sdetob(oentries));

	nsize = btosdt(sdetob(nentries));

	nlastseg = btosde(sdttob(nsize));

	if (nsize <= osize) {

		/* invalidate extra sde's at end of new last SDT */

		for (i = nentries; i < nlastseg; ++i)
			SD_CLRVALID((sde_t *)obase + i);

		/* free any extra SDT's */
		hat_sdtfree(obase + sdttob(nsize), osize - nsize);

		/* update SDT size in the hat structure */

		if (nentries == 0) {
			if (section == SCN2) {
				hatp->hat_srama[HAT_SCN2] = mmu_invalid;
				*(int *)&hatp->hat_sramb[HAT_SCN2] = 0;
			} else {
				hatp->hat_srama[HAT_SCN3] = dflt_sdt_p;
				*(int *)&hatp->hat_sramb[HAT_SCN3] = 0;
			}
		} else {
			*(int *)&hatp->hat_sramb[section - SCN2] =
				(nentries - 1) << SRAMBSHIFT;
		}
	} else {
		/* allocate new SDT */
		int slpflg = (nsize > NSDTPP) ? P_NOSLEEP : P_CANWAIT ;

		if ((error = hat_sdtalloc(&nbase, nsize, slpflg)) != 0)
			return (error);

		/* copy entries */

		bcopy((caddr_t)obase, (caddr_t)nbase, sdetob(oentries));

		/* free old SDT */

		if (osize == 0) {
			/*
			 * Don't worry, if we are not growing the segment
			 * table for the current process, the first entry
			 * will get set appropriately in pswtch().  We set
			 * the first entry now in case a loadmmu() is done
			 * after we return.
			 */
			if (section == SCN3) {
				register sde_t *sde = (sde_t *)nbase;

				sde->wd2.address =
					phys_ubptbl(u.u_procp->p_ubptbl);
				sde->seg_prot = KRWE;
				sde->seg_len = ctomo(USIZE);
				sde->seg_flags = SDE_flags;
			}
		} else {
			hat_sdtfree(obase, osize);
		}

		/* update SDT addr and size in the hat structure */

		hatp->hat_srama[section - SCN2] = kvtophys((caddr_t)nbase);
		*(int *)&hatp->hat_sramb[section - SCN2] =
			(nentries - 1) << SRAMBSHIFT;

	}

	return (0);
}

/*
 * hat_sdtalloc(base, n, flag)
 *
 * n = number of (NBPSDT byte) segment tables to allocate.
 *
 * flag & HAT_NOSLEEP	-> return immediately if no memory.
 * flag & HAT_CANWAIT	-> wait if no memory currently available.
 *
 * Allocate segment tables.  Typically called to get segment tables for
 * user process but may be called for ptdat structures from ptbl_alloc.
 *
 */
STATIC u_int
hat_sdtalloc(base, n, flag)
	paddr_t *base;
	int n;
	u_int flag;
{
	register u_int	x;
	register u_int	i;
	register u_int	mask;
	register page_t *pp;
	paddr_t physaddr;

	if (n == 0) {
		*base = NULL;
		return (0);
	}

	/*
	 * If we are trying to allocate less than a full
	 * page of segment tables, then check the list of
	 * pages which currently are being used for segment
	 * tables.
	 */
	
	if (n < NSDTPP) {
		mask  = setmask[n];

		for (pp = sdtfreelist.p_next;
		     pp != &sdtfreelist;
		     pp = pp->p_next) {
			x = (u_int)pp->p_sdtbits;
			for (i = 0; i <= NSDTPP - n; i++, x >>= 1)
				if ((x & mask) == 0) {
					/*
					 * We have found some segment tables.
					 * If no segment tables are left in
					 * the page, then remove page from
					 * the segment table list.
					 */
					pp->p_sdtbits = (caddr_t)
					  ((u_int)pp->p_sdtbits | (mask << i));

					if ((u_int)pp->p_sdtbits == setmask[NSDTPP]) {
						pp->p_prev->p_next = pp->p_next;
						pp->p_next->p_prev = pp->p_prev;
						pp->p_prev = NULL;
						pp->p_next = NULL;
					}

					/*
					 * Get address of segment table we
					 * have allocated.  Update the free
					 * segment table count and clear the
					 * segment table.
					 */
					physaddr = pfntophys(page_pptonum(pp)) +
					           sdttob(i);

					(void) bzero((caddr_t)physaddr, sdttob(n));
					*base = physaddr;
					return (0);
				}
		}
	}

	/*
	 * We could not allocate the required number of contiguous segment
	 * tables from a single page on the free list.
	 *
	 * Allocate some more physical memory.
	 */

	i = sdttopgs(n);
	if (availrmem - i < tune.t_minarmem ||
	    availsmem - i < tune.t_minasmem ){
		nomemmsg("hat_sdtalloc", i, 0, 0);
		*base = NULL;
		return (ENOMEM);
	}
	availrmem -= i;
	availsmem -= i;
	pages_pp_kernel += i;

	pp = page_get(ctob(i), (flag & HAT_CANWAIT)|P_PHYSCONTIG);
	if (pp == NULL) {
		cmn_err(CE_CONT, "hat_sdtalloc - not enough physically contiguouts memory for segment tables; $d pages.\n", i);
		availrmem += i;
		availsmem += i;
		pages_pp_kernel -= i;
		*base = NULL;
		return (EAGAIN);
	}
	ASSERT(pp->p_mapping == NULL);

	physaddr = pfntophys(page_pptonum(pp));

	ASSERT((physaddr & POFFMASK) == 0);

	/*
	 * Clear the free page table bit masks for all
	 * the pages that we have just allocated.
	 */
	for(; i > 1; --i, ++pp) {
		pp->p_sdtbits = (caddr_t) setmask[NSDTPP];
		pp->p_next = NULL;
		pp->p_prev = NULL;
	}

	/*
	 * Add any unused segment tables to the end of the
	 * free list.
	 *
	 * Note: the following only works if NSDTPP == 2^n;
	 * otherwise, use  i = n % NSDTPP.
	 */

	i = n & (NSDTPP - 1);
	if (i != 0) {
		pp->p_sdtbits = (caddr_t) setmask[i];
		pp->p_next = sdtfreelist.p_next;
		pp->p_prev = &sdtfreelist;
		sdtfreelist.p_next->p_prev = pp;
		sdtfreelist.p_next = pp;
	} else {
		pp->p_sdtbits = (caddr_t) setmask[NSDTPP];
		pp->p_next = NULL;
		pp->p_prev = NULL;
	}

	/*
	 * Update the count of the number of free page tables
	 * and zero out the page tables we have just allocated.
	 */
	(void) bzero((caddr_t)physaddr, sdttob(n));
	*base = physaddr;

	return (0);
}

/*
 * Free previously allocated segment tables
 */
STATIC void
hat_sdtfree(base, n)
	paddr_t	base;
	register int	n;
{
	register page_t *pp;
	register int	index;
	int	firstfree;

	if (n == 0)
		return;

	/*
	 * Get a pointer to the page structure for the page in
	 * which we are freeing segment tables.  Compute the index
	 * into the page of the first segment table being freed.
	 */
	pp = page_numtopp(phystopfn(base));
	ASSERT(pp != NULL);

	index = (base - (base & (~POFFMASK))) >> BPSDTSHFT;

	/*
	 * Compute the number of segment tables in the first page
	 * which we are freeing.
	 */
	firstfree = MIN(n, NSDTPP - index);

	/* Free segment tables up to first page boundary.
	 */
	if (firstfree < NSDTPP) {
		pp->p_sdtbits = (caddr_t)
		  ((u_int)pp->p_sdtbits & ~(setmask[firstfree] << index));

		if (pp->p_sdtbits == 0) {
			/* Should already be on the freelist because
			 * we didn't free the whole page but it is
			 * completely deallocated.
			 */
			ASSERT(pp->p_next != NULL);
			ASSERT(pp->p_prev != NULL);
			pp->p_prev->p_next = pp->p_next;
			pp->p_next->p_prev = pp->p_prev;
			PAGE_RELE(pp);
			++availrmem;
			++availsmem;
			--pages_pp_kernel;
		} else if (pp->p_next == NULL) {
			/* pp is not on the freelist yet, this must be
			 * the first free in this page.
			 */
			ASSERT(pp->p_prev == NULL);
			ASSERT(((u_int)pp->p_sdtbits |
				(setmask[firstfree]<<index)) == 0xffffffff);

			pp->p_next = sdtfreelist.p_next;
			pp->p_prev = &sdtfreelist;
			sdtfreelist.p_next->p_prev = pp;
			sdtfreelist.p_next = pp;
		}

		++pp;
		n -= firstfree;
	}

	/*
	 * Free any full pages of segment tables.
	 */
	while (n >= NSDTPP) {
		/* Page should not be on the free list because we're
		 * freeing the whole page, so it should have been
		 * completely allocated.
		 */
		ASSERT(pp->p_next == NULL);
		ASSERT(pp->p_prev == NULL);
		ASSERT((u_int)pp->p_sdtbits == 0xffffffff);

		pp->p_sdtbits = 0;

		PAGE_RELE(pp);

		++availrmem;
		++availsmem;
		--pages_pp_kernel;

		++pp;
		n -= NSDTPP;
	}

	/* Free up any trailing segment tables.
	 */
	if (n > 0) {
		pp->p_sdtbits = (caddr_t)
		  ((u_int)pp->p_sdtbits & ~(setmask[n]));

		if (pp->p_sdtbits == 0) {
			/* Should already be on the freelist because
			 * we didn't free the whole page but it is
			 * completely deallocated.
			 */
			ASSERT(pp->p_next != NULL);
			ASSERT(pp->p_prev != NULL);
			pp->p_prev->p_next = pp->p_next;
			pp->p_next->p_prev = pp->p_prev;
			PAGE_RELE(pp);
			++availrmem;
			++availsmem;
			--pages_pp_kernel;
		} else if (pp->p_next == NULL) {
			/* pp is not on the freelist yet, this must be
			 * the first free in this page.
			 */
			ASSERT(pp->p_prev == NULL);
			ASSERT(((u_int)pp->p_sdtbits |
				(setmask[firstfree]<<index)) == 0xffffffff);

			pp->p_next = sdtfreelist.p_next;
			pp->p_prev = &sdtfreelist;
			sdtfreelist.p_next->p_prev = pp;
			sdtfreelist.p_next = pp;
		}
	}

	return;
}


/*
 * pte = hat_ptalloc(ptd_ret, flag)
 *
 * pte = address of the first pte in the page table that was allocated.
 *
 * ptd_ret = address of a ptr to the ptdat structure for the page table to be
 *             allocated.
 *
 * flag & HAT_NOSLEEP	-> return immediately if no memory.
 * flag & HAT_CANWAIT	-> wait if no memory currently available.
 * flag & HAT_NOSTEAL	-> don't steal a page table from another proc.
 *
 * Allocate page tables; below are some macros that will be used in
 * 	hat_ptalloc() and hat_ptfree() .
 *
 */

#define	APPEND_PT(PTD, LIST)	{ \
					LIST.pt_prev->pt_next = PTD; \
					PTD->pt_prev = LIST.pt_prev; \
					PTD->pt_next = &LIST; \
					LIST.pt_prev = PTD; \
				}

#define	PREPEND_PT(PTD, LIST)	{ \
					LIST.pt_next->pt_prev = PTD; \
					PTD->pt_next = LIST.pt_next; \
					PTD->pt_prev = &LIST; \
					LIST.pt_next = PTD; \
				}

#define REMOVE_PT(PTD)		{ \
					PTD->pt_prev->pt_next = PTD->pt_next; \
					PTD->pt_next->pt_prev = PTD->pt_prev; \
				}

STATIC pte_t *
hat_ptalloc(ptd_ret, flag)
	ptdat_t	**ptd_ret;
	u_int	flag;
{
	register ptdat_t *ptd;
	register paddr_t  physaddr;
	register pte_t   *pte;
	register pte_t  **nextpte;
	page_t	*pp;
	struct	as	*as;
	struct	as	*cur_as;
	sde_t	*sde;
	int	section;
	int	segment;
	int	i;

tryagain:

	if ((ptd = free_pts.pt_next) != &free_pts) {
		/* Take ptdat off of page table free list.
		 */
		REMOVE_PT(ptd);

		/* Put ptdat on the end of the active page table list.
		 */
		APPEND_PT(ptd, active_pts);

		/* Update page table bitmap.
		 */
		pp = ptd->pt_pp;
		pte = ptd->pt_addr;
		physaddr = (u_int)pte;
		pp->p_ptbits |=
		  1 << ((physaddr - (ctob(btoct(physaddr)))) >> (BPTSHFT + 1));

		/* Return ptdat and page table address.
		 */
		bzero((caddr_t)pte, NBPPT);
		*ptd_ret = ptd;
		return (pte);
	}

	/* Try to allocate a new page for page tables.
	 */
#ifdef DEBUG
	if (hat_pgcnt < hat_ptpgs)
#endif
	if (availrmem - 1 < tune.t_minarmem ||
	    availsmem - 1 < tune.t_minasmem) {
		nomemmsg("hat_ptalloc", 1, 0, 0);
	} else {
		--availrmem;
		--availsmem;
		++pages_pp_kernel;

		if ((pp = page_get(NBPP, P_NOSLEEP)) == NULL) {
			++availrmem;
			++availsmem;
			--pages_pp_kernel;
#ifdef DEBUG
			++hat_pgfail;
#endif
		} else if (hat_sdtalloc((u_int *)ptd_ret, 1, HAT_NOSLEEP) != 0) {
			ASSERT(pp->p_mapping == NULL);
			PAGE_RELE(pp);
			++availrmem;
			++availsmem;
			--pages_pp_kernel;
		} else {
			/* Return ptdat and page table address.
			 */
#ifdef DEBUG
			++hat_pgcnt;
#endif
			ASSERT(pp->p_mapping == NULL);
			ptd = *ptd_ret;
			pp->p_ptbits = 1;
			pp->p_ptdats = (caddr_t) ptd;
			physaddr = pfntophys(page_pptonum(pp));
			pte = (pte_t *)physaddr;
			bzero((caddr_t)pte, NBPPT);
			APPEND_PT(ptd, active_pts);

			/* Put the rest of the page tables on the freelist.
			 */
			for (i = 0; i < ((NPTPP >> 1) - 1); i++) {
				++ptd;
				physaddr += (NBPPT << 1);
				ptd->pt_addr = (pte_t *)physaddr;
				ptd->pt_pp = pp;
				APPEND_PT(ptd, free_pts);
			}

			return (pte);
		}
	}

	/* Try to steal a page table from some other process.
	 */
	if ((flag & HAT_NOSTEAL) == 0) {

		cur_as = u.u_procp->p_as;

		for (ptd = active_pts.pt_next;
		     ptd != &active_pts;
		     ptd = ptd->pt_next) {
#ifdef DEBUG
			++hat_ptstolen;
#endif

			ASSERT((ptd->pt_as != NULL) && (ptd->pt_as != &kas));

			/* XXX - TEMPORARY
			 * if ((ptd->pt_keepcnt == 0) && (ptd->pt_as != cur_as)) {
			 */
			if (ptd->pt_keepcnt == 0) {
				addr_t addr = (addr_t)(ptd->pt_secseg << 16);

				as = ptd->pt_as;
				section = ptd->pt_secseg >> 14;

				ASSERT(section == SCN2 || section == SCN3);

				sde = (sde_t *)as->a_hat.hat_srama[section - SCN2];
				segment = (ptd->pt_secseg >> 1) & SEGNMASK;

				ASSERT(segment <= (int)as->a_hat.hat_sramb[section-SCN2].SDTlen);

				/* Doesn't overlap with u-area; the u-area's
				 * page table is in the proc structure, it
				 * shouldn't be on the active list.
				 */
				ASSERT((section != SCN3) || (segment != 0));

				sde += segment;
				ASSERT(SD_ISVALID(sde));
				physaddr = sde->wd2.address;
				ASSERT(physaddr != NULL);
				SD_CLRVALID(sde);

				pte = (pte_t *)physaddr;
				for(i = 0; i < NPGPT; i++, pte++, addr += NBPP){
					if (pte->pg_pte == NULL)
						continue;
					as->a_rss--;
					pp = page_numtopp(pte->pgm.pg_pfn);
					if (pp != NULL) {
						pp->p_ref |= pte->pgm.pg_ref;
						pp->p_mod |= pte->pgm.pg_mod;

						/* Must remove pte from its
						 * p_mapping list.
						 */
						nextpte =
						  (pte_t **)(&(pp->p_mapping));
						while (*nextpte != pte) {
							ASSERT(*nextpte != NULL);
							nextpte =
							  (pte_t **)(*nextpte + NPGPT);
						}
						*nextpte = *((pte_t **)(pte + NPGPT));
					}
					pte->pg_pte = 0;
					flushaddr(addr);
				}

				/* Move page table to end of active list.
				 */
				REMOVE_PT(ptd);
				APPEND_PT(ptd, active_pts);

				/* Return the page table's address and ptdat.
				 * Already cleared the pte's above.
				 */
				*ptd_ret = ptd;
				return ((pte_t *)physaddr);
			}
		}
	}

	if ((flag & HAT_CANWAIT) == 0)
		return((pte_t *)NULL);

#ifdef DEBUG
	if (hat_debug & 1)
		cmn_err(CE_CONT, "hat_ptalloc - have to wait.\n");
#endif

	++pt_waiting;
	sleep((caddr_t)&free_pts, PSWP+2);
	goto tryagain;
}

/*
 * Free previously allocated page tables
 */
STATIC void
hat_ptfree(pt)
	paddr_t pt;
{
	register page_t	*pp;
	register ptdat_t	*ptd;
	register int	i;
	int	index;

	ASSERT(pt != NULL);

	pp = page_numtopp(phystopfn(pt));
	ASSERT(pp != NULL);

	index = (pt - ctob(btoct(pt))) >> (BPTSHFT + 1);

	ptd = (ptdat_t *)pp->p_ptdats + index;
	ASSERT(ptd->pt_keepcnt == 0);
	ptd->pt_addr = (pte_t *)pt;
	ptd->pt_pp = pp;

	pp->p_ptbits &= ~(0x1 << index);

	if ((pp->p_ptbits & PT_RESERVE) != 0) {
		/* Remove ptd from active page table list and put it
		 * at the head of the free page table list.
		 */
		REMOVE_PT(ptd);
		PREPEND_PT(ptd, free_pts);
		if (pt_waiting) {
			wakeup((caddr_t)&free_pts);
			pt_waiting = 0;
		}
	} else {
		if (pt_waiting) {
			wakeup((caddr_t)&free_pts);
			pt_waiting = 0;
		} else if (pp->p_ptbits == 0) {
#ifdef DEBUG
			--hat_pgcnt;
#endif
			/* Remove the other page tables, in this page, from
			 * the freelist and remove the current one from the
			 * active list.
			 */
			ptd = (ptdat_t *)pp->p_ptdats;
			for (i = 0; i < (NPTPP >> 1); i++, ptd++) {
				REMOVE_PT(ptd);
			}

			hat_sdtfree(pp->p_ptdats, 1);

			/* Free the page.
			 */
			pp->p_ptdats = NULL;
			PAGE_RELE(pp);
			++availrmem;
			++availsmem;
			--pages_pp_kernel;
			return;
		}
		/* Remove ptd from active page table list and put it
		 * on the tail of the free page table list.
		 */
		REMOVE_PT(ptd);
		APPEND_PT(ptd, free_pts);
	}
}

int
hat_exec(oas, ostka, stksz, nas, nstka, hatflag)
 struct as *oas;
 addr_t ostka;
 int stksz;
 struct as *nas;
 addr_t nstka;
 u_int hatflag;
{
	struct hat *hatp = &oas->a_hat;
	int osec;
	int nsec;
	addr_t endaddr;
	addr_t nextaddr;
	int skipsize;
	int curnseg, endnseg;
	int curoseg;
	int error;
	sde_t *osde;
	sde_t *nsde;
	struct ptdat *ptd;
	struct ptdat *optd;
	struct ptdat ptdat_tmp;
	pte_t *pt;
	pte_t *opte;
	pte_t *npt;
	pte_t *npte;
	pte_t **nextpte;
	int pfn;
	struct page *pp;

	ASSERT(PAGOFF(stksz) == 0);
	ASSERT(PAGOFF((u_int)ostka) == 0);
	ASSERT(PAGOFF((u_int)nstka) == 0);
	nsec = SECNUM(nstka);
	osec = SECNUM(ostka);
	endaddr = nstka + stksz - 1;
	curoseg = SEGNUM(ostka);
	curnseg = SEGNUM(nstka);
	endnseg = SEGNUM(endaddr);
	if ((error = hat_growsdt(&nas->a_hat, nsec, endnseg+1)) != 0)
		return(error);
	osde = (sde_t *)oas->a_hat.hat_srama[osec - SCN2];
	osde += curoseg;
	nsde = (sde_t *)nas->a_hat.hat_srama[nsec - SCN2];
	nsde += curnseg;
	if (hatflag) {
		/* Move the page tables themselves as the flag
		 * states that they contain only pages to be moved
		 * and the pages are properly aligned in the table.
		 */

		for (; curnseg <= endnseg; osde++, nsde++, curnseg++) {
			if (!SD_ISVALID(osde))
				continue;
			pt = (pte_t *)osde->wd2.address;
			ptd = hat_pt2ptdat(pt, &ptdat_tmp);
			ptd->pt_as = nas;
			ptd->pt_secseg = (((nsec << 13) | curnseg) << 1);
			*nsde = *osde;
			SD_CLRVALID(osde);
			oas->a_rss -= ptd->pt_inuse;
			nas->a_rss += ptd->pt_inuse;
		}
		loadmmu(hatp, osec);
		return(0);
	}

	/* The PTEs have to be moved */

	while (stksz > 0) {
		if (!SD_ISVALID(osde)) {
			nextaddr = (addr_t) roundup((u_int)ostka+1, stob(1));
			skipsize = nextaddr - ostka;
			osde++;
			stksz -= skipsize;
			ostka = nextaddr;
			curoseg++;
			nstka += skipsize;
			if (SEGNUM(nstka) != curnseg) {
				nsde++;
				curnseg++;
			}
			continue;
		}
		pt = (pte_t *)osde->wd2.address;
		opte = pt + PAGNUM(ostka);
		if (opte->pg_pte == 0) {
			goto nextpage;
		}
		pfn = opte->pgm.pg_pfn;
		pp = page_numtopp(pfn);
		if (!SD_ISVALID(nsde)) {
			ASSERT(pp != NULL);
			pp->p_keepcnt++;
			optd = hat_pt2ptdat(pt, &ptdat_tmp);
			optd->pt_keepcnt++;
			if (hat_ptalloc((paddr_t *)&npt, &ptd, 0)!= 0) {
				cmn_err(CE_PANIC,"hat_exec: hat_ptalloc failed.");
				/* NOTREACHED */
			}
			optd->pt_keepcnt--;
			pp->p_keepcnt--;
			ptd->pt_as = nas;
			ptd->pt_secseg = ((u_int)nstka >> SEGNSHFT) << 1;
			ptd->pt_inuse = 0;
			ptd->pt_keepcnt = 0;
			nsde->wd2.address = (paddr_t) pt;
			nsde->seg_prot = KRWE|URWE;
			nsde->seg_flags = SDE_flags;
			nsde->seg_len = (stob(1)>>3) - 1;
		}
		else
			npt = (pte_t *) nsde->wd2.address;
		npte = npt + PAGNUM(nstka);
		npte->pg_pte = pt->pg_pte;
		nextpte = (pte_t **) (&(pp->p_mapping));
		while (*nextpte != opte) {
			ASSERT(*nextpte != (pte_t *)NULL);
			nextpte =
			  (pte_t **)(*nextpte + NPGPT);
		}
		*((pte_t **)(npte + NPGPT)) = *((pte_t **)(opte + NPGPT));
		opte->pg_pte = 0;
		/* the convention elsewhere (hat_unload) is that
		 * the old mapping pointer need not be cleared.
		 */
		*nextpte = npte;
		ptd = hat_pt2ptdat(pt, &ptdat_tmp);
		oas->a_rss--;
		if (--ptd->pt_inuse == 0) {
			hat_ptfree(pt);
			SD_CLRVALID(osde);
		}
		ptd = hat_pt2ptdat(npt, &ptdat_tmp);
		ptd->pt_inuse++;
		nas->a_rss++;
nextpage:
		ostka += NBPP;
		if (SEGNUM(ostka) != curoseg) {
			osde++;
			curoseg++;
		}
		nstka += NBPP;
		if (SEGNUM(nstka) != curnseg) {
			nsde++;
			curnseg++;
		}
		stksz -= NBPP;
	}
	return(0);
}

void
hat_asload()
{
	register struct hat *hatp = &(u.u_procp->p_as->a_hat);

	loadmmu(hatp, SCN2);
	loadmmu(hatp, SCN3);
}

/*
 * Determine (kernel) virtual address of the sde_t needed for translation
 * of virtual address within the address space of current process
 *
 *	va	--> virtual address
 */

sde_t *
vatosde(va)
	caddr_t va;
{
	register sde_t	*sde;
	register proc_t	*p;
	register int	sid;
	union {
		unsigned intvar;
		VAR vaddr;
	} virtaddr;

	virtaddr.intvar = (unsigned)va;
	sid = virtaddr.vaddr.v_sid;

	/*
	 * Set sde to base of required SDT.
	 */

	switch (sid) {
	case SCN0:
	case SCN1:
		sde = (sde_t *)srama[sid];
		break;
	case SCN2:
	case SCN3:
		p = u.u_procp;
		sde = (sde_t *)p->p_as->a_hat.hat_srama[sid - SCN2];
		break;
	}
	sde += virtaddr.vaddr.v_ssl;

	return (sde);
}

/*
 * Determine (kernel) virtual address of the pte_t needed for translation
 * of virtual address within the address space of current process
 *
 *	va	--> virtual address
 *	sde	--> (kernel) virtual address of segment descriptor used for
 *		 translating va
 */

/*
 * Really only needed for backward compatibility. No reason why this
 * shouldn't be a macro. Undef it from sys/immu.h .
 */
#undef	vatopte

pte_t *
vatopte(va, sde)
	caddr_t va;
	register sde_t *sde;
{
	register pte_t *pte;
	union {
		caddr_t intvar;
		VAR vaddr;
	} virtaddr;

	virtaddr.intvar = va;

	pte = (pte_t *)sde->wd2.address + virtaddr.vaddr.v_psl;
	return (pte);
}

/*
 * Routine to translate virtual addresses to physical addresses.
 * Used by the floppy, Integral, and Duart driver to access the dma.
 * If an invalid address is received vtop returns a 0.
 */

#define EBADDR	0

extern struct proc *curproc;

paddr_t
vtop(vaddr, pp)
	register caddr_t vaddr;
	proc_t *pp;
{
	register paddr_t	retval;
	register sde_t		*sp;
	register sde_t		*sde_p;
	register pte_t		*pte_p;
	register int		sid;
	register		s;

	s = splhi();
	sid = SECNUM(vaddr);
	if (sid == SCN0 || sid == SCN1 || pp == curproc) {
		splx(s);
		return (svirtophys(vaddr));
	} else {
		sp = (sde_t *)pp->p_as->a_hat.hat_srama[sid - SCN2];
		sde_p = &sp[SEGNUM(vaddr)];
		if (SD_INDIRECT(sde_p))
			sde_p = (sde_t *)(sde_p->wd2.address);

		/* perform validity check */

		if ((sde_p->seg_flags & SDE_flags) != SDE_flags) {
			splx(s);
			return ((paddr_t)EBADDR);
		}

		/* perform maximum offset check */

		if (((u_int)vaddr & MSK_IDXSEG) > (long)motob(sde_p->seg_len)) {
			splx(s);
			return ((paddr_t)EBADDR); 
		}

		if (SD_ISCONTIG(sde_p)) {
			retval = (paddr_t)((long)(sde_p->wd2.address & ~0x7) +
				 ((u_int)vaddr & MSK_IDXSEG));
		} else {
			pte_p = (pte_t *)((int)sde_p->wd2.address & ~0x1f);
			pte_p = &pte_p[((u_int)vaddr >> 11) & 0x3f];
			if (!pte_p->pgm.pg_v) {
				splx(s);
				return(EBADDR);
			}
			retval = pfntophys(pte_p->pgm.pg_pfn) + PAGOFF(vaddr);
		}
		splx(s);
		return (retval);
	} 
}

paddr_t
svirtophys(vaddr)
	long vaddr;
{
	register int	retval;
	register int	prio;
	register int	tempaddr;
	register int	caddrsave;
	int		svirtophyserr();

	prio = splhi();
	tempaddr = vaddr;
	vaddr = (tempaddr & ~3);
	caddrsave = u.u_caddrflt;
	u.u_caddrflt = (int)svirtophyserr;
	asm("	MOVTRW *0(%ap),%r8");
	asm("	NOP");
	u.u_caddrflt = caddrsave;
	retval |= (tempaddr & 3);
	splx(prio);
	return ((paddr_t) retval);
}

void
svirtophysfail()
{
	asm("	.globl	svirtophyserr");
	asm("svirtophyserr:	");
	cmn_err(CE_PANIC, "svirtophys - movtrw failed.");
	/* NOTREACHED */
}

/*
 * This routine flushes the mmu for a range of addresses.
 */

void
flushmmu(va, npgs)
	register caddr_t	va;	/* First virtual address to flush. */
	register int		npgs;	/* Number of pages to flush. */
{
	register int i;
	for (i = 0; i < npgs; i++)
		flushaddr(va + ctob(i));
}
