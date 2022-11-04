/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _VM_PAGE_H
#define _VM_PAGE_H

#ident	"@(#)kernel:vm/page.h	1.15"

/*
 * VM - Ram pages.
 *
 * Each physical page has a page structure, which is used to maintain
 * these pages as a cache.  A page can be found via a hashed lookup
 * based on the [vp, offset].  If a page has an [vp, offset] identity,
 * then it is entered on a doubly linked circular list off the
 * vnode using the vpnext/vpprev pointers.   If the p_free bit
 * is on, then the page is also on a doubly linked circular free
 * list using next/prev pointers.  If the p_intrans bit is on,
 * then the page is currently being read in or written back.
 * In this case, the next/prev pointers are used to link the
 * pages together for a consecutive IO request.  If the page
 * is in transit and the the page is coming in (pagein), then you
 * must wait for the IO to complete before you can attach to the page.
 * 
 */
typedef struct page {
	u_int	p_lock: 1,		/* locked for name manipulation */
		p_want: 1,		/* page wanted */
		p_free: 1,		/* on free list */
		p_intrans: 1,		/* data for [vp, offset] intransit */
		p_gone: 1,		/* page has been released */
		p_mod: 1,		/* software copy of modified bit */
		p_ref: 1,		/* software copy of reference bit */
		p_pagein: 1,		/* being paged in, data not valid */
		p_nc: 1,		/* do not cache page */
		p_age: 1;		/* on page_freelist */
	u_int	p_nio : 6;		/* # of outstanding io reqs needed */
	u_short	p_keepcnt;		/* number of page `keeps' */
	struct	vnode *p_vnode;		/* logical vnode this page is from */
	u_int	p_offset;		/* offset into vnode for this page */
	struct page *p_hash;		/* hash by [vnode, offset] */
	struct page *p_next;		/* next page in free/intrans lists */
	struct page *p_prev;		/* prev page in free/intrans lists */
	struct page *p_vpnext;		/* next page in vnode list */
	struct page *p_vpprev;		/* prev page in vnode list */
	caddr_t	p_mapping;		/* hat specific translation info */
	u_short	p_lckcnt;		/* number of locks on page data */
	u_short	p_cowcnt;		/* number of copy on write lock */
	daddr_t	p_dblist[PAGESIZE/NBPSCTR]; /* disk storage for the page */
#ifdef DEBUG
	struct proc *p_uown;		/* process owning it as u-page */
#endif
} page_t;

#ifdef _KERNEL
#define PAGE_HOLD(pp)	(pp)->p_keepcnt++
#define PAGE_RELE(pp)	page_rele(pp)

/*
 * page_get() request flags.
 */
#ifndef P_NOSLEEP
#define	P_NOSLEEP	0x0000
#define	P_CANWAIT	0x0001
#define	P_PHYSCONTIG	0x0002
#endif

#define	PAGE_HASHSZ	page_hashsz

extern	int page_hashsz;
extern	page_t **page_hash;

/*
 * In the long term, we should generalize the next three values to
 * a tuple controlled at a higher level to allow for non-contiguous
 * memory layout.
 */
extern	page_t *pages;		/* array of all page structures */
extern	page_t *epages;		/* end of all pages */
extern	u_int	pages_base;		/* page # for pages[0] */
extern	uint	pages_end;

#ifdef sun386
extern	page_t *epages2;		/* end of absolutely all pages */
extern	u_int	pages_base2;		/* page # for compaq expanded mem */
#endif

/*
 * Variables controlling locking of physical memory.
 */
extern	u_int	pages_pp_locked;	/* physical pages actually locked */
extern	u_int	pages_pp_claimed;	/* physical pages reserved */
extern	u_int	pages_pp_kernel;	/* physical page locks by kernel */
extern	u_int	pages_pp_maximum;	/* tuning: lock + claim <= max */

/*
 * Page frame operations.
 */

