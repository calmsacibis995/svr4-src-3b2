/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:vm/vm_page.c	1.40"

/*
 * VM - physical page management.
 */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/buf.h"
#include "sys/errno.h"
#include "sys/time.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/immu.h"		/* XXX - needed for outofmem() */
#include "sys/proc.h"		/* XXX - needed for outofmem() */
#include "sys/vm.h"
#include "vm/trace.h"
#include "sys/swap.h"
#include "sys/debug.h"
#include "sys/cmn_err.h"
#include "sys/tuneable.h"
#include "sys/sysmacros.h"
#include "sys/sysinfo.h"
#include "sys/inline.h"
#include "sys/disp.h"		/* XXX - needed for PREEMPT() */

#include "vm/hat.h"
#include "vm/anon.h"
#include "vm/page.h"
#include "vm/seg.h"
#include "vm/pvn.h"
#include "vm/mp.h"
#include "vm/vmlog.h"

#if defined(__STDC__)
STATIC void page_add(page_t **, page_t *);
STATIC int page_addmem(int);
STATIC int page_delmem(int);
STATIC void page_print(page_t *);
STATIC void page_unfree(page_t *);
#else
STATIC void page_add();
STATIC int page_addmem();
STATIC int page_delmem();
STATIC void page_print();
STATIC void page_unfree();
#endif

STATIC int nopageage = 1;

STATIC u_int max_page_get;	/* max page_get request size in pages */
STATIC u_int freemem_wait;		/* someone waiting for freemem */

u_int pages_pp_locked = 0;	/* physical pages actually locked */
u_int pages_pp_claimed = 0;		/* physical pages reserved */
u_int pages_pp_kernel = 0;		/* physical page locks by kernel */

extern void	cleanup();
void	call_debug();

/* XXX where should this be defined ? */
struct buf *bclnlist;
int availrmem;
int availsmem;

#ifdef PAGE_DEBUG
int do_checks = 0;
int do_check_vp = 1;
int do_check_free = 1;
int do_check_list = 1;
int do_check_pp = 1;

STATIC void page_vp_check();
STATIC void page_free_check();
STATIC void page_list_check();
STATIC void page_pp_check();

#define	CHECK(vp)	if (do_checks && do_check_vp) page_vp_check(vp)
#define	CHECKFREE()	if (do_checks && do_check_free) page_free_check()
#define	CHECKLIST(pp)	if (do_checks && do_check_list) page_list_check(pp)
#define	CHECKPP(pp)	if (do_checks && do_check_pp) page_pp_check(pp)

#else /* PAGE_DEBUG */

#define	CHECK(vp)
#define	CHECKFREE()
#define	CHECKLIST(pp)
#define	CHECKPP(pp)

#endif /* PAGE_DEBUG */


/*
 * Set to non-zero to avoid reclaiming pages which are
 * busy being paged back until the IO and completed.
 */
int nopagereclaim = 0;

/*
 * The logical page free list is maintained as two physical lists.
 * The free list contains those pages that should be reused first.
 * The cache list contains those pages that should remain unused as
 * long as possible so that they might be reclaimed.
 */
STATIC page_t *page_freelist;		/* free list of pages */
STATIC page_t *page_cachelist;		/* cache list of free pages */
STATIC int page_freelist_size;		/* size of free list */
STATIC int page_cachelist_size;		/* size of cache list */

mon_t	page_mplock;			/* lock for manipulating page links */

STATIC	mon_t	page_freelock;		/* lock for manipulating free list */

page_t *pages;			/* array of all page structures */
page_t *epages;			/* end of all pages */
u_int	pages_base;			/* page # for pages[0] */
u_int	pages_end;			/* page # for pages[max] */

#ifdef	sun386
page_t *epages2;			/* end of all pages */
u_int	pages_base2;			/* page # for discontiguous phys mem */
u_int	pages_end2;			/* page # for end */

#define OK_PAGE_GET 0x50		/* min ok value for MAX_PAGE_GET */
#define KEEP_FREE 0x20			/* number of pages to keep free */

#endif /* sun386 */


STATIC	mon_t page_locklock;	/* mutex on locking variables */

#if 0
STATIC	u_int pages_pp_factor = 10;/* divisor for unlocked percentage */
#endif

#define	PAGE_LOCK_MAXIMUM \
	((1 << (sizeof (((page_t *)0)->p_lckcnt) * NBBY)) - 1)

STATIC struct page_tcnt {
	int	pc_free_cache;		/* free's into cache list */
	int	pc_free_dontneed;	/* free's with dontneed */
	int	pc_free_pageout;	/* free's from pageout */
	int	pc_free_free;		/* free's into free list */
	int	pc_get_cache;		/* get's from cache list */
	int	pc_get_free;		/* get's from free list */
	int	pc_reclaim;		/* reclaim's */
	int	pc_abortfree;		/* abort's of free pages */
	int	pc_find_hit;		/* find's that find page */
	int	pc_find_miss;		/* find's that don't find page */
#define	PC_HASH_CNT	(2*PAGE_HASHAVELEN)
	int	pc_find_hashlen[PC_HASH_CNT+1];
} pagecnt;

/*
 * Initialize the physical page structures.
 * Since we cannot call the dynamic memory allocator yet,
 * we have startup() allocate memory for the page
 * structs and the hash tables for us.
 */
#ifdef	sun386
void
page_init(pp, num, base, num2, base2)
	register page_t *pp;
	u_int num, base;
	u_int num2, base2;
#else
void
page_init(pp, num, base)
	register page_t *pp;
	u_int num, base;
#endif
{

	/*
	 * Store away info in globals.  In the future, we will want to
	 * redo this stuff so that we can have multiple chunks.
	 */
	pages = pp;
	epages = &pp[num];
	pages_base = base;
	pages_end = base + num;

#ifdef	sun386
	epages2 = &pp[num+num2];
	pages_base2 = base2;
	pages_end2 = base2 + num2;
#endif
	/*
	 * Arbitrarily limit the max page_get request
	 * to 1/2 of the page structs we have.
	 *
	 * If this value is < OK_PAGE_GET, then we set max_page_get to
	 * num - KEEP_FREE.  If this number is less 1/2 of memory,
	 * use 1/2 of mem.  If it's greater than OK_PAGE_GET, use OK_PAGE_GET.
	 *
	 * All of this is just an attempt to run even if very little memory
	 * is available.  There are no guarantees!  The system will probably
	 * die later with insufficient memory even though we get by here.
	 */
#ifdef	sun386
	max_page_get = (num + num2) >> 1;

	if (max_page_get < OK_PAGE_GET && num - KEEP_FREE > max_page_get) {
		max_page_get = num - KEEP_FREE;
		if (max_page_get > OK_PAGE_GET)
			max_page_get = OK_PAGE_GET;
	}
#else
	max_page_get = num >> 1;
#endif

	/*
	 * The physical space for the pages array
	 * representing ram pages have already been
	 * allocated.  Here we mark all the pages as
	 * locked.  Later calls to page_free() will
	 * make the pages available.
	 */
#ifdef	sun386
	for (; pp < epages2; pp++)
		pp->p_lock = 1;
#else
	for (; pp < epages; pp++)
		pp->p_lock = 1;
#endif
	/*
	 * Determine the number of pages that can be pplocked.  This
	 * is the number of page frames less the maximum that can be
	 * taken for buffers less another percentage.  The percentage should
	 * be a tunable parameter, and in SVR4 should be one of the "tune"
	 * structures.
	 */
/*
	pages_pp_maximum = 
#ifdef	sun386
	    num + num2;
#else	sun386
	    num;
#endif	sun386
	pages_pp_maximum = num / 10;
*/
	if (pages_pp_maximum <= (tune.t_minarmem + 20) ||
	    pages_pp_maximum > num)
		pages_pp_maximum = num / 10;
#if 0
	pages_pp_maximum -= (btop(nbuf * MAXBSIZE) + 1) +
	    (pages_pp_maximum / pages_pp_factor);
#endif

	/*
	 * Verify that the hashing stuff has been initialized by machdep.c
	 */
	if (page_hash == NULL || page_hashsz == 0)
		cmn_err(CE_PANIC, "page_init");

#ifdef lint
	page_print(pp);
#endif /* lint */
}

