/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _VM_VM_HAT_H
#define _VM_VM_HAT_H

#ident	"@(#)kernel:vm/vm_hat.h	1.15"

#include "sys/immu.h"
#include "vm/page.h"

/*
 * VM - Hardware Address Translation management.
 *
 * This file describes the contents of the machine specific
 * hat data structures and the machine specific hat procedures.
 * The machine independent interface is described in <vm/hat.h>.
 */

/*
 * The hat structure is the machine dependent hardware address translation
 * structure kept in the address space structure to show the translation.
 */
typedef struct hat {
	SRAMA	hat_srama[2];	/* SDT addresses for sections 2 and 3 */
	SRAMB	hat_sramb[2];	/* SRAMB entries for sections 2 and 3 */
} hat_t;

/*
 * The page table data (ptdat) structure is a machine dependent hardware
 * address translation structure that keeps information about a page table.
 * This struct is not kept for the kernel page tables.
 *
 * Active page tables:
 *
 * pt_as      - address space the page table belongs to.
 * pt_secseg  - section/segment the page table is mapping.
 * pt_inuse   - number of active pte's in the page table.
 * pt_keepcnt - number of pte's SOFTLOCK'd in page table.
 *
 * Free page tables:
 *
 * pt_addr    - physical addr of the page table.
 * pt_pp      - pp of the page that contains page table.
 *
 */
typedef struct ptdat {
	union {
		struct {
			struct as *ptas;
			ushort ptsecseg;
			u_char ptinuse;
			u_char ptkeepcnt;
		} pt_active;
		struct {
			pte_t	*ptaddr;
			page_t	*ptpp;
		} pt_free;
	} pt_use;
	struct ptdat *pt_prev;	/* prev ptdat in the list. */
	struct ptdat *pt_next;	/* next ptdat in the list. */
} ptdat_t;

#define	pt_as    	pt_use.pt_active.ptas
#define	pt_secseg	pt_use.pt_active.ptsecseg
#define	pt_inuse 	pt_use.pt_active.ptinuse
#define	pt_keepcnt	pt_use.pt_active.ptkeepcnt
#define	pt_addr  	pt_use.pt_free.ptaddr
#define	pt_pp    	pt_use.pt_free.ptpp

/*
 * Flags to indicate the section of the virtual address space with respect
 * to the hat structure contained in the as structure for a process.
 */
#define HAT_SCN2	0
#define HAT_SCN3	1

/*
 * Flags to pass to hat_ptalloc().
 *
 * NOTE: HAT_NOSLEEP and HAT_CANWAIT must match up with P_NOSLEEP
 *       and P_CANWAIT (respectively) in vm/page.h .
 */
#define	HAT_NOSLEEP	0	/* return immediately if no memory.          */
#define	HAT_CANWAIT	1	/* wait if no memory currently available.    */
#define HAT_NOSTEAL	2	/* don't steal a page tbl from another proc. */

/*
 * Page table flag bits
 */
#define	PT_RESERVE	0x100

#define	p_ptbits	p_offset
#define	p_ptdats	p_mapping
#define	p_sdtbits	p_mapping

#ifdef _KERNEL

/*
 * Round up the p_ubptbl address, that was given, to the next 32 byte
 * boundary and convert it to a physical address.
 *
 * paddr_t
 * phys_ubptbl(pp->p_ubptbl)
 * caddr_t virtaddr;
 * {
 *	virtaddr = (virtaddr + 0x1F) & ~0x1F;
 *	return( kvtophys(virtaddr) );
 * }
 */
#if defined(lint)

#define phys_ubptbl(va)	((paddr_t)kvtophys((caddr_t)\
	((((u_long)(va)) + 0x1F) & ~0x1F)))

#else

asm paddr_t
phys_ubptbl(va)
{
%	mem	va;
	ADDW3	&0x1f,va,%r0
	ANDW3	&0xffffffe0,%r0,%r1
	MOVTRW	0(%r1),%r0
}

#endif

extern SRAMA dflt_sdt_p;
extern SRAMA mmu_invalid;

#endif /* _KERNEL */

#endif	/* _VM_VM_HAT_H */
