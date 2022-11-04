/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_VMMAC_H
#define _SYS_VMMAC_H

#ident	"@(#)head.sys:sys/vmmac.h	1.3"

#include "sys/sysmacros.h"


/*
 * Turn virtual addresses into kernel map indices.
 *
 * "Sysmap" is an array of page table entries used to map virtual
 * addresses, starting at (kernel virtual address) Sysbase, to many
 * different things.  Sysmap is managed throught the resource map named
 * "kernelmap".  kmx means kernelmap index, the index (into Sysmap)
 * returned by rmalloc(kernelmap, ...).
 *
 * kmxtob expects an (integer) kernel map index and returns the virtual
 * address by the mmu page number.  btokmx expects a (caddr_t) virtual
 * address and returns the integer kernel map index.
 */
#define	kmxtob(a)	(Sysbase + ((a) << MMU_PAGESHIFT))
#define	btokmx(b)	(((caddr_t)(b) - Sysbase) >> MMU_PAGESHIFT)

/* Average new into old with aging factor time */
#define	ave(smooth, cnt, time) \
	smooth = ((time - 1) * (smooth) + (cnt)) / (time)

/* XXX - this doesn't really belong here */
#define	outofmem()	wakeup((caddr_t)nproc[2]);

#endif	/* _SYS_VMMAC_H */