/*
 * Use cv_wait() to wait for the given page.  It is assumed that
 * the page_mplock is already locked upon entry and will this lock
 * will continue to be held upon return.
 */
void
page_cv_wait(pp)
	page_t *pp;
{
	int s;

	/*
	 * Protect against someone clearing the
	 * want bit before we get to sleep.
	 */
	s = splvm();
	if (bclnlist == NULL) {
		/* page may be done except for cleanup if
		 * bclnlist != NULL.
		 * during startup, in particular, this
		 * would cause deadlock.
		 * Later, it causes an unnecessary delay
		 * unless that case is handled.
		 */
		pp->p_want = 1;
		cv_wait(&page_mplock, (char *)pp);
	}
	(void) splx(s);

	/*
	 * We may have been awakened from swdone,
	 * in which case we must clean up the i/o
	 * list before being able to use the page.
	 */
	mon_exit(&page_mplock);
	if (bclnlist != NULL)
		cleanup();
	mon_enter(&page_mplock);
}

/*
 * Reclaim the given page from the free list to vp list.
 */
void
page_reclaim(pp)
	register page_t *pp;
{
	int s;
	register struct anon *ap;

	ASSERT (pp >= pages && pp < epages);
	s = splimp();
	mon_enter(&page_freelock);

	if (pp->p_free) {
#ifdef	TRACE
		register int age = pp->p_age;

		ap = NULL;
#endif	/* TRACE */
		page_unfree(pp);
		pagecnt.pc_reclaim++;
		if (pp->p_vnode) {
			cnt.v_pgrec++;
			cnt.v_pgfrec++;
			vminfo.v_pgrec++;

			if (ap = swap_anon(pp->p_vnode, pp->p_offset)) {
				if (ap->un.an_page == NULL && ap->an_refcnt > 0)
					ap->un.an_page = pp;
				cnt.v_xsfrec++;
				vminfo.v_xsfrec++;
			} else {
				cnt.v_xifrec++;
				vminfo.v_xifrec++;
			}
			CHECK(pp->p_vnode);
		}

		trace6(TR_PG_RECLAIM, pp, pp->p_vnode, pp->p_offset,
			ap, age, freemem);
	}

	mon_exit(&page_freelock);
	(void) splx(s);
}

/*
 * Quick page lookup to merely find if a named page exists
 * somewhere w/o having to worry about which list it is on.
 */
page_t *
page_find(vp, off)
	register struct vnode *vp;
	register u_int off;
{
	register page_t *pp;
	register int len = 0;

	for (pp = page_hash[PAGE_HASHFUNC(vp, off)]; pp; pp = pp->p_hash, len++)
		if (pp->p_vnode == vp && pp->p_offset == off && pp->p_gone == 0)
			break;
	if (pp)
		pagecnt.pc_find_hit++;
	else
		pagecnt.pc_find_miss++;
	if (len > PC_HASH_CNT)
		len = PC_HASH_CNT;
	pagecnt.pc_find_hashlen[len]++;
	return (pp);
}

/*
 * Find a page representing the specified <vp, offset>.
 * If we find the page but it is intransit coming in,
 * we wait for the IO to complete and then reclaim the
 * page if it was found on the free list.
 */
page_t *
page_lookup(vp, off)
	struct vnode *vp;
	u_int off;
{
	register page_t *pp;

again:
	pp = page_find(vp, off);
	if (pp != NULL) {
		ASSERT (pp >= pages && pp < epages);
		/*
		 * Try calling cleanup here to reap the
		 * async buffers queued up for processing.
		 */
		if (pp->p_intrans && pp->p_pagein && bclnlist) {
VMLOG(X_PAGELOOKUP_CLEANUP, pp, vp, off);
			cleanup();
		}

		mon_enter(&page_mplock);
		while (pp->p_lock && pp->p_intrans && pp->p_vnode == vp &&
		    pp->p_offset == off && !pp->p_gone &&
		    (pp->p_pagein || nopagereclaim)) {
VMLOG(X_PAGELOOKUP_WAIT, pp, vp, off);
			cnt.v_intrans++;
			page_cv_wait(pp);
		}
VMLOG(X_PAGELOOKUP_WAITDONE, pp, pp->p_vnode, pp->p_offset);
		mon_exit(&page_mplock);

		/*
		 * If we still have the right page and it is now
		 * on the free list, get it back via page_reclaim.
		 * Note that when a page is on the free list, it
		 * maybe ripped away at interrupt level.  After
		 * we reclaim the page, this page cannot not be
		 * taken away from as at interrupt level anymore.
		 */
		if (pp->p_vnode == vp && pp->p_offset == off && !pp->p_gone &&
		    pp->p_free)
			page_reclaim(pp);
		/*
		 * Verify page contents again, if we
		 * lost it go start all over again.
		 */
		if (pp->p_vnode != vp || pp->p_offset != off || pp->p_gone)
			goto again;
	}
	return (pp);
}

/*
 * Enter page ``pp'' in the hash chains and
 * vnode page list as referring to <vp, offset>.
 */
int
page_enter(pp, vp, offset)
	page_t *pp;
	struct vnode *vp;
	u_int offset;
{
	int v;

	mon_enter(&page_mplock);

	if (page_find(vp, offset) != NULL) {
		/* already entered? */
		v = -1;
	} else {
VMLOG(X_PAGEENTER, pp, vp, offset);
		page_hashin(pp, vp, offset, 1);
		CHECK(vp);

		v = 0;
	}

	mon_exit(&page_mplock);

	trace4(TR_PG_ENTER, pp, vp, offset, v);

	return (v);
}

