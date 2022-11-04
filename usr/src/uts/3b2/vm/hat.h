/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _VM_HAT_H
#define _VM_HAT_H

#ident	"@(#)kernel:vm/hat.h	1.5"

/*
 * VM - Hardware Address Translation management.
 *
 * This file describes the machine independent interfaces to
 * the hardware address translation management routines.  Other
 * machine specific interfaces and structures are defined
 * in <vm/vm_hat.h>.  The hat layer manages the address
 * translation hardware as a cache driven by calls from the
 * higher levels of the VM system.
 */

#include "vm/vm_hat.h"

#ifdef _KERNEL
/*
 * No flags specified.
 */
#define	HAT_NOFLAGS	0

/*
 * One time hat initialization
 */
void	hat_init();

/*
 * Operations on hat resources for an address space:
 *	- initialize any needed hat structures for the address space
 *	- free all hat resources now owned by this address space
 *	- initialize any needed hat structures when the process is
 *	  swapped in.
 *	- free all hat resources that are not needed while the process
 *	  is swapped out.
 *	- dup any hat resources that can be created when duplicating
 *	  another process' address space.
 *
 * N.B. - The hat structure is guaranteed to be zeroed when created.
 * The hat layer can choose to define hat_alloc as a macro to avoid
 * a subroutine call if this is sufficient initialization.
 */
void	hat_alloc(/* as */);
void	hat_free(/* as */);
void	hat_swapin(/* as */);
void	hat_swapout(/* as */);
int	hat_dup(/* as */);

/* Operations to allocate/reserve mapping structures
 *	- allocate/reserve mapping structures for a segment.
 *	- free mapping structures for a given segment.
 */
u_int	hat_map(/* seg, ppl, base, prot, flags */);

/*
 * Flags to pass to hat_map().
 *
 * XXX - Not all used right now. Currently only seg_vn pages are
 *       pre-loaded; i.e. before a fault.
 */
#define	HAT_PRELOAD	1	/* pre-load pages for segment.            */
#define	HAT_VNLIST	2	/* When preloading pages, use vnode list. */
#define	HAT_FRLIST	4	/* "" , use free/intrans list pointers.   */

/*
 * Operations on a named address with in a segment:
 *	- load/lock the given page struct
 *	- load/lock the given page frame number
 *	- unlock the given address
 *
 * (Perhaps we need an interface to load several pages at once?)
 */
void	hat_memload(/* seg, addr, pp, prot, flags */);
void	hat_devload(/* seg, addr, pf, prot, flags */);
void	hat_unlock(/* seg, addr */);

/*
 * Operations over an address range:
 *	- change protections
 *	- change mapping to refer to a new segment
 *	- unload mapping
 */
void	hat_chgprot(/* seg, addr, len, prot */);
void	hat_newseg(/* seg, addr, len, nseg */);
void	hat_unload(/* seg, addr, len, flags */);

/*
 * Flags to pass to hat_memload(),hat_devload(), and hat_unload().
 */
#define	HAT_LOCK	1
#define	HAT_UNLOCK	2
#define	HAT_FREEPP	4
#define	HAT_RELEPP	8

/*
 * Operations that work on all active translation for a given page:
 *	- unload all translations to page
 *	- get hw stats from hardware into page struct and reset hw stats
 */
void	hat_pageunload(/* pp */);
void	hat_pagesync(/* pp */);

/*
 * Operations that return physical page numbers (ie - used by mapin):
 *	- return the pfn for kernel virtual address
 *	- return the pfn for arbitrary virtual address
 */
u_int	hat_getkpfnum(/* addr */);
/*
 * XXX - This one is not yet implemented - not yet needed
 * u_int hat_getpfnum(as, addr);
 */

#endif /* _KERNEL */

#endif	/* _VM_HAT_H */
