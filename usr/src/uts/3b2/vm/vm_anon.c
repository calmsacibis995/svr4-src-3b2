/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:vm/vm_anon.c	1.16"

/*
 * VM - anonymous pages.
 *
 * This layer sits immediately above the vm_swap layer.  It manages
 * physical pages that have no permanent identity in the file system
 * name space, using the services of the vm_swap layer to allocate
 * backing storage for these pages.  Since these pages have no external
 * identity, they are discarded when the last reference is removed.
 *
 * An important function of this layer is to manage low-level sharing
 * of pages that are logically distinct but that happen to be
 * physically identical (e.g., the corresponding pages of the processes
 * resulting from a fork before one process or the other changes their
 * contents).  This pseudo-sharing is present only as an optimization
 * and is not to be confused with true sharing in which multiple
 * address spaces deliberately contain references to the same object;
 * such sharing is managed at a higher level.
 *
 * The key data structure here is the anon struct, which contains a
 * reference count for its associated physical page and a hint about
 * the identity of that page.  Anon structs typically live in arrays,
 * with an instance's position in its array determining where the
 * corresponding backing storage is allocated; however, the swap_xlate()
 * routine abstracts away this representation information so that the
 * rest of the anon layer need not know it.  (See the swap layer for
 * more details on anon struct layout.)
 *
 * In the future versions of the system, the association between an
 * anon struct and its position on backing store will change so that
 * we don't require backing store all anonymous pages in the system.
 * This is important for consideration for large memory systems.
 * We can also use this technique to delay binding physical locations
 * to anonymous pages until pageout/swapout time where we can make
 * smarter allocation decisions to improve anonymous klustering.
 *
 * Many of the routines defined here take a (struct anon **) argument,
 * which allows the code at this level to manage anon pages directly,
 * so that callers can regard anon structs as opaque objects and not be
 * concerned with assigning or inspecting their contents.
 *
 * Clients of this layer refer to anon pages indirectly.  That is, they
 * maintain arrays of pointers to anon structs rather than maintaining
 * anon structs themselves.  The (struct anon **) arguments mentioned
 * above are pointers to entries in these arrays.  It is these arrays
 * that capture the mapping between offsets within a given segment and
 * the corresponding anonymous backing storage address.
 */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/mman.h"
#include "sys/time.h"
#include "sys/cred.h"
#include "sys/vnode.h"
#include "sys/vmmeter.h"
#include "sys/swap.h"
#include "sys/tuneable.h"
#include "sys/cmn_err.h"
#include "sys/debug.h"

#include "sys/proc.h"		/* XXX - needed for PREEMPT() */
#include "sys/disp.h"		/* XXX - needed for PREEMPT() */

#include "vm/hat.h"
#include "vm/anon.h"
#include "vm/as.h"
#include "vm/page.h"
#include "vm/seg.h"
#include "vm/pvn.h"
#include "vm/rm.h"
#include "vm/mp.h"
#include "vm/trace.h"

#include "vm/vmlog.h"			/* XXX */

extern int	availsmem;
extern void	pagecopy();
extern void	pagezero();

struct	anoninfo anoninfo;

STATIC mon_t anon_lock;
STATIC int anon_resv_debug = 0;
STATIC int anon_enforce_resv = 1;
STATIC int npagesteal;

/*
 * Reserve anon space.
 * Return non-zero on success.
 */
int
anon_resv(size)
	u_int size;
{
	register u_int pages = btopr(size);

	if (availsmem - pages < tune.t_minasmem) {
		nomemmsg("anon_resv", pages, 0, 0);
		return 0;
	}

	anoninfo.ani_resv += pages;
	if (anoninfo.ani_resv > anoninfo.ani_max) {
		if (anon_enforce_resv)
			anoninfo.ani_resv -= pages;
		else if (anon_resv_debug)
			cmn_err(CE_CONT, "anon: swap space overcommitted by %d\n",
			    anoninfo.ani_resv - anoninfo.ani_max);
		return (!anon_enforce_resv);
	} 
	availsmem -= pages;
	return (1);
}

/*
 * Give back an anon reservation.
 */
void
anon_unresv(size)
	u_int size;
{
	register u_int pages = btopr(size);

	availsmem += pages;
	anoninfo.ani_resv -= pages;
	if ((int)anoninfo.ani_resv < 0)
		cmn_err(CE_WARN, "anon: reservations below zero???\n");
}