/*
 * page_abort will cause a page to lose its
 * identity and to go (back) to the free list.
 * XXX - redo page locking and transitions,
 * redo page_abort so that we can affort to wait.
 */
void
page_abort(pp)
	register page_t *pp;
{

VMLOG(X_PAGEABORT, pp, pp->p_vnode, pp->p_offset);
	ASSERT (pp >= pages && pp < epages);
	if (pp->p_vnode == NULL) {
		if (pp->p_free)
			return;		/* nothing to do */
	} else
		pp->p_gone = 1;
	/*
	 * Page is set to go away -- kill any logical locking.
	 */
	if (pp->p_lckcnt > 0) {
		mon_enter(&page_locklock);
		pages_pp_locked--;
		availrmem++;
		pp->p_lckcnt = 0;
		mon_exit(&page_locklock);
	}
	if (pp->p_cowcnt > 0) {
		mon_enter(&page_locklock);
		pages_pp_locked -= pp->p_cowcnt;
		availrmem += pp->p_cowcnt;
		pp->p_cowcnt = 0;
		mon_exit(&page_locklock);
	}

	if (pp->p_keepcnt > 0) {
		/*
		 * We cannot do anything with the page now.
		 * Hope that page_free is called later when
		 * the keep count goes back to zero.
		 */
VMLOG(X_PAGEABORT_KEPT, pp, pp->p_vnode, pp->p_offset);
		trace4(TR_PG_ABORT, pp, pp->p_vnode, pp->p_offset, 1);
		return;
	}
	if (pp->p_intrans) {
		/*
		 * Since the page is already `gone', we can
		 * just let pvn_done() worry about freeing
		 * this page later when the IO finishes.
		 */
VMLOG(X_PAGEABORT_INTRANS, pp, pp->p_vnode, pp->p_offset);
		trace4(TR_PG_ABORT, pp, pp->p_vnode, pp->p_offset, 2);
		return;
	}
	if (pp->p_mapping) {
		/*
		 * Should be ok to just unload now
		 */
VMLOG(X_PAGEABORT_UNLOAD, pp, pp->p_vnode, pp->p_offset);
		hat_pageunload(pp);
	}
	pp->p_ref = pp->p_mod = 0;
	trace4(TR_PG_ABORT, pp, pp->p_vnode, pp->p_offset, 0);
	if (pp->p_free) {
VMLOG(X_PAGEABORT_FREE, pp, pp->p_vnode, pp->p_offset);
		/*
		 * Already on free list - pull the page out of the free
		 * list so it can be re-entered using page_free() below.
		 */
		page_unfree(pp);
		pagecnt.pc_abortfree++;
	} 
#ifdef VMDEBUG
	else {
VMLOG(X_PAGEABORT_GONE, pp, pp->p_vnode, pp->p_offset);
	}
#endif
	/*
	 * Let page_free() do the rest of the work
	 */
	page_free(pp, 0);
}

/*
 * Put page on the "free" list.  The free list is really two circular lists
 * with page_freelist and page_cachelist pointers into the middle of the lists.
 */
void
page_free(pp, dontneed)
	register page_t *pp;
	int dontneed;
{
	register struct vnode *vp;
	struct anon *ap;
	int s;

	ASSERT (pp >= pages && pp < epages);
#ifdef DEBUG
	ASSERT(pp->p_uown == NULL);
#endif
	vp = pp->p_vnode;
VMLOG(X_PAGEFREE, pp, vp, pp->p_offset);

	CHECK(vp);

	/*
	 * If we are a swap page, get rid of corresponding
	 * page hint pointer in the anon vector (since it is
	 * easy to do right now) so that we have to find this
	 * page via a page_lookup to force a reclaim.
	 */
	if (ap = swap_anon(pp->p_vnode, pp->p_offset)) {
		if (ap->an_refcnt > 0)
			ap->un.an_page = NULL;
	}

	if (pp->p_gone) {
VMLOG(X_PAGEFREE_GONE, pp, vp, pp->p_offset);
		if (pp->p_intrans || pp->p_keepcnt != 0) {
VMLOG(X_PAGEFREE_INTRANS, pp, vp, pp->p_offset);
			/*
			 * This page will be freed later from pvn_done
			 * (intrans) or the segment unlock routine.
			 * For now, the page will continue to exist,
			 * but with the "gone" bit on.
			 */
			trace6(TR_PG_FREE, pp, vp, pp->p_offset,
				dontneed, freemem, 0);
			return;
		}
VMLOG(X_PAGEFREE_NOTINTRANS, pp, vp, pp->p_offset);
		if (vp)
			page_hashout(pp);
		vp = NULL;
	}

	if (pp->p_keepcnt != 0 || pp->p_mapping != NULL || pp->p_free ||
	    pp->p_lckcnt != 0 || pp->p_cowcnt != 0)
		cmn_err(CE_PANIC, "page_free");

	s = splimp();
	mon_enter(&page_freelock);

	/*
	 * Now we add the page to the head of the free list.
	 * But if this page is associated with a paged vnode
	 * then we adjust the head forward so that the page is
	 * effectively at the end of the list.
	 */
	freemem++;
	pp->p_free = 1;
	pp->p_ref = pp->p_mod = 0;
	if (vp == NULL) {
		/* page has no identity, put it on the front of the free list */
		pp->p_age = 1;
		page_freelist_size++;
		page_add(&page_freelist, pp);
		pagecnt.pc_free_free++;
		trace6(TR_PG_FREE, pp, vp, pp->p_offset, dontneed, freemem, 1);
	} else {
		page_cachelist_size++;
		page_add(&page_cachelist, pp);
		if (!dontneed || nopageage) {
			/* move it to the tail of the list */
			page_cachelist = page_cachelist->p_next;
			pagecnt.pc_free_cache++;
			trace6(TR_PG_FREE, pp, vp, pp->p_offset,
				dontneed, freemem, 2);
		} else {
			pagecnt.pc_free_dontneed++;
			trace6(TR_PG_FREE, pp, vp, pp->p_offset,
				dontneed, freemem, 3);
		}
	}

	mon_exit(&page_freelock);

	CHECK(vp);
	CHECKFREE();

	page_unlock(pp);

	if (freemem_wait) {
VMLOG(X_PAGEFREE_FREEMEM_WAIT, pp, freemem, freemem_wait);
		freemem_wait = 0;
		wakeup((caddr_t)&freemem);
	}
	(void) splx(s);
}

STATIC int free_pages = 1;