void	page_init(/* pp, num, base */);
void	page_reclaim(/* pp */);
page_t *page_find(/* vp, off */);
page_t *page_lookup(/* vp, off */);
int	page_enter(/* pp, vp, off */);
void	page_abort(/* pp */);
void	page_free(/* pp */);
page_t *page_get(/* bytes, flags */);
int	page_pp_lock(/* pp, claim, kernel */);
void	page_pp_unlock(/* pp, claim, kernel */);
void	page_pp_useclaim(/* opp, npp */);
int	page_addclaim(/* claim */);
void	page_subclaim(/* claim */);
void	page_hashin(/* pp, vp, offset, lock */);
void	page_hashout(/* pp */);
void	page_sub(/* ppp, pp */);
void	page_sortadd(/* ppp, pp */);
void	page_wait(/* pp */);
page_t *page_numtookpp(/* pfnum */);

#if defined(DEBUG) || defined(sun386)

u_int	page_pptonum(/* pp */);
page_t *page_numtopp(/* pfnum */);

#else


#define page_pptonum(pp) \
	(((uint)((pp) - pages) * \
		(PAGESIZE/MMU_PAGESIZE)) + pages_base)

#define page_numtopp(pfnum) \
	((pfnum) < pages_base || (pfnum) >= pages_end) ? \
		((struct page *) NULL) : \
		((struct page *) (&pages[(uint) ((pfnum) - pages_base) / \
			(PAGESIZE/MMU_PAGESIZE)]))

#endif

#ifdef DEBUG

void	page_rele(/* pp */);
void	page_lock(/* pp */);
void	page_unlock(/* pp */);

#else


#include "vm/mp.h"
extern	mon_t	page_mplock;		/* lock for manipulating page links */

#define page_rele(pp) { \
	mon_enter(&page_mplock); \
	\
	if (--((struct page *)(pp))->p_keepcnt == 0) { \
		while (((struct page *)(pp))->p_want) { \
			cv_broadcast(&page_mplock, (char *)(pp)); \
			((struct page *)(pp))->p_want = 0; \
		} \
	} \
	\
	mon_exit(&page_mplock); \
	\
	if (((struct page *)(pp))->p_keepcnt == 0 \
		&& (((struct page *)(pp))->p_gone \
		|| ((struct page *)(pp))->p_vnode == NULL)) \
		page_abort(pp); \
}
#define page_lock(pp) { \
	mon_enter(&page_mplock); \
	while (pp->p_lock) \
		page_cv_wait(pp); \
	pp->p_lock = 1; \
	mon_exit(&page_mplock); \
}

#define page_unlock(pp) { \
	mon_enter(&page_mplock); \
	((struct page *)(pp))->p_lock = 0; \
	while (((struct page *)(pp))->p_want) { \
		cv_broadcast(&page_mplock, (char *)(pp)); \
		((struct page *)(pp))->p_want = 0; \
	} \
	mon_exit(&page_mplock); \
}

#endif

#endif /* _KERNEL */

/*
 * Page hash table is a power-of-two in size, externally chained
 * through the hash field.  PAGE_HASHAVELEN is the average length
 * desired for this chain, from which the size of the page_hash
 * table is derived at boot time and stored in the kernel variable
 * page_hashsz.  In the hash function it is given by PAGE_HASHSZ.
 * PAGE_HASHVPSHIFT is defined so that 1 << PAGE_HASHVPSHIFT is
 * the approximate size of a vnode struct.
 */
#define	PAGE_HASHAVELEN		4
#define	PAGE_HASHVPSHIFT	6
#define	PAGE_HASHFUNC(vp, off) \
	((((off) >> PAGESHIFT) + ((int)(vp) >> PAGE_HASHVPSHIFT)) & \
		(PAGE_HASHSZ - 1))

#if defined(__STDC__)
extern void page_cv_wait(page_t *);
extern void ppcopy(page_t *, page_t *);
#else
extern void page_cv_wait();
extern void ppcopy();
#endif	/* __STDC__ */

#endif	/* _VM_PAGE_H */
