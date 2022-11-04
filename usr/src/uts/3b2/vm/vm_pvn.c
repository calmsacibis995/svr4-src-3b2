/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:vm/vm_pvn.c	1.19"

/*
 * VM - paged vnode.
 *
 * This file supplies vm support for the vnode operations that deal with pages.
 */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/inline.h"
#include "sys/time.h"
#include "sys/buf.h"
#include "sys/vnode.h"
#include "sys/uio.h"
#include "sys/vmmeter.h"
#include "sys/vmsystm.h"
#include "sys/mman.h"
#include "sys/vfs.h"
#include "sys/cred.h"
#include "sys/kmem.h"
#include "sys/cmn_err.h"
#include "sys/debug.h"
#include "sys/sysinfo.h"
#include "vm/trace.h"

#include "vm/hat.h"
#include "vm/as.h"
#include "vm/seg.h"
#include "vm/rm.h"
#include "vm/pvn.h"
#include "vm/page.h"
#include "vm/seg_map.h"
#include "vm/vmlog.h"

extern void	bp_mapout();
extern int	kzero();
extern void	pageio_done();

STATIC int pvn_debug = 0;
STATIC int pvn_nofodklust = 0;
STATIC int pvn_range_noklust = 0;

#define	dprint		if (pvn_debug) cmn_err
#define	dprint2		if (pvn_debug > 1) cmn_err
#define	dprint3		if (pvn_debug > 2) cmn_err

/*
 * Find the largest contiguous block which contains `addr' for file offset
 * `offset' in it while living within the file system block sizes (`vp_off'
 * and `vp_len') and the address space limits for which no pages currently
 * exist and which map to consecutive file offsets.
 */
page_t *
pvn_kluster(vp, off, seg, addr, offp, lenp, vp_off, vp_len, isra)
	struct vnode *vp;
	register u_int off;
	register struct seg *seg;
	register addr_t addr;
	u_int *offp, *lenp;
	u_int vp_off, vp_len;
	int isra;
{
	register int delta, delta2;
	register page_t *pp;
	page_t *plist = NULL;
	addr_t straddr;
	int bytesavail;
	u_int vp_end;

	ASSERT(off >= vp_off && off < vp_off + vp_len);

	/*
	 * We only want to do klustering/read ahead if there
	 * is more than minfree pages currently available.
	 */
	if (freemem - minfree > 0)
		bytesavail = ptob(freemem - minfree);
	else
		bytesavail = 0;

	if (bytesavail == 0) {
		if (isra)
			return ((page_t *)NULL);	/* ra case - give up */
		else
			bytesavail = PAGESIZE;		/* just pretending */
	}

	if (bytesavail < vp_len) {
		/*
		 * Don't have enough free memory for the
		 * max request, try sizing down vp request.
		 */
		delta = off - vp_off;
		vp_len -= delta;
		vp_off += delta;
		if (bytesavail < vp_len) {
			/*
			 * Still not enough memory, just settle for
			 * bytesavail which is at least PAGESIZE.
			 */
			vp_len = bytesavail;
		}
	}

	vp_end = vp_off + vp_len;
	ASSERT(off >= vp_off && off < vp_end);

	if (page_find(vp, off))
		return ((page_t *)NULL);		/* already have page */

	if (vp_len <= PAGESIZE || pvn_nofodklust) {
		straddr = addr;
		*offp = off;
		*lenp = MIN(vp_len, PAGESIZE);
	} else {
		/* scan forward from front */
		for (delta = PAGESIZE; off + delta < vp_end;
		    delta += PAGESIZE) {
			if (page_find(vp, off + delta))
				break;		/* already have this page */
			/*
			 * Call back to the segment driver to verify that
			 * the klustering/read ahead operation makes sense.
			 */
			if ((*seg->s_ops->kluster)(seg, addr, delta))
				break;		/* page not file extension */
		}
		delta2 = delta;

		/* scan back from front */
		for (delta = 0; off + delta > vp_off; delta -= PAGESIZE) {
			if (page_find(vp, off + delta - PAGESIZE))
				break;		/* already have the page */
			/*
			 * Call back to the segment driver to verify that
			 * the klustering/read ahead operation makes sense.
			 */
			if ((*seg->s_ops->kluster)(seg, addr, delta - PAGESIZE))
				break;		/* page not eligible */
		}

		straddr = addr + delta;
		*offp = off = off + delta;
		*lenp = delta2 - delta;
		ASSERT(off >= vp_off);

		if ((vp_off + vp_len) < (off + *lenp)) {
			ASSERT(vp_end > off);
			*lenp = vp_end - off;
		}
	}

	/*
	 * Allocate pages for <vp, off> at <seg, addr> for delta bytes.
	 * Note that for the non-read ahead case we might not have the
	 * memory available right now so that rm_allocpage operation could
	 * sleep and someone else might race to this same spot if the
	 * vnode object was not locked before this routine was called.
	 */
	delta2 = *lenp;
	delta = roundup(delta2, PAGESIZE);
	/* `pp' list kept */
	pp = rm_allocpage(seg, straddr, (u_int)delta, P_CANWAIT);

	plist = pp;
	do {
		pp->p_intrans = 1;
		pp->p_pagein = 1;

#ifdef TRACE
		{
			addr_t taddr = straddr + (off - *offp);

			trace3(TR_SEG_KLUSTER, seg, taddr, isra);
			trace6(TR_SEG_ALLOCPAGE, seg, taddr, TRC_SEG_UNK,
			    vp, off, pp);
		}
#endif /* TRACE */
		if (page_enter(pp, vp, off)) {		/* `pp' locked if ok */
			/*
			 * Oops - somebody beat us to the punch
			 * and has entered the page before us.
			 * To recover, we use pvn_fail to free up
			 * all the pages we have already allocated
			 * and we return NULL so that whole operation
			 * is attempted over again.  This should never
			 * happen if the caller of pvn_kluster does
			 * vnode locking to prevent multiple processes
			 * from creating the same pages as the same time.
			 */
dprint(CE_CONT, "pvn_kluster:  page exists, aborting %x <%x, %x>\n", pp, vp, off);
			pvn_fail(plist, B_READ);
			return ((page_t *)NULL);
		}
		off += PAGESIZE;
	} while ((pp = pp->p_next) != plist);

	return (plist);
}