void
free_vp_pages(vp, off, len)
	register struct vnode *vp;
	register u_int off;
	u_int len;
{
	extern int swap_in_range();
	register page_t *pp, *epp;
	register u_int eoff;
	int s;

	eoff = off + len;

	if (free_pages == 0)
		return;
	if (swap_in_range(vp, off, len))
		return;
	CHECK(vp);
	/* free_vp_page may take some time so PREEMPT() */
	PREEMPT();
	if ((pp = epp = vp->v_pages) != 0) {
		s = splimp();
		do {
			ASSERT(pp->p_vnode == vp);
			if (pp->p_offset < off || pp->p_offset >= eoff)
				continue;
			ASSERT( !pp->p_intrans || pp->p_keepcnt);
			if (pp->p_mod ) /* XXX somebody needs to handle these */
				continue;
			if (pp->p_keepcnt || pp->p_mapping || pp->p_free ||
				pp->p_lckcnt || pp->p_cowcnt)
				continue;
			ASSERT(pp >= pages && pp < epages);
VMLOG(X_PAGEFREE, pp, vp, pp->p_offset);
			ASSERT(!pp->p_gone);
			mon_enter(&page_freelock);
			freemem++;
			pp->p_free = 1;
			pp->p_ref = 0;
			page_cachelist_size++;
			page_add(&page_cachelist, pp);
			page_cachelist = page_cachelist->p_next;
			pagecnt.pc_free_cache++;
			trace6(TR_PG_FREE, pp, vp, pp->p_offset,
				dontneed, freemem, 3);
			mon_exit(&page_freelock);
			CHECK(vp);
			CHECKFREE();
			page_unlock(pp);
		} while ((pp = pp->p_vpnext) != epp);
		if (freemem_wait && page_cachelist != NULL) {
VMLOG(X_PAGEFREE_FREEMEM_WAIT, pp, freemem, freemem_wait);
			freemem_wait = 0;
			wakeup((caddr_t)&freemem);
		}
		splx(s);
	}
}

/*
 * Remove the page from the free list.
 */
STATIC void
page_unfree(pp)
	register page_t *pp;
{

	ASSERT (pp >= pages && pp < epages);
	if (!pp->p_free)
		cmn_err(CE_PANIC, "page_unfree");
	page_sub(pp->p_age ? &page_freelist : &page_cachelist, pp);
	if (pp->p_age) {
		page_freelist_size--;
	} else {
		page_cachelist_size--;
	}
	pp->p_free = pp->p_age = 0;
	freemem--;
}

/*
 * Allocate enough pages for bytes of data.
 * Return a doubly linked, circular list of pages.
 * Must spl around entire routine to prevent races from
 * pages being allocated at interrupt level.
 */
page_t *
page_get(bytes, flags)
	u_int bytes;
	u_int flags;
{
	register page_t *pp;
	page_t *plist = NULL;
	register int npages;
	register int physcontig;
	int s, i;

	npages = btopr(bytes);
	/*
	 * Try to see whether request is too large to *ever* be
	 * satisfied, in order to prevent deadlock.  We arbitrarily
	 * decide to limit maximum size requests to max_page_get.
	 */
	if (npages >= max_page_get) {
		trace4(TR_PAGE_GET, bytes, flags, freemem, 1);
		return (plist);
	}

	physcontig = ((flags & P_PHYSCONTIG) && (npages > 1));

	/*
	 * If possible, wait until there are enough
	 * free pages to satisfy our entire request.
	 *
	 * XXX:	Before waiting, we try to arrange to get more pages by
	 *	processing the i/o completion list and prodding the
	 *	pageout daemon.  However, there's nothing to guarantee
	 *	that these actions will provide enough pages to satisfy
	 *	the request.  In particular, the pageout daemon stops
	 *	running when freemem > lotsfree, so if npages > lotsfree
	 *	there's nothing going on that will bring freemem up to
	 *	a value large enough to satisfy the request.
	 */
	s = splimp();
	while (freemem < npages) {

try_again:
		if (!(flags & P_CANWAIT)) {
			trace4(TR_PAGE_GET, bytes, flags, freemem, 2);
			(void) splx(s);
			return (plist);
		}
		/*
		 * Given that we can wait, call cleanup directly to give
		 * it a chance to add pages to the free list.  This strategy
		 * avoids the cost of context switching to the pageout
		 * daemon unless it's really necessary.
		 */
		if (bclnlist != NULL) {
			(void) splx(s);
			cleanup();
			s = splimp();
			continue;
		}
		/*
		 * There's nothing immediate waiting to become available.
		 * Turn the pageout daemon loose to find something.
		 */
		trace1(TR_PAGEOUT_CALL, 0);
		outofmem();
		freemem_wait++;
		VMLOG(X_PAGEGET_SLEEP, npages, freemem, freemem_wait);
		trace4(TR_PAGE_GET_SLEEP, bytes, flags, freemem, 0);
		(void) sleep((caddr_t)&freemem, PSWP+2);
		trace4(TR_PAGE_GET_SLEEP, bytes, flags, freemem, 1);
	}

	mon_enter(&page_freelock);

	if (physcontig) {
		register int numpages = npages;

		for(pp = pages; pp < epages; ++pp) {
			if (pp->p_free) {
				for(++pp; --numpages > 0 && pp < epages; pp++)
					if (!pp->p_free)
						break;
				if (numpages == 0)
					break;
				numpages = npages;
			}
		}

		if (numpages != 0) {
			mon_exit(&page_freelock);
			goto try_again;
		}
	}

	freemem -= npages;

	VMLOG(X_PAGEGET_GOTMEM, npages, freemem, freemem_wait);
	trace4(TR_PAGE_GET, bytes, flags, freemem, 0);
	/*
	 * If satisfying this request has left us with too little
	 * memory, start the wheels turning to get some back. The
	 * first clause of the test prevents waking up the pageout
	 * daemon in situations where it would decide that there's
	 * nothing to do.  (However, it also keeps bclnlist from
	 * being processed when it otherwise would.)
	 *
	 * XXX: Check against lotsfree rather than desfree?
	 */
	if (nscan < desscan && freemem < desfree) {
		trace1(TR_PAGEOUT_CALL, 1);
		outofmem();
	}

	/*
	 * Pull the pages off the free list and build the return list.
	 */
	while (npages--) {

		if (physcontig) {
			--pp;
		} else if ((pp = page_freelist) == NULL) {
			pp = page_cachelist;
			if (pp == NULL)
				cmn_err(CE_PANIC, "page_get: freemem error");
			ASSERT(pp->p_age == 0);
		} 
#ifdef DEBUG
		else {
			ASSERT(pp->p_age != 0);
			ASSERT(pp->p_vnode == NULL);
		}
#endif

		if (pp->p_age) {
			trace5(TR_PG_ALLOC, pp, pp->p_vnode, pp->p_offset,
				0, 0);
			pagecnt.pc_get_free++;
			page_sub(&page_freelist, pp);
			page_freelist_size--;
		} else {
			trace5(TR_PG_ALLOC, pp, pp->p_vnode, pp->p_offset,
				pp->p_age, 1);
			pagecnt.pc_get_cache++;
			page_sub(&page_cachelist, pp);
			page_cachelist_size--;
			if (pp->p_vnode) {
				/* destroy old vnode association */
				CHECK(pp->p_vnode);
				page_hashout(pp);
			}
		}

		ASSERT(pp->p_mapping == NULL);

		/*
		 * Initialize the p_dblist[] fields.
		 */
		for (i=0; i<(PAGESIZE/NBPSCTR); i++)
			pp->p_dblist[i] = -1;

		pp->p_free = pp->p_mod = pp->p_nc = pp->p_age = 0;
		pp->p_lock = pp->p_intrans = pp->p_pagein = 0;
		pp->p_ref = 1;		/* protect against immediate pageout */
		pp->p_keepcnt = 1;	/* mark the page as `kept' */
		page_add(&plist, pp);
	}
	mon_exit(&page_freelock);
	CHECKFREE();
	(void) splx(s);
	return (plist);
}

