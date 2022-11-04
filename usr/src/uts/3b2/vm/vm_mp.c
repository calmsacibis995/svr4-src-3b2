/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:vm/vm_mp.c	1.4"

/*
 * VM - multiprocessor/ing support.
 *
 * Currently the mon_enter() / mon_exit() pair implements a 
 * simple monitor for objects protected by the appropriate lock.
 * The cv_wait() / cv_broadcast pait implements a simple
 * condition variable which can be used for `sleeping'
 * and `waking' inside a monitor if some resource is
 * is needed which is not available.
 *
 * XXX - this code is written knowing about the semantics
 * of sleep/wakeup and UNIX scheduling on a uniprocessor machine.
 */

#include "sys/param.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/inline.h"
#include "sys/cmn_err.h"

#include "vm/mp.h"

#ifdef DEBUG
#define	ISLOCKED	0x1
#define	LOCKWANT	0x2

/*
 * mon_enter is used as a type of multiprocess semaphore
 * used to implement a monitor where the lock represents
 * the ability to operate on the associated object.
 * For now, the lock/object association is done
 * by convention only.
 */
void
mon_enter(lk)
	mon_t *lk;
{}

/*
 * Release the lock associated with a monitor,
 * waiting up anybody that has already decided
 * to wait for this lock (monitor).
 */
void
mon_exit(lk)
	mon_t *lk;
{}

/*
 * Wait for the named condition variable.
 * Must already have the monitor lock when cv_wait is called.
 */
void
cv_wait(lk, cond)
	mon_t *lk;
	char *cond;
{

	(void) sleep(cond, PSWP+1);
}

/*
 * Wake up all processes waiting on the named condition variable.
 *
 * We just use current UNIX sleep/wakeup semantics to delay the actual
 * context switching until later after we have released the lock.
 */
void
cv_broadcast(lk, cond)
	mon_t *lk;
	char *cond;
{
	wakeup(cond);
}
#endif	/*DEBUG */