/*
 * Entry point to be use by page r/w subr's and other such routines which
 * want to report an error and abort a list of pages setup for pageio
 * which do not do though the normal pvn_done processing.
 */
void
pvn_fail(plist, flags)
	page_t *plist;
	int flags;
{
	static struct buf abort_buf;
	struct buf *bp;
	page_t *pp;
	int len;
	int s;

VMLOG(X_PVNFAIL, plist, plist->p_vnode, plist->p_offset);
	len = 0;
	pp = plist;
	do {
		len += PAGESIZE;
	} while ((pp = pp->p_next) != plist);

	bp = &abort_buf;
	s = splimp();
	while (bp->b_pages != NULL) {
dprint(CE_CONT, "pvn_fail: abort_buf busy, sleeping - plist %x, flags %x\n",
    plist, flags);
		(void) sleep((caddr_t)&bp->b_pages, PSWP+2);
	}
dprint(CE_CONT, "pvn_fail: aborting - plist %x, flags %x\n", plist, flags);
	(void) splx(s);
	/* ~B_PAGEIO is a flag to pvn_done not to pageio_done the bp */
	bp->b_flags = B_KERNBUF | B_ERROR | B_ASYNC | (flags & ~B_PAGEIO);
	bp->b_pages = plist;
	bp->b_bcount = len;
	pvn_done(bp);			/* let pvn_done do all the work */
	if (bp->b_pages != NULL) {
		/* XXX - this should never happen, should it be a panic? */
dprint(CE_CONT, "pvn_fail: page list not empty! - plist %x\n", plist);
		bp->b_pages = NULL;
	}
	wakeup((caddr_t)&bp->b_pages);
}

/*
 * Routine to be called when pageio's complete.
 *
 * XXX - need to check all the page transitions
 */