#ifdef DEBUG
/*
 * XXX - need to fix up all this page rot!
 */

/*
 * Release a keep count on the page and handle aborting the page if the
 * page is no longer held by anyone and the page has lost its identity.
 */
void
page_rele(pp)
	page_t *pp;
{

	mon_enter(&page_mplock);

	ASSERT (pp >= pages && pp < epages);
	if (pp->p_keepcnt == 0)			/* sanity check */
		cmn_err(CE_PANIC, "page_rele");
	if (--pp->p_keepcnt == 0) {
		while (pp->p_want) {
			cv_broadcast(&page_mplock, (char *)pp);
			pp->p_want = 0;
		}
	}

	mon_exit(&page_mplock);

	if (pp->p_keepcnt == 0 && (pp->p_gone || pp->p_vnode == NULL))
		page_abort(pp);			/* yuck */
}

/*
 * Lock a page.
 */
void
page_lock(pp)
	page_t *pp;
{

	ASSERT (pp >= pages && pp < epages);
	mon_enter(&page_mplock);
	while (pp->p_lock)
		page_cv_wait(pp);
	pp->p_lock = 1;
	mon_exit(&page_mplock);
}

/*
 * Unlock a page.
 */
void
page_unlock(pp)
	page_t *pp;
{

	ASSERT (pp >= pages && pp < epages);
	mon_enter(&page_mplock);
/* XXX */
	if (pp->p_intrans)
		call_debug("page_unlock: unlocking intrans page");
/* XXX */
	pp->p_lock = 0;
	while (pp->p_want) {
		cv_broadcast(&page_mplock, (char *)pp);
		pp->p_want = 0;
	}
	mon_exit(&page_mplock);
}
#endif

/*
 * Add page ``pp'' to the hash/vp chains for <vp, offset>.
 */
void
page_hashin(pp, vp, offset, lock)
	register page_t *pp;
	register struct vnode *vp;
	u_int offset, lock;
{
	register page_t **hpp;

	ASSERT (pp >= pages && pp < epages);
#ifdef DEBUG
	ASSERT(pp->p_uown == NULL);
#endif
	pp->p_vnode = vp;
	pp->p_offset = offset;
	pp->p_lock = lock;

	hpp = &page_hash[PAGE_HASHFUNC(vp, offset)];
	pp->p_hash = *hpp;
	*hpp = pp;

	/*
	 * Add the page to the front of the linked list of pages
	 * using p_vpnext/p_vpprev pointers for the list.
	 */
	if (vp->v_pages == NULL) {
		pp->p_vpnext = pp->p_vpprev = pp;
	} else {
		pp->p_vpnext = vp->v_pages;
		pp->p_vpprev = vp->v_pages->p_vpprev;
		vp->v_pages->p_vpprev = pp;
		pp->p_vpprev->p_vpnext = pp;
	}
	vp->v_pages = pp;
	CHECKPP(pp);
}

/*
 * Remove page ``pp'' from the hash and vp chains and remove vp association.
 */
void
page_hashout(pp)
	register page_t *pp;
{
	register page_t **hpp, *hp;
	register struct vnode *vp;

	ASSERT (pp >= pages && pp < epages);
	CHECKPP(pp);
	vp = pp->p_vnode;
	hpp = &page_hash[PAGE_HASHFUNC(vp, pp->p_offset)];
	for (;;) {
		hp = *hpp;
		if (hp == pp)
			break;
		if (hp == NULL)
			cmn_err(CE_PANIC, "page_hashout");
		hpp = &hp->p_hash;
	}
	*hpp = pp->p_hash;

	pp->p_hash = NULL;
	pp->p_vnode = NULL;
	pp->p_offset = 0;
	pp->p_gone = 0;

	/*
	 * Remove this page from the linked list of pages
	 * using p_vpnext/p_vpprev pointers for the list.
	 */
	CHECKPP(pp);
	if (vp->v_pages == pp)
		vp->v_pages = pp->p_vpnext;		/* go to next page */

	if (vp->v_pages == pp)
		vp->v_pages = NULL;			/* page list is gone */
	else {
		pp->p_vpprev->p_vpnext = pp->p_vpnext;
		pp->p_vpnext->p_vpprev = pp->p_vpprev;
	}
	pp->p_vpprev = pp->p_vpnext = pp;	/* make pp a list of one */
}

/*
 * Add the page to the front of the linked list of pages
 * using p_next/p_prev pointers for the list.
 */
STATIC void
page_add(ppp, pp)
	register page_t **ppp, *pp;
{

	ASSERT (pp >= pages && pp < epages);
	if (*ppp == NULL) {
		pp->p_next = pp->p_prev = pp;
	} else {
		pp->p_next = *ppp;
		pp->p_prev = (*ppp)->p_prev;
		(*ppp)->p_prev = pp;
		pp->p_prev->p_next = pp;
	}
	*ppp = pp;
	CHECKPP(pp);
}

/*
 * Remove this page from the linked list of pages
 * using p_next/p_prev pointers for the list.
 */
void
page_sub(ppp, pp)
	register page_t **ppp, *pp;
{

	ASSERT (pp >= pages && pp < epages);
	CHECKPP(pp);
	if (*ppp == NULL || pp == NULL)
		cmn_err(CE_PANIC, "page_sub");

	if (*ppp == pp)
		*ppp = pp->p_next;		/* go to next page */

	if (*ppp == pp)
		*ppp = NULL;			/* page list is gone */
	else {
		pp->p_prev->p_next = pp->p_next;
		pp->p_next->p_prev = pp->p_prev;
	}
	pp->p_prev = pp->p_next = pp;		/* make pp a list of one */
}

/*
 * Add this page to the list of pages, sorted by offset.
 * Assumes that the list given by *ppp is already sorted.
 */
