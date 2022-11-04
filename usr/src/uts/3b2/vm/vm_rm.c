/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:vm/vm_rm.c	1.7"

/*
 * VM - resource manager
 * As you can see, it needs lots of work
 */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/proc.h"
#include "sys/cmn_err.h"

#include "vm/hat.h"
#include "vm/as.h"
#include "vm/rm.h"
#include "vm/seg.h"
#include "vm/page.h"

#ifdef DEBUG
/*ARGSUSED*/
page_t *
rm_allocpage(seg, addr, len, flags)
	struct seg *seg;
	addr_t addr;
	u_int len;
	u_int flags;
{

	return (page_get(len, flags));
}
#endif

/*
 * This routine is called when we couldn't allocate an anon slot.
 * For now, we simply print out a message and kill of the process
 * who happened to have gotten burned.
 *
 * XXX - swap reservation needs lots of work so this only happens in
 * `nice' places or we need to have a method to allow for recovery.
 */
void
rm_outofanon()
{
	struct proc *p;

	p = u.u_procp;
	cmn_err(CE_WARN, "Sorry, pid %d (%s) was killed due to lack of swap space\n",
	    p->p_pid, u.u_comm);
	/*
	 * To be sure no looping (e.g. in vmsched trying to
	 * swap out) mark process locked in core (as though
	 * done by user) after killing it so no one will try
	 * to swap it out.
	 */
	psignal(p, SIGKILL);
	p->p_flag |= SLOCK;
}

void
rm_outofhat()
{

	cmn_err(CE_PANIC, "out of mapping resources");			/* XXX */
	/*NOTREACHED*/
}

/*
 * Yield the size of an address space.
 */
size_t
rm_assize(as)
	register struct as *as;
{

	return (as == (struct as *)NULL ? 0 : as->a_size);
}

/*
 * Yield the memory claim requirement for an address space.
 */
size_t
rm_asrss(as)
	register struct as *as;
{

	return (as == (struct as *)NULL ? 0 : as->a_rss);
}