void
pvn_done(bp)
	register struct buf *bp;
{
	register page_t *pp;
	register int bytes;

	pp = bp->b_pages;
VMLOG(X_PVNDONE, bp, pp->p_vnode, pp->p_offset);

	/*
	 * Release any I/O mappings to the pages described by the
	 * buffer that are finished before processing the completed I/O.
	 */
	if ((bp->b_flags & B_REMAPPED) && (pp->p_nio <= 1))
		bp_mapout(bp);

	/*
	 * Handle of each page in the I/O operation.
	 */
	for (bytes = 0; bytes < bp->b_bcount; bytes += PAGESIZE) {
		if (pp->p_nio > 1) {
			/*
			 * There were multiple IO requests outstanding
			 * for this particular page.  This can happen
			 * when the file system block size is smaller
			 * than PAGESIZE.  Since there are more IO
			 * requests still outstanding, we don't process
			 * the page given on the buffer now.
			 */
			if (bp->b_flags & B_ERROR) {
				if (bp->b_flags & B_READ) {
dprint(CE_CONT, "page read failure with nio %d on <%x, %x>, aborting page %x\n",
    pp->p_nio, pp->p_vnode, pp->p_offset, pp);
                                        trace3(TR_PG_PVN_DONE, pp, pp->p_vnode,
                                                pp->p_offset);
					page_abort(pp);	/* assumes no waiting */
				} else {
dprint(CE_CONT, "page write failure with nio %d on <%x, %x>, marking page %x dirty\n",
    pp->p_nio, pp->p_vnode, pp->p_offset, pp);
					pp->p_mod = 1;
				}
			}
			pp->p_nio--;
			break;
			/* real page locked for the other io operations */
		}

		pp = bp->b_pages;
		page_sub(&bp->b_pages, pp);
VMLOG(X_PVNDONE_OTHER, pp, pp->p_vnode, pp->p_offset);

		pp->p_intrans = 0;
		pp->p_pagein = 0;

		PAGE_RELE(pp);

VMLOG(X_PVNDONE_PAGE, pp, pp->p_vnode, pp->p_offset);

		/*
		 * Check to see if page was freed by the PAGE_RELE()
		 */
		if (pp->p_free)
			continue;

		/*
		 * Check to see if the page is now gone or has error
		 */
		if ((pp->p_gone || pp->p_vnode == NULL ||
		    ((bp->b_flags & (B_ERROR|B_READ)) == (B_ERROR|B_READ)))) {
dprint2(CE_CONT, "pvn_done aborting page %x <%x, %x>\n", pp, pp->p_vnode, pp->p_offset);
			page_abort(pp);
			continue;
		}

		/*
		 * Check if we are going to do invalidation
		 */
		if ((bp->b_flags & B_INVAL) != 0) {
VMLOG(X_PVNDONE_INVAL, bp, bp->b_flags, pp);
dprint2(CE_CONT, "pvn_done invalidate %x <%x, %x>\n", pp, pp->p_vnode, pp->p_offset);
			page_abort(pp);
			continue;
		}

		if ((bp->b_flags & (B_ERROR | B_READ)) == B_ERROR) {
VMLOG(X_PVNDONE_WRITE_ERR, bp, bp->b_flags, pp);
			if (bp->b_flags & B_FREE) {
dprint2(CE_CONT, "pageout failure [ENOMEM?] on <%x, %x>, making page %x dirty again\n",
    pp->p_vnode, pp->p_offset, pp);
			} else {
dprint(CE_CONT, "page write failure on <%x, %x>, making page %x dirty again\n",
    pp->p_vnode, pp->p_offset, pp);
			}
			/*
			 * Write operation failed.  We don't want
			 * to abort (or free) the page.  We set
			 * the mod bit again so it will get
			 * written back again later when things
			 * are hopefully better again.
			 */
			pp->p_mod = 1;
		}

		if (bp->b_flags & B_FREE) {
			cnt.v_pgpgout++;
			vminfo.v_pgpgout++;
			if (pp->p_keepcnt == 0) {
				/*
				 * Check if someone has reclaimed the
				 * page.  If no ref or mod, no one is
				 * using it so we can free it.
				 * The rest of the system is careful
				 * to use hat_ghostunload to unload
				 * translations set up for IO w/o
				 * affecting ref and mod bits.
				 */
				if (pp->p_mod == 0 && pp->p_mapping)
					hat_pagesync(pp);
				if ((!pp->p_ref && !pp->p_mod) ||
				    pp->p_gone || pp->p_vnode == NULL) {
					if (pp->p_mapping)
						hat_pageunload(pp);
VMLOG(X_PVNDONE_DIRTY_FREE, pp, *(int *)pp, pp->p_offset);
					page_free(pp,
					    (int)(bp->b_flags & B_DONTNEED));
					cnt.v_dfree++;
					vminfo.v_dfree++;
				} else {
dprint2(CE_CONT, "page %x reclaimed, ref %d mod %d\n", pp, pp->p_ref, pp->p_mod);
					page_unlock(pp);
					cnt.v_pgrec++;
					vminfo.v_pgrec++;
				}
			} else {
VMLOG(X_PVNDONE_DIRTY_UNLOCK, pp, *(int *)pp, pp->p_offset);
				page_unlock(pp);
			}
			continue;
		}

VMLOG(X_PVNDONE_UNLOCK, pp, *(int *)pp, pp->p_offset);
		page_unlock(pp);		/* a read or write */
	}

	/*
	 * Count pageout operations if applicable.  Release the
	 * buf struct associated with the operation if async & pageio.
	 */
	if (bp->b_flags & B_FREE) {
		cnt.v_pgout++;
		vminfo.v_pgout++;
	}
	if ((bp->b_flags & (B_ASYNC | B_PAGEIO)) == (B_ASYNC | B_PAGEIO))
		pageio_done(bp);
}