/*
 * Allocate an anon slot.
 */
struct anon *
anon_alloc()
{
	register struct anon *ap;

	mon_enter(&anon_lock);
	ap = swap_alloc();
	if (ap != NULL) {
		anoninfo.ani_free--;
		ap->an_refcnt = 1;
		ap->un.an_page = NULL;
#ifdef DEBUG
		ASSERT(ap->an_use == AN_NONE);
		ap->an_use = AN_DATA;
#endif
	}
	mon_exit(&anon_lock);
	return (ap);
}

#ifdef DEBUG
struct anon *
anon_upalloc()
{
	register struct anon *ap;

	mon_enter(&anon_lock);
	ap = swap_alloc();
	if (ap != NULL) {
		anoninfo.ani_free--;
		ap->an_refcnt = 1;
		ap->un.an_page = NULL;
		ASSERT(ap->an_use == AN_NONE);
		ap->an_use = AN_UPAGE;
	}
	mon_exit(&anon_lock);
	return (ap);
}
#endif

/*
 * Decrement the reference count of an anon page.
 * If reference count goes to zero, free it and
 * its associated page (if any).
 */
STATIC void
anon_decref(ap)
	register struct anon *ap;
{
	register page_t *pp;
	struct vnode *vp;
	u_int off;

	mon_enter(&anon_lock);

	ASSERT(ap->an_refcnt > 0);
#ifdef DEBUG
	ASSERT(ap->an_use == AN_DATA);
#endif
	if (ap->an_refcnt == 1) {
		/*
		 * If there is a page for this anon slot we will need to
		 * call page_abort to get rid of the vp association and
		 * put the page back on the free list as really free.
		 */
		swap_xlate(ap, &vp, &off);
		ap->an_refcnt--;
		pp = page_find(vp, off);
VMLOG(X_ANONDECREF_FREE, ap, off, pp);
#ifdef DEBUG
		ap->an_use = AN_NONE;
#endif
		swap_free(ap);
		anoninfo.ani_free++;
	} else {
		ap->an_refcnt--;
		pp = NULL;
	}
VMLOG(X_ANONDECREF, ap, off, ap->an_refcnt);

	mon_exit(&anon_lock);

	/*
	 * If we had a page, now we can do the page_abort
	 * since the anon_lock has been released.
	 */
	if (pp != NULL) {
VMLOG(X_ANONDECREF_ABORT, ap, off, pp);
		page_abort(pp);
	}
}

#ifdef DEBUG
void
anon_updecref(ap)
	register struct anon *ap;
{
	register page_t *pp;
	struct vnode *vp;
	u_int off;

	mon_enter(&anon_lock);

	ASSERT(ap->an_use == AN_UPAGE);
	ASSERT(ap->an_refcnt == 1);
	if (ap->an_refcnt == 1) {
		/*
		 * If there is a page for this anon slot we will need to
		 * call page_abort to get rid of the vp association and
		 * put the page back on the free list as really free.
		 */
		swap_xlate(ap, &vp, &off);
		ap->an_refcnt--;
		pp = page_find(vp, off);
VMLOG(X_ANONDECREF_FREE, ap, off, pp);
		ap->an_use = AN_NONE;
		swap_free(ap);
		anoninfo.ani_free++;
	} else {
		ap->an_refcnt--;
		pp = NULL;
	}
VMLOG(X_ANONDECREF, ap, off, ap->an_refcnt);

	mon_exit(&anon_lock);

	/*
	 * If we had a page, now we can do the page_abort
	 * since the anon_lock has been released.
	 */
	if (pp != NULL) {
VMLOG(X_ANONDECREF_ABORT, ap, off, pp);
		page_abort(pp);
	}
}
#endif

/*
 * Duplicate references to size bytes worth of anon pages.
 * Used when duplicating a segment that contains private anon pages.
 * This code assumes that procedure calling this one has already used
 * hat_chgprot() to disable write access to the range of addresses that
 * that *old actually refers to.
 */
void
anon_dup(old, new, size)
	register struct anon **old, **new;
	u_int size;
{
	register int i;

	i = btopr(size);
	while (i-- > 0) {
		if ((*new = *old) != NULL) {
			(*new)->an_refcnt++;
#ifdef DEBUG
			ASSERT((*new)->an_use == AN_DATA);
#endif
		}
		old++;
		new++;
	}
}

