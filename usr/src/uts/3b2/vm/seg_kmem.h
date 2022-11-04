/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _VM_SEG_KMEM_H
#define _VM_SEG_KMEM_H

#ident	"@(#)kernel:vm/seg_kmem.h	1.7"

/*
 * VM - Kernel Segment Driver
 */

/*
 * These variables should be put in a place which
 * is guaranteed not to get paged out of memory.
 */
extern struct as kas;		/* kernel's address space */
extern struct seg kpseg;	/* kernel's "ptov" segment */
extern struct seg kvseg;	/* kernel's "sptalloc" segment */
extern struct seg ktextseg;	/* kernel's "most everything else" segment */

#if defined(__STDC__)

extern int sptalloc(int, int, caddr_t, int);
extern void sptfree(caddr_t, int, int);

/*
 * For segkmem_create, the argsp is actually a pointer to the
 * optional array of pte's used to map the given segment.
 */
extern int segkmem_create(struct seg *, void *);

extern caddr_t kseg(int);
extern void unkseg(caddr_t);

#else

extern int sptalloc();
extern void sptfree();
extern int segkmem_create();
extern caddr_t kseg();
extern void unkseg();

#endif	/* __STDC__ */

/*
 * Flags to pass to segkmem_mapin().
 */
#define	PTELD_LOCK	0x01
#define	PTELD_INTREP	0x02
#define	PTELD_NOSYNC	0x04

#endif	/* _VM_SEG_KMEM_H */