/*
 * Flags are composed of {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED, B_DELWRI}
 * B_DELWRI indicates that this page is part of a kluster operation and
 * is only to be considered if it doesn't involve any waiting here.
 * Returns non-zero if page added to dirty list.
 */
STATIC int
pvn_getdirty(pp, dirty, flags)
	register page_t *pp, **dirty;
	int flags;
{
	struct vnode *vp;
	u_int offset;

	vp = pp->p_vnode;
	offset = pp->p_offset;
	if (pp->p_free) {
		if ((flags & B_INVAL) != 0) {
			/*
			 * Since the page is already clean,
			 * we can just abort it here.
			 */
VMLOG(X_PVNDIRTY_ABORT, pp, pp->p_offset, flags);
			page_abort(pp);
		}
VMLOG(X_PVNDIRTY_FREE_RET, pp, pp->p_offset, flags);
		return (0);
	}

	if ((flags & B_DELWRI) != 0 && (pp->p_keepcnt > 0 || pp->p_lock)) {
		/*
		 * This is a klusting case that would
		 * case us to block, just give up.
		 */
		return (0);
	}

	if (pp->p_intrans && (flags & (B_INVAL | B_ASYNC)) == B_ASYNC) {
		/*
		 * Don't bother waiting for an intrans page if we are not
		 * doing invalidation and this is an async operation
		 * (the page will be correct when the current io completes).
		 */
VMLOG(X_PVNDIRTY_INTRANS, pp, pp->p_offset, flags);
		return (0);
	}

	/*
	 * XXX - the following code is a hack to try and avoid a deadlock
	 * when someone is trying to lock pages for a nfsd request, but
	 * hasn't gotten them all while this process has gotten the
	 * inode locked for an async putpage call for a system sync
	 * and is about to wait below on the page held by the nfsd.
	 */
	if (flags == B_ASYNC && pp->p_keepcnt > 0)
		return (0);

	/*
	 * If we have to invalidate or free the page,
	 * wait for the page keep count to go to zero.
	 */
	if ((flags & (B_INVAL | B_FREE)) != 0) {
		while (pp->p_keepcnt > 0) {
VMLOG(X_PVNDIRTY_WAIT, pp, *(int *)pp, pp->p_offset);
			page_wait(pp);
		}
	}

VMLOG(X_PVNDIRTY_LOCK, pp, pp->p_vnode, pp->p_offset);
	page_lock(pp);

	if (pp->p_vnode != vp || pp->p_offset != offset) {
		/*
		 * Lost the page - nothing to do?
		 */
VMLOG(X_PVNDIRTY_LOST, pp, pp->p_vnode, vp);
		page_unlock(pp);
		return (0);
	}

	/*
	 * If the page has mappings and it is not the case that the
	 * page is already marked dirty and we are going to unload
	 * the page below because we are going to free/invalidate
	 * it, then we sync current mod bits from the hat layer now.
	 */
	if (pp->p_mapping && !(pp->p_mod && (flags & (B_FREE | B_INVAL)) != 0))
		hat_pagesync(pp);

	if (pp->p_mod == 0) {
VMLOG(X_PVNDIRTY_NOMOD, pp, pp->p_vnode, pp->p_offset);
		if ((flags & (B_INVAL | B_FREE)) != 0) {
			if (pp->p_mapping)
				hat_pageunload(pp);
			if ((flags & B_INVAL) != 0) {
VMLOG(X_PVNDIRTY_INVAL, pp, pp->p_vnode, pp->p_offset);
				page_unlock(pp);
				page_abort(pp);
				return (0);
			}
			if (pp->p_free == 0) {
				if ((flags & B_FREE) != 0) {
VMLOG(X_PVNDIRTY_FREE, pp, pp->p_vnode, pp->p_offset);
					page_free(pp, (flags & B_DONTNEED));
					return (0);
				}
			}
		}
VMLOG(X_PVNDIRTY_NOMOD_RET, pp, pp->p_vnode, pp->p_offset);
		page_unlock(pp);
		return (0);
	}

	/*
	 * Page is dirty, get it ready for the write back
	 * and add page to the dirty list.  First unload
	 * the page if we are going to free/invalidate it.
	 */
	if (pp->p_mapping && (flags & (B_FREE | B_INVAL)) != 0)
		hat_pageunload(pp);
	pp->p_mod = 0;
	pp->p_ref = 0;
        trace3(TR_PG_PVN_GETDIRTY, pp, pp->p_vnode, pp->p_offset);
	pp->p_intrans = 1;
	pp->p_pagein = (flags & B_INVAL) ? 1 : 0;
	PAGE_HOLD(pp);
	page_sortadd(dirty, pp);
VMLOG(X_PVNDIRTY_ADD, pp, pp->p_vnode, pp->p_offset);
	return (1);
}