void
page_sortadd(ppp, pp)
	register page_t **ppp, *pp;
{
	register page_t *p1;
	register u_int off;

	ASSERT (pp >= pages && pp < epages);
	CHECKLIST(*ppp);
	CHECKPP(pp);
	if (*ppp == NULL) {
		pp->p_next = pp->p_prev = pp;
		*ppp = pp;
	} else {
		/*
		 * Figure out where to add the page to keep list sorted
		 */
		p1 = *ppp;
		if (pp->p_vnode != p1->p_vnode && p1->p_vnode != NULL &&
		    pp->p_vnode != NULL)
			call_debug("page_sortadd: bad vp");

		off = pp->p_offset;
		if (off < p1->p_prev->p_offset) {
			do {
				if (off == p1->p_offset)
					call_debug("page_sortadd: same offset");
				if (off < p1->p_offset)
					break;
				p1 = p1->p_next;
			} while (p1 != *ppp);
		}

		/* link in pp before p1 */
		pp->p_next = p1;
		pp->p_prev = p1->p_prev;
		p1->p_prev = pp;
		pp->p_prev->p_next = pp;

		if (off < (*ppp)->p_offset)
			*ppp = pp;		/* pp is at front */
	}
	CHECKLIST(*ppp);
}

/*
 * Wait for page if kept and then reclaim the page if it is free.
 * Caller needs to verify page contents after calling this routine.
 */
void
page_wait(pp)
	register page_t *pp;
{
	struct vnode *vp;
	u_int offset;

	ASSERT (pp >= pages && pp < epages);
	CHECKPP(pp);
	vp = pp->p_vnode;
	offset = pp->p_offset;

	mon_enter(&page_mplock);
	while (pp->p_keepcnt > 0)
		page_cv_wait(pp);
	mon_exit(&page_mplock);

	/*
	 * If the page is now on the free list and still has
	 * its original identity, get it back.  If the page
	 * has lost its old identity, the caller of page_wait
	 * is responsible for verifying the page contents.
	 */
	if (pp->p_free && pp->p_vnode == vp && pp->p_offset == offset)
		page_reclaim(pp);
	CHECKPP(pp);
}

/*
 * Lock a physical page into memory "long term".  Used to support "lock
 * in memory" functions.  Accepts the page to be locked, and a claim variable
 * to indicate whether a claim for an extra page should be recorded (to
 * support, for instance, a potential copy-on-write).  
 */

/* ARGSUSED */

int
page_pp_lock(pp, claim, kernel)
	page_t *pp;		/* page to be locked */
	int claim;			/* amount extra to be claimed */
	int kernel;			/* must succeed -- ignore checking */
{
	int r = 0;			/* result -- assume failure */

	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
	page_lock(pp);
	mon_enter(&page_locklock);

	/* To support potential copy-on-write, reserve extra
	 * page.
	 */
	if (claim) {
		if ((availrmem - 1 ) >= pages_pp_maximum) {
		 	--availrmem;
			pages_pp_locked++;
			if (pp->p_cowcnt < (u_short) PAGE_LOCK_MAXIMUM)
				if (++pp->p_cowcnt == PAGE_LOCK_MAXIMUM)
					printf("Page frame 0x%x locked permanently\n",
						page_pptonum(pp));
			r = 1;
		} 
	} else {
		if (pp->p_lckcnt) {
			if (pp->p_lckcnt < (u_short) PAGE_LOCK_MAXIMUM)
				if (++pp->p_lckcnt == PAGE_LOCK_MAXIMUM)
					printf("Page frame 0x%x locked permanently\n",
						page_pptonum(pp));
			r = 1;
		} else {
			if ((availrmem - 1 ) >= pages_pp_maximum) {
				pages_pp_locked++;
				--availrmem;
				++pp->p_lckcnt;
				r = 1;
			}
		}
	}
	mon_exit(&page_locklock);
	page_unlock(pp);	
	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
	return (r);
}

/*
 * Decommit a lock on a physical page frame.  Account for claims if
 * appropriate.
 */

/* ARGSUSED */

void
page_pp_unlock(pp, claim, kernel)
	page_t *pp;		/* page to be unlocked */
	int claim;			/* amount to be unclaimed */
	int kernel;			/* this was a kernel lock */
{

	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
	page_lock(pp);
	mon_enter(&page_locklock);
	if (claim) {
		ASSERT(pp->p_cowcnt > 0);
		pp->p_cowcnt--;
		pages_pp_locked--;
		availrmem++;
	} else {
		ASSERT(pp->p_lckcnt > 0);
		if (--pp->p_lckcnt == 0) {
			pages_pp_locked--;
			availrmem++;
		}
	}
	mon_exit(&page_locklock);
	page_unlock(pp);
	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
}

/*
 * Transfer a claim to a real lock on a physical page.  Used after a 
 * copy-on-write of a locked page has occurred.  
 */
void
page_pp_useclaim(opp, npp)
	page_t *opp;		/* original page frame losing lock */
	page_t *npp;		/* new page frame gaining lock */
{

	ASSERT((short)opp->p_lckcnt >= 0);
	ASSERT((short)opp->p_cowcnt >= 0);
	ASSERT((short)npp->p_lckcnt >= 0);
	ASSERT((short)npp->p_cowcnt >= 0);
	page_lock(npp);
	page_lock(opp);
	mon_enter(&page_locklock);
	npp->p_lckcnt++;
	ASSERT(opp->p_cowcnt);
	opp->p_cowcnt--;
	mon_exit(&page_locklock);
	page_unlock(opp);
	page_unlock(npp);
	ASSERT((short)opp->p_lckcnt >= 0);
	ASSERT((short)opp->p_cowcnt >= 0);
	ASSERT((short)npp->p_lckcnt >= 0);
	ASSERT((short)npp->p_cowcnt >= 0);
}

/*
 * Simple claim adjust functions -- used to support to support changes in
 * claims due to changes in access permissions.
 */
int
page_addclaim(pp)
	page_t *pp;			/* claim count to add */
{
	int r = 1;			/* result */

	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
	mon_enter(&page_locklock);
	ASSERT(pp->p_lckcnt > 0);
	if (--pp->p_lckcnt == 0) {
		pp->p_cowcnt++;
	} else {
		if ((availrmem - 1 ) >= pages_pp_maximum) {
		 	--availrmem;
			pages_pp_locked++;
			if (pp->p_cowcnt < (u_short) PAGE_LOCK_MAXIMUM)
				if (++pp->p_cowcnt == PAGE_LOCK_MAXIMUM)
					printf("Page frame 0x%x locked permanently\n",
						page_pptonum(pp));
		} else {
			pp->p_lckcnt++;
			r = 0;
		}
	}
	mon_exit(&page_locklock);
	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
	return (r);
}

void
page_subclaim(pp)
	page_t *pp;			/* claim count to remove */
{

	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
	mon_enter(&page_locklock);
	ASSERT(pp->p_cowcnt > 0);
	pp->p_cowcnt--;
	if (pp->p_lckcnt) {
		availrmem++;
		pages_pp_locked--;
	}
	pp->p_lckcnt++;
	mon_exit(&page_locklock);
	ASSERT((short)pp->p_lckcnt >= 0);
	ASSERT((short)pp->p_cowcnt >= 0);
}

/*
 * Simple functions to transform a page pointer to a
 * physical page number and vice versa.  For now assume
 * pages refers to the page number pages_base and pages
 * increase from there with one page structure per
 * PAGESIZE / MMU_PAGESIZE physical page frames.
 */