/*
 * Free a group of "size" anon pages, size in bytes.
 */
void
anon_free(app, size)
	register struct anon **app;
	u_int size;
{
	register int i;

	i = btopr(size);
	while (i-- > 0) {
		if (*app)
			anon_decref(*app);
		app++;
		/* 
		 * This loop takes a while so we put in a preemption point
		 * here. We preempt only when the current process is not
		 * in zombie state. (This function is also called when a 
		 * process's state is set to zombie)
		 */
		if (curproc->p_stat != SZOMB)
                        PREEMPT();
	}
}

#ifdef DEBUG
void
anon_upfree(app, size)
	register struct anon **app;
	u_int size;
{
	register int i;

	i = btopr(size);
	while (i-- > 0) {
		if (*app)
			anon_updecref(*app);
		app++;
	}
}
#endif

/*
 * Return the kept page(s) and protections back to the segment driver.
 */
int
anon_getpage(app, protp, pl, plsz, seg, addr, rw, cred)
	struct anon **app;
	u_int *protp;
	page_t *pl[];
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct cred *cred;
{
	register page_t *pp, **ppp;
	register struct anon *ap = *app;
	struct vnode *vp;
	u_int off;
	int err;
	extern int nopagereclaim;

VMLOG(X_ANONGETPAGE, seg, addr, app);
	swap_xlate(ap, &vp, &off);
again:
	pp = ap->un.an_page;
	/*
	 * If the anon pointer has a page associated with it,
	 * see if it looks ok.  If page is being page in,
	 * wait for it to finish as we must return a list of
	 * pages since this routine acts like the VOP_GETPAGE
	 * routine does.
	 */
	if (pp != NULL && pp->p_vnode == vp && pp->p_offset == off &&
	    !pp->p_gone) {
		if (pp->p_intrans && (pp->p_pagein || nopagereclaim)) {
VMLOG(X_ANONGETPAGE_GOTIT_WAIT, pp, pp->p_vnode, pp->p_offset);
			page_wait(pp);
			goto again;		/* try again */
		}
		if (pp->p_free)
			page_reclaim(pp);
		PAGE_HOLD(pp);
		if (ap->an_refcnt == 1)
			*protp = PROT_ALL;
		else
			*protp = PROT_ALL & ~PROT_WRITE;
		pl[0] = pp;
		pl[1] = NULL;
		err = 0;
VMLOG(X_ANONGETPAGE_GOTIT, pp, seg, addr);
	} else {
		/*
		 * Simply treat it as a vnode fault on the anon vp.
		 */
		trace3(TR_SEG_GETPAGE, seg, addr, TRC_SEG_ANON);
		err = VOP_GETPAGE(vp, off, PAGESIZE, protp, pl, plsz,
		    seg, addr, rw, cred);
		if (err == 0) {
			for (ppp = pl; (pp = *ppp++) != NULL; ) {
				if (pp->p_offset == off) {
					ap->un.an_page = pp;
					break;
				}
			}
		}
VMLOG(X_ANONGETPAGE_VOP, pp, vp, off);
		if (ap->an_refcnt != 1)
			*protp &= ~PROT_WRITE;	/* make read-only */
	}
	return (err);
}

/*
 * Turn a reference to a shared anon page into a private
 * page with a copy of the data from the original page.
 */