/*
 * Run down the vplist and handle all pages whose offset is >= off.
 * Returns a list of dirty kept pages all ready to be written back.
 *
 * Assumptions:
 *	The vp is already locked by the VOP_PUTPAGE routine calling this.
 *	That the VOP_GETPAGE also locks the vp, and thus no one can
 *	    add a page to the vp list while the vnode is locked.
 *	Flags are {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED}
 */
page_t *
pvn_vplist_dirty(vp, off, flags)
	register struct vnode *vp;
	u_int off;
	int flags;
{
	register page_t *pp;
	register page_t *ppnext;
	register page_t *ppsav;
	register int ppnext_wasfree, ppsav_wasfree;
	register int ppnext_age, ppsav_age;
	page_t *dirty;

	if (vp->v_type == VCHR || (pp = vp->v_pages) == NULL)
		return ((page_t *)NULL);

#define	PAGE_KEEP(pp, wasfree, age) \
{ \
	if ((pp)->p_free) { \
		age = (pp)->p_age; \
		page_reclaim(pp); \
		wasfree = 1; \
	} else { \
		age = wasfree = 0; \
	} \
	PAGE_HOLD(pp); \
}
#define	PAGE_UNKEEP(pp, wasfree, age) \
{ \
	PAGE_RELE(pp); \
	if (wasfree && (pp)->p_keepcnt == 0 && (pp)->p_mapping == NULL) \
		page_free(pp, age); \
}

	/*
	 * Traverse the page list.  We have to be careful since pages
	 * can be removed from the vplist while we are looking at it
	 * (a page being pulled off the free list for something else,
	 * or an async io operation completing and the page and/or
	 * bp is marked for invalidation) so have to be careful determining
	 * that we have examined all the pages.  We use ppsav to point
	 * to the first page that stayed on the vp list after calling
	 * pvn_getdirty and we PAGE_KEEP it to prevent it from going away
	 * on us.  When we PAGE_UNKEEP the page, it will go back to
	 * the free list if that's where we got it from.  We also need
	 * to PAGE_KEEP the next pp in the vplist to prevent it from
	 * going away while we are traversing the list.
	 */

	ppnext = NULL;
	ppnext_age = ppnext_wasfree = 0;

	ppsav = NULL;
	ppsav_age = ppsav_wasfree = 0;

	dirty = NULL;

	do {
		if (ppnext != NULL) {
VMLOG(X_VPLIST_UNKEEP_NEXTL, pp, ppnext, ppnext_wasfree);
			PAGE_UNKEEP(ppnext, ppnext_wasfree, ppnext_age);
		}

		if (pp->p_vpnext != pp) {
			ppnext = pp->p_vpnext;
			PAGE_KEEP(ppnext, ppnext_wasfree, ppnext_age);
VMLOG(X_VPLIST_KEEP_NEXT, pp, ppnext, ppnext_wasfree);
		} else {
VMLOG(X_VPLIST_NEXT_NULL, pp, ppnext, ppnext_wasfree);
			ppnext = NULL;
		}

		if (pp->p_offset >= off) {
VMLOG(X_VPLIST_GETDIRTY, pp, pp->p_offset, flags);
			(void) pvn_getdirty(pp, &dirty, flags);
		}

		if (ppsav == NULL && vp->v_pages == pp) {
			/*
			 * If we haven't found a marker before and this pp
			 * is still on the vplist, use it as our marker.
			 */
			ppsav = pp;
			PAGE_KEEP(ppsav, ppsav_wasfree, ppsav_age);
VMLOG(X_VPLIST_KEEPSAV, ppsav, ppsav->p_offset, ppsav_wasfree);
		}
	} while (vp->v_pages != NULL && (pp = ppnext) != NULL && pp != ppsav);

	if (ppnext != NULL) {
VMLOG(X_VPLIST_UNKEEP_NEXT, ppnext, ppnext->p_offset, ppnext_wasfree);
		PAGE_UNKEEP(ppnext, ppnext_wasfree, ppnext_age);
	}
	if (ppsav != NULL) {
VMLOG(X_VPLIST_UNKEEP_SAV, ppsav, ppsav->p_offset, ppsav_wasfree);
		PAGE_UNKEEP(ppsav, ppsav_wasfree, ppsav_age);
	}

	return (dirty);
}

