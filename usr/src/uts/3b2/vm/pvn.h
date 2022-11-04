/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _VM_PVN_H
#define _VM_PVN_H

#ident	"@(#)kernel:vm/pvn.h	1.2"

/*
 * VM - paged vnode.
 *
 * The VM system manages memory as a cache of paged vnodes.
 * This file desribes the interfaces to common subroutines
 * used to help implement the VM/file system routines.
 */

struct	page *pvn_kluster(/* vp, off, seg, addr, offp, lenp, vp_off,
	    vp_len, isra */);
void	pvn_fail(/* plist, flags */);
void	pvn_done(/* bp */);
struct	page *pvn_vplist_dirty(/* vp, off, flags */);
struct	page *pvn_range_dirty(/* vp, off, eoff, offlo, offhi, flags */);
void	pvn_vptrunc(/* vp, vplen, zbytes */);
void	pvn_unloadmap(/* vp, offset, ref, mod */);
int	pvn_getpages(/* getapage, vp, off, len, protp, pl, plsz, seg, addr,
	    rw, cred */);

/*
 * When requesting pages from the getpage routines, pvn_getpages will
 * allocate space to return PVN_GETPAGE_NUM pages which map PVN_GETPAGE_SZ
 * worth of bytes.  These numbers are chosen to be the minimum of the max's
 * given in terms of bytes and pages.
 */
#define	PVN_MAX_GETPAGE_SZ	0x10000		/* getpage size limit */
#define	PVN_MAX_GETPAGE_NUM	0x8		/* getpage page limit */

#if PVN_MAX_GETPAGE_SZ > PVN_MAX_GETPAGE_NUM * PAGESIZE

#define	PVN_GETPAGE_SZ	ptob(PVN_MAX_GETPAGE_NUM)
#define	PVN_GETPAGE_NUM	PVN_MAX_GETPAGE_NUM

#else

#define	PVN_GETPAGE_SZ	PVN_MAX_GETPAGE_SZ
#define	PVN_GETPAGE_NUM	btop(PVN_MAX_GETPAGE_SZ)

#endif

#endif	/* _VM_PVN_H */