#ifdef	sun386
u_int
page_pptonum(pp)
	page_t *pp;
{
	if (pp > epages2)
		cmn_err(CE_PANIC, "page_pptonum");

	if (pp >= epages)
		return ((u_int)((pp - epages) * (PAGESIZE/MMU_PAGESIZE)) +
		    pages_base2);

	return ((u_int)((pp - pages) * (PAGESIZE/MMU_PAGESIZE)) + pages_base);
}

page_t *
page_numtopp(pfn)
	register u_int pfn;
{
	if (pfn >= pages_base2 && pfn < pages_end2)
		return (&epages[(pfn - pages_base2)/(PAGESIZE/MMU_PAGESIZE)]);

	if (pfn < pages_base || pfn >= pages_end)
		return ((page_t *)NULL);
	else
		return (&pages[(pfn - pages_base) / (PAGESIZE/MMU_PAGESIZE)]);
}

#else

#ifdef DEBUG

u_int
page_pptonum(pp)
	page_t *pp;
{
	return ((u_int)((pp - pages) * (PAGESIZE/MMU_PAGESIZE)) + pages_base);
}

page_t *
page_numtopp(pfn)
	register u_int pfn;
{
	if (pfn < pages_base || pfn >= pages_end)
		return ((page_t *)NULL);
	else
		return (&pages[(pfn - pages_base) / (PAGESIZE/MMU_PAGESIZE)]);
}
#endif	/*DEBUG*/
#endif	/*sun386*/

/*
 * This routine is like page_numtopp, but will only return page structs
 * for pages which are ok for loading into hardware using the page struct.
 */
page_t *
page_numtookpp(pfn)
	register u_int pfn;
{
	register page_t *pp;

#ifdef	sun386
	if (pfn >= pages_base2 && pfn < pages_end2) {
		pp = &epages[(pfn - pages_base2) / (PAGESIZE/MMU_PAGESIZE)];
		if (pp->p_free || pp->p_gone)
			return ((page_t *)NULL);
		return (pp);
	}
#endif

	if (pfn < pages_base || pfn >= pages_end)
		return ((page_t *)NULL);
	pp = &pages[(pfn - pages_base) / (PAGESIZE/MMU_PAGESIZE)];
	if (pp->p_free || pp->p_gone)
		return ((page_t *)NULL);
	return (pp);
}

/*
 * This routine is like page_numtopp, but will only return page structs
 * for pages which are ok for loading into hardware using the page struct.
 * If not for the things like the window system lock page where we
 * want to make sure that the kernel and the user are exactly cache
 * consistent, we could just always return a NULL pointer here since
 * anyone mapping physical memory isn't guaranteed all that much
 * on a virtual address cached machine anyways.  The important thing
 * here is not to return page structures for things that are possibly
 * currently loaded in DVMA space, while having the window system lock
 * page still work correctly.
 */
page_t *
page_numtouserpp(pfn)
	register u_int pfn;
{
	register page_t *pp;

#ifdef	sun386
	if (pfn >= pages_base2 && pfn < pages_end2) {
		pp = &epages[(pfn - pages_base2) / (PAGESIZE/MMU_PAGESIZE)];
		if (pp->p_free || pp->p_gone || pp->p_intrans || pp->p_lock ||
		    /* is this page possibly involved in indirect (raw) IO */
		    (pp->p_keepcnt > 0 && pp->p_vnode != NULL))
			return ((page_t *)NULL);
		return (pp);
	}
#endif

	if (pfn < pages_base || pfn >= pages_end)
		return ((page_t *)NULL);
	pp = &pages[(pfn - pages_base) / (PAGESIZE/MMU_PAGESIZE)];
	if (pp->p_free || pp->p_gone || pp->p_intrans || pp->p_lock ||
	    /* is this page possibly involved in indirect (raw) IO? */
	    (pp->p_keepcnt > 0 && pp->p_vnode != NULL))
		return ((page_t *)NULL);
	return (pp);
}

/*
 * Debugging routine only!
 * XXX - places calling this either need to
 * remove the test altogether or call cmn_err(CE_PANIC).
 */
#include "vm/reboot.h"
#include "vm/debugger.h"

extern int	call_demon();

void
call_debug(mess)
	char *mess;
{
 	cmn_err(CE_WARN, mess);
	call_demon();
}

#ifdef PAGE_DEBUG
/*
 * Debugging routine only!
 */
STATIC void
page_vp_check(vp)
	register struct vnode *vp;
{
	register page_t *pp;
	int count = 0;
	int err = 0;

	if (vp == NULL)
		return;

	if ((pp = vp->v_pages) == NULL) {
		/* random check to see if no pages on this vp exist */
		if ((pp = page_find(vp, 0)) != NULL) {
			cmn_err(CE_CONT, "page_vp_check: pp=%x on NULL vp list\n", vp);
			call_debug("page_vp_check");
		}
		return;
	}

	do {
		if (pp->p_vnode != vp) {
			cmn_err(CE_CONT, "pp=%x pp->p_vnode=%x, vp=%x\n",
			    pp, pp->p_vnode, vp);
			err++;
		}
		if (pp->p_vpnext->p_vpprev != pp) {
			cmn_err(CE_CONT, "pp=%x, pp->p_vpnext=%x, pp->p_vpnext->p_vpprev=%x\n",
			    pp, pp->p_vpnext, pp->p_vpnext->p_vpprev);
			err++;
		}
		if (++count > 10000) {
			cmn_err(CE_CONT, "vp loop\n");
			err++;
			break;
		}
		pp = pp->p_vpnext;
	} while (err == 0 && pp != vp->v_pages);

	if (err)
		call_debug("page_vp_check");
}

/*
 * Debugging routine only!
 */
STATIC void
page_free_check()
{
	int err = 0;
	int count = 0;
	register page_t *pp;

	if (page_freelist != NULL) {
		pp = page_freelist;
		do {
			if (pp->p_free == 0 || pp->p_age == 0) {
				err++;
				cmn_err(CE_CONT, "page_free_check: pp = %x\n", pp);
			}
			count++;
			pp = pp->p_next;
		} while (pp != page_freelist);
	}
	if (page_cachelist != NULL) {
		pp = page_cachelist;
		do {
			if (pp->p_free == 0 || pp->p_age != 0) {
				err++;
				cmn_err(CE_CONT, "page_free_check: pp = %x\n", pp);
			}
			count++;
			pp = pp->p_next;
		} while (pp != page_cachelist);
	}

	if (err || count != freemem) {
		cmn_err(CE_CONT, "page_free_check:  count = %x, freemem = %x\n",
		    count, freemem);
		call_debug("page_check_free");
	}
}

/*
 * Debugging routine only!
 * Verify that the list is properly sorted by offset on same vp
 */