/*
 * Use page_find's and handle all pages for this vnode whose offset
 * is >= off and < eoff.  This routine will also do klustering up
 * to offlo and offhi up until a page which is not found.  We assume
 * that offlo <= off and offhi >= eoff.
 *
 * Returns a list of dirty kept pages all ready to be written back.
 */
page_t *
pvn_range_dirty(vp, off, eoff, offlo, offhi, flags)
	register struct vnode *vp;
	u_int off, eoff;
	u_int offlo, offhi;
	int flags;
{
	page_t *dirty = NULL;
	register page_t *pp;
	register u_int o;

	ASSERT(offlo <= off && offhi >= eoff);

	off &= PAGEMASK;
	eoff = (eoff + PAGEOFFSET) & PAGEMASK;

	/* first do all the pages from [off..eoff) */
	for (o = off; o < eoff; o += PAGESIZE) {
		pp = page_find(vp, o);
		if (pp != NULL) {
VMLOG(X_RANGE_GETDIRTY, pp, pp->p_offset, flags);
			(void) pvn_getdirty(pp, &dirty, flags);
		}
	}

	if (pvn_range_noklust)
		return (dirty);

	/* now scan backwards looking for pages to kluster */
	for (o = off - PAGESIZE; (int)o >= 0 && o >= offlo; o -= PAGESIZE) {
		pp = page_find(vp, o);
		if (pp == NULL)
			break;		/* page not found */
		if (pvn_getdirty(pp, &dirty, flags | B_DELWRI) == 0)
			break;		/* page not added to dirty list */
	}

	/* now scan forwards looking for pages to kluster */
	for (o = eoff; o < offhi; o += PAGESIZE) {
		pp = page_find(vp, o);
		if (pp == NULL)
			break;		/* page not found */
		if (pvn_getdirty(pp, &dirty, flags | B_DELWRI) == 0)
			break;		/* page not added to dirty list */
	}

	return (dirty);
}

