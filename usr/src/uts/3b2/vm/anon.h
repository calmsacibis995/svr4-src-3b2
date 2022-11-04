/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _VM_ANON_H
#define _VM_ANON_H

#ident	"@(#)kernel:vm/anon.h	1.4"

/*
 * VM - Anonymous pages.
 */

/*
 * Each page which is anonymous, either in memory or in swap,
 * has an anon structure.  The structure's primary purpose is
 * to hold a reference count so that we can detect when the last
 * copy of a multiply-referenced copy-on-write page goes away.
 * When on the free list, un.next gives the next anon structure
 * in the list.  Otherwise, un.page is a ``hint'' which probably
 * points to the current page.  This must be explicitly checked
 * since the page can be moved underneath us.  This is simply
 * an optimization to avoid having to look up each page when
 * doing things like fork.
 */
struct anon {
	int	an_refcnt;
	union {
		struct	page *an_page;	/* ``hint'' to the real page */
		struct	anon *an_next;	/* free list pointer */
	} un;
	struct anon *an_bap;		/* pointer to real anon */
#ifdef DEBUG
	int	an_use;
#define AN_NONE		0
#define AN_DATA		1
#define AN_UPAGE	2
#endif
};

struct anoninfo {
	u_int	ani_max;	/* maximum anon pages available */
	u_int	ani_free;	/* number of anon pages currently free */
	u_int	ani_resv;	/* number of anon pages reserved */
};

#ifdef _KERNEL
extern	struct anoninfo anoninfo;

struct	anon *anon_alloc();
void	anon_dup(/* old, new, size */);
void	anon_free(/* app, size */);
int	anon_getpage(/* app, protp, pl, sz, seg, addr, rw, cred */);
struct	page *anon_private(/* app, seg, addr, ppsteal */);
struct	page *anon_zero(/* seg, addr, app */);
void	anon_unloadmap(/* ap, ref, mod */);
int	anon_resv(/* size */);
void	anon_unresv(/* size */);
#endif /* _KERNEL */

#endif	/* _VM_ANON_H */