void
page_list_check(plist)
	page_t *plist;
{
	register page_t *pp = plist;

	if (pp == NULL)
		return;
	while (pp->p_next != plist) {
		if (pp->p_next->p_offset <= pp->p_offset ||
		    pp->p_vnode != pp->p_next->p_vnode) {
			cmn_err(CE_CONT, "pp = %x <%x, %x> pp next = %x <%x, %x>\n",
			    pp, pp->p_vnode, pp->p_offset, pp->p_next,
			    pp->p_next->p_vnode, pp->p_next->p_offset);
			call_debug("page_list_check");
		}
		pp = pp->p_next;
	}
}

/*
 * Debugging routine only!
 * Verify that pp is actually on vp page list.
 */
void
page_pp_check(pp)
	register page_t *pp;
{
	register page_t *p1;
	register struct vnode *vp;

	if ((vp = pp->p_vnode) == (struct vnode *)NULL)
		return;

	if ((p1 = vp->v_pages) == (page_t *)NULL) {
		cmn_err(CE_CONT, "pp = %x, vp = %x\n", pp, vp);
		call_debug("NULL vp page list");
		return;
	}

	do {
		if (p1 == pp)
			return;
	} while ((p1 = p1->p_vpnext) != vp->v_pages);

	cmn_err(CE_CONT, "page %x not on vp %x page list\n", pp, vp);
	call_debug("vp page list");
}
#endif /* PAGE_DEBUG */

/*
 * The following are used by the sys3b S3BDELMEM and S3BADDMEM
 * functions.
 */

STATIC page_t *Delmem;	/* Linked list of deleted pages. */
STATIC int Delmem_cnt;	/* Count of number of deleted pages. */

STATIC int
page_delmem(count)
	register int count;
{
	register page_t	*pp;
		
	if (freemem < count
	  || availrmem - count < tune.t_minarmem 
	  || availsmem - count < tune.t_minasmem) {
		return EINVAL;
	}

	while (count > 0) {
		page_t **ppl;

		ppl = &Delmem;

		pp = page_get(PAGESIZE, P_NOSLEEP);
		if (pp == NULL) 
			return EINVAL;
		page_add(ppl, pp);

		count--;
		Delmem_cnt++;
		availrmem--;
		availsmem--;
	}

	return(0);
}

STATIC int
page_addmem(count)
	register int count;
{
	register page_t	*pp;

	while (count > 0) {
		page_t **ppl;

		pp = *(ppl = &Delmem);
		if (pp == NULL) 
			return EINVAL;
		page_sub(ppl, pp);
		page_rele(pp);

		count--;
		Delmem_cnt--;
		availrmem++;
		availsmem++;
	}

	return(0);
}

int
page_deladd(add, count, rvp)
	register int count;
	rval_t *rvp;
{
	register int error;

	if (add)
		error = page_addmem(count);
	else
		error = page_delmem(count);
	if (error == 0) {
		if (add)
			rvp->r_val1 = freemem;
		else
			rvp->r_val1 = Delmem_cnt;
	}
	return(error);
}

/*
 * Debugging routine only!
 */

STATIC void
page_print(pp)
	register page_t *pp;
{
	register struct vnode *vp;

        cmn_err(CE_CONT, "^mapping 0x%x nio %d keepcnt %d lck %d cow %d",
                pp->p_mapping, pp->p_nio, pp->p_keepcnt,
                pp->p_lckcnt, pp->p_cowcnt);
	cmn_err(CE_CONT, "^%s%s%s%s%s%s%s%s%s\n", 
		(pp->p_lock)    ? " LOCK"    : "" ,
		(pp->p_want)    ? " WANT"    : "" ,
		(pp->p_free)    ? " FREE"    : "" ,
		(pp->p_intrans) ? " INTRANS" : "" ,
		(pp->p_gone)    ? " GONE"    : "" ,
		(pp->p_mod)     ? " MOD"     : "" ,
		(pp->p_ref)     ? " REF"     : "" ,
		(pp->p_pagein)  ? " PAGEIN"  : "" ,
		(pp->p_age)	? " AGE"  : "" );
        cmn_err(CE_CONT, "^vnode 0x%x, offset 0x%x",
                pp->p_vnode, pp->p_offset);
	if (swap_anon(pp->p_vnode, pp->p_offset))
		cmn_err(CE_CONT, "^  (ANON)");
	else if ((vp = pp->p_vnode) != 0)
		cmn_err(CE_CONT, "^  v_flag 0x%x, v_count %d, v_type %d",
			vp->v_flag, vp->v_count, vp->v_type);
	cmn_err(CE_CONT, "^\nnext 0x%x, prev 0x%x, vpnext 0x%x vpprev 0x%x\n",
	    pp->p_next, pp->p_prev, pp->p_vpnext, pp->p_vpprev);
}

void
phystopp(v)
{
	int pfn;
	page_t *pp;

	pfn = v >> PAGESHIFT;
	cmn_err(CE_CONT, "^pfn=0x%x, ", pfn);
	pp = page_numtopp(pfn);
	if (pp)
		cmn_err(CE_CONT, "^pp=0x%x\n", pp);
	else
		cmn_err(CE_CONT, "^pp=NULL\n");
}

void
pptophys(pp)
	page_t *pp;
{
	int pfn;

	pfn = ((u_int)((pp - pages) * (PAGESIZE/MMU_PAGESIZE)) + pages_base);
	cmn_err(CE_CONT, "^pfn=0x%x, ", pfn);
	cmn_err(CE_CONT, "^phys=0x%x\n", ctob(pfn));
}

xpage_find(vp, off)
	register struct vnode *vp;
	register u_int off;
{
	register page_t *pp;
	register int len = 0;

	for (pp = page_hash[PAGE_HASHFUNC(vp, off)]; pp; pp = pp->p_hash, len++)
		if (pp->p_vnode == vp && pp->p_offset == off) {
			if (pp->p_gone == 0)
				return 0;
		}
	return 1;
}

void
findpage(vp, off)
	register struct vnode *vp;
	register u_int off;
{
	register page_t *pp;
	register int len = 0;
	register int found = 0;

	for (pp = page_hash[PAGE_HASHFUNC(vp, off)]; pp; pp = pp->p_hash, len++)
		if (pp->p_vnode == vp && pp->p_offset == off) {
			if (found++)
				cmn_err(CE_CONT, "^\t\t\t\t\t\t      ");
			cmn_err(CE_CONT, "^%x %s%s%s%s%s%s%s%s%s %d\n", 
				pp,
				(pp->p_lock)    ? "L"    : " " ,
				(pp->p_want)    ? "W"    : " " ,
				(pp->p_free)    ? "F"    : " " ,
				(pp->p_intrans) ? "I" : " " ,
				(pp->p_gone)    ? "G"    : " " ,
				(pp->p_mod)     ? "M"     : " " ,
				(pp->p_ref)     ? "R"     : " " ,
				(pp->p_pagein)  ? "P"  : " " ,
				(pp->p_age)	? "A"  : "" ,
				pp->p_keepcnt);
		}
	if (found == 0)
		cmn_err(CE_CONT, "^not found\n");
}