/*
 * Take care of invalidating all the pages for vnode vp going to size
 * vplen.  This includes zero'ing out zbytes worth of file beyond vplen.
 * This routine should only be called with the vp locked by the file
 * system code so that more pages cannot be added when sleep here.
 */
void
pvn_vptrunc(vp, vplen, zbytes)
	register struct vnode *vp;
	register u_int vplen;
	u_int zbytes;
{
	register page_t *pp;

	if (vp->v_pages == NULL || vp->v_type == VCHR)
		return;

	/*
	 * Simple case - abort all the pages on the vnode
	 */
	if (vplen == 0) {
		while ((pp = vp->v_pages) != (page_t *)NULL) {
			/*
			 * When aborting these pages, we make sure that
			 * we wait to make sure they are really gone.
			 */
			page_lock(pp);
			while (pp->p_keepcnt > 0)
				page_wait(pp);
			if (pp->p_vnode == vp)
				page_abort(pp);
			else
				page_unlock(pp);
		}
		return;
	}

	/*
	 * Tougher case - have to find all the pages on the
	 * vnode which need to be aborted or partially zeroed.
	 */

	/*
	 * First we get the last page and handle the partially
	 * zeroing via kernel mappings.  This will make the page
	 * dirty so that we know that when this page is written
	 * back, the zeroed information will go out with it.  If
	 * the page is not currently in memory, then the kzero
	 * operation will cause it to be brought it.  We use kzero
	 * instead of bzero so that if the page cannot be read in
	 * for any reason, the system will not panic.  We need
	 * to zero out a minimum of the fs given zbytes, but we
	 * might also have to do more to get the entire last page.
	 */
	if (zbytes != 0) {
		addr_t addr;

		if ((zbytes + (vplen & MAXBOFFSET)) > MAXBSIZE)
			cmn_err(CE_PANIC, "pvn_vptrunc zbytes");
		addr = segmap_getmap(segkmap, vp, vplen & MAXBMASK);
		(void) kzero(addr + (vplen & MAXBOFFSET),
		    MAX(zbytes, PAGESIZE - (vplen & PAGEOFFSET)));
		(void) segmap_release(segkmap, addr, SM_WRITE | SM_ASYNC);
	}

	/*
	 * Synchronously abort all pages on the vp list which are
	 * beyond the new length.  The algorithm here is to start
	 * scanning at the beginning of the vplist until there
	 * are no pages with an offset >= vplen.  If we find such
	 * a page, we wait for it if it is kept for any reason and
	 * then we abort it after verifying that it is still a page
	 * that needs to go away.  We assume here that the vplist
	 * is not messed with at interrupt level.
	 */
again:
	for (pp = vp->v_pages; pp != NULL; pp = pp->p_vpnext) {
		if (pp->p_offset >= vplen) {
			/* need to abort this page */
			page_lock(pp);
			while (pp->p_keepcnt > 0)
				page_wait(pp);
			/* verify page contents again */
			if (pp->p_vnode == vp && pp->p_offset >= vplen)
				page_abort(pp);
			else
				page_unlock(pp);
			goto again;		/* start over again */
		}
		if (pp == pp->p_vpnext || vp->v_pages == pp->p_vpnext)
			break;
	}
}

/*
 * This routine is called when the low level address translation
 * code decides to unload a translation.  It calls back to the
 * segment driver which in many cases ends up here.
 */
/*ARGSUSED*/
void
pvn_unloadmap(vp, offset, ref, mod)
	struct vnode *vp;
	u_int offset;
	u_int ref, mod;
{

	/*
	 * XXX - what is the pvn code going to do w/ this information?
	 * This guy gets called for each loaded page when a executable
	 * using the segvn driver terminates...
	 */
}