page_t *
anon_private(app, seg, addr, opp, steal)
	struct anon **app;
	struct seg *seg;
	addr_t addr;
	page_t *opp;
	int steal;
{
	register struct anon *old = *app;
	register struct anon *new;
	register page_t *pp;
	struct vnode *vp;
	u_int off;

VMLOG(X_ANONPRIVATE, seg, addr, app);

	new = anon_alloc();
	if (new == (struct anon *)NULL) {
		rm_outofanon();
		return ((page_t *)NULL);	/* out of swap space */
	}

	swap_xlate(new, &vp, &off);
again:
	pp = page_lookup(vp, off);

/*
 * Because of a 386 hw bug, the page that we would be copying from
 * (opp) has had an additional PAGE_HOLD applied to it by
 * the time we get here.
 * XXX - but the translation is also locked for the 386 in this case.
 */
#ifdef sun386
#define	STEAL_KEEP	2
#else
#define	STEAL_KEEP	1
#endif

	if (pp == NULL && steal != 0 && old == NULL
	  && opp->p_mod == 0 && opp->p_keepcnt == STEAL_KEEP) {
		pp = opp;
		hat_pageunload(pp);		/* XXX - not kosher for 386 */
		page_hashout(pp);		/* destroy old name for page */
		trace6(TR_SEG_ALLOCPAGE, seg, addr, TRC_SEG_ANON, vp, off, pp);
		if (page_enter(pp, vp, off))	/* rename as anon page */
			cmn_err(CE_PANIC, "anon private steal");
		new->un.an_page = pp;
		*app = new;
		pp->p_mod = 1;
		PAGE_HOLD(pp);
		page_unlock(pp);
		npagesteal++;
		return (pp);
	}

	if (pp == NULL) {
		/*
		 * Normal case, need to allocate new page frame.
		 */
		pp = rm_allocpage(seg, addr, PAGESIZE, P_CANWAIT);
		trace6(TR_SEG_ALLOCPAGE, seg, addr, TRC_SEG_ANON, vp, off, pp);
		if (page_enter(pp, vp, off)) {
			page_abort(pp);
			goto again;		/* try again */
		}
VMLOG(X_ANONPRIVATE_ALLOC, pp, vp, off);
	} else {
		/*
		 * Already found a page with the right identity - just use it.
		 */
		page_lock(pp);
		PAGE_HOLD(pp);
	}
	new->un.an_page = pp;

	/*
	 * To make this work we have to have *app still pointing at
	 * the original anon structure here for anon_getpage()!
	 */
	pp->p_intrans = pp->p_pagein = 1;

	/*
	 * If we have the original page (which has been held), copy
	 * directly from it.  Otherwise copy directly from the segment
	 * address, which--XXX--is assumed to be mapped in.
	 */
	if (opp)
		ppcopy(opp, pp);
	else
		pagecopy(addr, pp);

	pp->p_intrans = pp->p_pagein = 0;

	/*
	 * Ok, now we can unload the old translation info
	 */
	*app = new;
	hat_unload(seg, addr, PAGESIZE, HAT_NOFLAGS);

	pp->p_mod = 1;				/* mark as modified */
	page_unlock(pp);

	/*
	 * If we copied away from an anonymous page, then
	 * we are one step closer to freeing up an anon slot.
	 */
	if (old != NULL)
		anon_decref(old);
	return (pp);
}

/*
 * Allocate a private zero-filled anon page.
 */

/* ARGSUSED */
page_t *
anon_zero(seg, addr, app)
	struct seg *seg;
	addr_t addr;
	struct anon **app;
{
	register struct anon *ap;
	register page_t *pp;
	struct vnode *vp;
	u_int off;

VMLOG(X_ANONZERO, seg, addr, app);
	*app = ap = anon_alloc();
	if (ap == NULL) {
		rm_outofanon();
		return ((page_t *)NULL);
	}

	swap_xlate(ap, &vp, &off);
again:
	pp = page_lookup(vp, off);

	if (pp == NULL) {
		/*
		 * Normal case, need to allocate new page frame.
		 */
		pp = rm_allocpage(seg, addr, PAGESIZE, P_CANWAIT);
		trace6(TR_SEG_ALLOCPAGE, seg, addr, TRC_SEG_ANON, vp, off, pp);
		if (page_enter(pp, vp, off)) {
			page_abort(pp);
			goto again;		/* try again */
		}
VMLOG(X_ANONZERO_ALLOC, pp, vp, off);
	} else {
		/*
		 * Already found a page with the right identity - just use it.
		 */
		page_lock(pp);
		PAGE_HOLD(pp);
	}
	ap->un.an_page = pp;

	pagezero(pp, 0, PAGESIZE);
	cnt.v_zfod++;
	pp->p_mod = 1;		/* mark as modified so pageout writes back */
	page_unlock(pp);
	return (pp);
}

/*
 * This gets calls by the seg_vn driver unload routine
 * which is called by the hat code when it decides to
 * unload a particular mapping.
 */
void
anon_unloadmap(ap, ref, mod)
	struct anon *ap;
	u_int ref, mod;
{
	struct vnode *vp;
	u_int off;

	swap_xlate(ap, &vp, &off);
	pvn_unloadmap(vp, off, ref, mod);
}
