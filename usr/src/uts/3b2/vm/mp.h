/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _VM_MP_H
#define _VM_MP_H

#ident	"@(#)kernel:vm/mp.h	1.4"

/*
 * VM - multiprocessor/ing support.
 *
 * Currently the mon_enter() / mon_exit() pair implements a
 * simple monitor for objects protected by the appropriate lock.
 * The cv_wait() / cv_broadcast pait implements a simple
 * condition variable which can be used for `sleeping'
 * and `waking' inside a monitor if some resource
 * is needed which is not available.
 */

typedef struct mon_t {
	unchar	dummy;
} mon_t;

#if defined(DEBUG) || defined(lint)
void	mon_enter(/* lk */);
void	mon_exit(/* lk */);
void	cv_wait(/* lk, cond */);
void	cv_broadcast(/* lk, cond */);

#else

/*
 * mon_enter is used as a type of multiprocess semaphore
 * used to implement a monitor where the lock represents
 * the ability to operate on the associated object.
 * For now, the lock/object association is done
 * by convention only.
 * For single processor systems that are debugged, no lock is needed.
 * For multiprocessor systems that are debugged, a simple lock suffices.
 * Only the single processor macros are included.
 */

#define mon_enter(lk)
#define mon_exit(lk)
#define cv_wait(lk, cond) ((void) sleep(cond, PSWP+1))
#define cv_broadcast(lk, cond) ((void) wakeup(cond))

#endif	/* DEBUG */

#define	lock_init(lk)	(lk)->dummy = 0

#endif	/* _VM_MP_H */