/*
 * Handles common work of the VOP_GETPAGE routines when the more than
 * one page must be returned by calling a file system specific operation
 * to do most of the work.  Must be called with the vp already locked
 * by the VOP_GETPAGE routine.
 */
int
pvn_getpages(getapage, vp, off, len, protp, pl, plsz, seg, addr, rw, cred)
	int (*getapage)();
	struct vnode *vp;
	u_int off, len;
	u_int *protp;
	page_t *pl[];
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct cred *cred;
{
	register page_t **tpl, **ppp;
	register int i;
	page_t *pl_array[PVN_GETPAGE_NUM + 1];
	int plsz_extra;
	int err = 0;

	if (pl != NULL) {
		tpl = pl_array;
		ppp = pl;
		plsz_extra = plsz - len;
		if (plsz_extra < 0)
			cmn_err(CE_PANIC, "pvn_getpage len");
	} else {
		tpl = NULL;
	}

	for (i = 0; i < len; i += PAGESIZE) {
		err = (*getapage)(vp, off + i, protp, tpl, PVN_GETPAGE_SZ,
		    seg, addr + i, rw, cred);
		if (err)
			break;
		if (tpl != NULL) {
			for (; *tpl != NULL && (int)plsz > 0; tpl++) {
				register u_int toff = (*tpl)->p_offset;

				if (toff >= off && toff < off + len) {
					/*
					 * Copy this page back to
					 * primary page list.
					 */
					*ppp++ = *tpl;
					plsz -= PAGESIZE;
					/*
					 * See if we should skip ahead for next
					 * for loop because we got more more
					 * than just the page we asked for.
					 *
					 * N.B.  This code assumes that if the
					 * getapage function returns pages
					 * beyond the one requested that there
					 * will be no missing pages between the
					 * requested page and the highest page.
					 */
					if (toff > off + i)
						i = toff - off;
				} else if (plsz_extra > 0) {
					/*
					 * Not part of the range we really
					 * needed, but we can fit it in.
					 */
					*ppp++ = *tpl;
					plsz -= PAGESIZE;
					plsz_extra -= PAGESIZE;
				} else {
					/*
					 * We don't have room for it.
					 */
					PAGE_RELE(*tpl);
				}
			}
			while (*tpl != NULL)
				PAGE_RELE(*tpl++);
			tpl = pl_array;
		}
	}

	if (pl != NULL) {
		*ppp = NULL;		/* terminate list */
		if (err) {
			for (ppp = pl; *ppp != NULL; *ppp++ = NULL)
				PAGE_RELE(*ppp);
		}
	}

	return (err);
}

#ifdef VMDEBUG
#include "vm/kernel.h"

/* XXX -- where is this defined ? */
extern struct vnode *rootvp;

#define	NVM_LOG	5000

struct vm_log {
	time_t vm_time;
	int vm_what;
	struct proc *vm_p;
	int vm_a;
	int vm_b;
	int vm_c;
};

struct vm_log *vm_log;
struct vm_log *vmp;
int do_vmlog = 0;
int nvm_log = NVM_LOG;
int vm_min = 0;
int vm_max = 999;

void
_vmlog(what, a, b, c)
{
	extern struct proc *curproc;

	if (do_vmlog && vm_log == (struct vm_log *)NULL) {
		if (rootvp == (struct vnode *)NULL)
			return;		/* too soon */

		do_vmlog = 0;
		vm_log = (struct vm_log *)kmem_zalloc((u_int)
		   (nvm_log * sizeof (*vm_log)), KM_SLEEP);
		vmp = vm_log;
		do_vmlog = 1;
	}
	if (what >= vm_min && what < vm_max) {
		vmp->vm_time = hrestime.tv_sec;
		vmp->vm_what = what;
		vmp->vm_p = curproc;
		vmp->vm_a = a;
		vmp->vm_b = b;
		vmp->vm_c = c;
		if (++vmp >= &vm_log[nvm_log])
			vmp = vm_log;
	}
}
#endif /* VMDEBUG */
