/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:vm/vm_vpage.c	1.3"

/*
 * VM - virtual page utilities.
 */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/systm.h"
#include "vm/vpage.h"
#include "vm/mp.h"

/*
 * Lock a virtual page via its prot structure.
 * First acquire a higher-level lock against multiprocessing
 * which protects a set of page-level locks (to save space).
 * If the vpage structure is locked, wait for it to be unlocked,
 * and then indicate failure to the caller.  Otherwise,
 * lock the virtual page, release the higher level lock and return.
 */
int
vpage_lock(l, vp)
	register mon_t *l;
	register struct vpage *vp;
{
	register int v;

	mon_enter(l);

	if (vp->vp_lock) {
		while (vp->vp_lock) {
			vp->vp_want = 1;
			cv_wait(l, (char *)vp);
		}
		v = -1;
	} else {
		vp->vp_lock = 1;
		v = 0;
	}

	mon_exit(l);
	return (v);
}

/*
 * Unlock a vpage.
 * Get a high-level lock to protect the prot structure
 * against multiprocessing.  Clear the lock and clear
 * the want, remembering if anyone wants the page.
 * Release the high-level lock and kick any waiters.
 */
void
vpage_unlock(l, vp)
	register mon_t *l;
	register struct vpage *vp;
{
	register int w;

	mon_enter(l);
	vp->vp_lock = 0;
	w = vp->vp_want;
	vp->vp_want = 0;
	if (w)
		cv_broadcast(l, (char *)vp);
	mon_exit(l);
}
