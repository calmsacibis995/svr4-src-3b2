/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _VM_SEG_U_H
#define _VM_SEG_U_H

#ident	"@(#)kernel:vm/seg_u.h	1.5"

/*
 * VM - U-area segment management
 *
 * This file contains definitions related to the u-area segment type.
 *
 * This segment type is not yet implemented.  The intent is that the
 * things defined here evolve into the seg_u implementation.
 */

/*
 * The number of pages covered by a single seg_u slot.
 *
 * This value is the number of (software) pages in the u-area
 * (including the stack in the u-area) plus an additional page
 * for a stack red zone.  If the seg_u implementation is ever
 * generalized to allow variable-size stack allocation, this
 * define will have to change.
 */
/* we don't need an extra page for redzone */
#define	SEGU_PAGES	USIZE

#define	segu_stom(v)	(v)
#define	segu_mtos(v)	(v)



/*
 * Private information per overall segu segment (as opposed
 * to per slot within segment)
 *
 * XXX:	We may wish to modify the free list to handle it as a queue
 *	instead of a stack; this possibly could reduce the frequency
 *	of cache flushes.  If so, we would need a list tail pointer
 *	as well as a list head pointer.
 */
struct segu_segdata {
	/*
	 * info needed:
	 *	- slot vacancy info
	 *	- a way of getting to state info for each slot
	 */
	struct	segu_data *usd_slots;	/* array of segu_data structs,
					   one per slot */
	struct	segu_data *usd_free;	/* slot free list head */
};

/*
 * Private per-slot information.
 */
struct segu_data {
	struct	segu_data *su_next;		/* free list link */
	struct	anon *su_swaddr[SEGU_PAGES];	/* disk address of u area when
						   swapped */
	u_int	su_flags;			/* state info: see below */
	struct proc *su_proc;			/* owner fo the slot */
};

/*
 * Flag bits
 *
 * When the SEGU_LOCKED bit is set, all the resources associated with the
 * corresponding slot are locked in place, so that referencing addresses
 * in the slot's range will not cause a fault.  Clients using this driver
 * to manage a u-area lock down the slot when the corresponding process
 * becomes runnable and unlock it when the process is swapped out.
 */
#define	SEGU_ALLOCATED	0x01		/* slot is in use */
#define	SEGU_LOCKED	0x02		/* slot's resources locked */

/*
 * Private segment information for seg_u.
 *
 * At this stage of the implementation,
 * the proc structure contains the goodies.
 */

#ifdef	_KERNEL
extern struct seg	kwuseg;
extern struct seg	*segu;

/*
 * Public routine declarations not part of the segment ops vector go here.
 */

#if defined(__STDC__)
extern int segu_create(struct seg *, void *);
extern addr_t segu_get(struct proc *);
extern void segu_release(struct proc *);
#else
extern int segu_create();
extern addr_t segu_get();
extern void segu_release();
#endif	/* __STDC__ */

#endif	/* _KERNEL */

#endif	/* _VM_SEG_U_H */
