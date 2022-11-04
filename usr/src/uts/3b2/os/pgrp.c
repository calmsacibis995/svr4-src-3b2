/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/pgrp.c	1.22"

#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/cred.h"
#include "sys/vnode.h"
#include "sys/file.h"
#include "sys/fstyp.h"
#include "sys/errno.h"
#include "sys/sbd.h"
#include "sys/immu.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/buf.h"
#include "sys/mount.h"
#include "sys/var.h"
#include "sys/conf.h"
#include "sys/debug.h"
#include "sys/proc.h"
#include "sys/signal.h"
#include "sys/siginfo.h"
#include "sys/fault.h"
#include "sys/syscall.h"
#include "sys/ucontext.h"
#include "sys/prsystm.h"
#include "sys/session.h"
#include "sys/stream.h"
#include "sys/strsubr.h"

#define NHSQUE		64
#define hashpg(X)	(&pgque[(X) & (NHSQUE-1)])

STATIC proc_t *pgque[NHSQUE];

/* 
 * Return 1 if process pointed to by 'cp' has a parent that would
 * prevent its process group from being orphaned.
 */

STATIC int
linkedpg(cp)
	register proc_t *cp;
{
	register proc_t *pp = cp->p_parent;

	if (pp->p_pgrp != cp->p_pgrp && pp->p_sessp == cp->p_sessp)
		return 1;
	return 0;
}

/*
 * Send the specified signal to all processes with 'pgid' as
 * process group ID. 
 */

void
signal(pgid, sig)
	register pid_t pgid;
	int sig;
{
	register proc_t *p;

	if (pgid == 0)
		return;

	for (p = *hashpg(pgid); p; p = p->p_pglink)
		if (p->p_pgrp == pgid)
			psignal(p, sig);
}

/*
 * For each child of process 'pp', detach that child's process group if
 * the process group will become orphaned without 'pp'.
 *
 * If the process group is already detached, or if 'dp' would not prevent
 *	the process group from being orphaned, return immediately.
 *
 * Otherwise, search for another member of the process group that will 
 *	prevent the group from being orphaned.  If one is found, return.
 *
 * Otherwise, mark all member of the group detached.  If any members
 * 	of the group were stopped, continue and hangup the whole group.
 *	(The continue is necessary to keep an orphaned process group
 *	from lingering.  The hangup is necessary, since we are resuming
 *	stopped jobs without the user's knowledge)
 */

void
detachcld(pp)
	register proc_t *pp;
{
	int stopped;
	int linked;
	register struct proc *cp;
	register struct proc *mp;
	register pid_t pgid;

	for (cp = pp->p_child; cp; cp = cp->p_sibling) {
		if (cp->p_flag & SDETACHED)
			continue;
		pgid = cp->p_pgrp;
		stopped = 0;
		linked = 0;
		for (mp = *hashpg(pgid); mp; mp = mp->p_pglink) {
			if (mp->p_pgrp != pgid)
				continue;
			if (mp->p_parent != pp && linkedpg(mp)) {
				linked++;
				break;
			}
			if ((mp->p_stat == SSTOP
			    && mp->p_whystop == PR_JOBCONTROL)
		  	  || sigintset(&mp->p_sig, &stopdefault))
				stopped++;
		}
		if (linked)
			continue;
		for (mp = *hashpg(pgid); mp; mp = mp->p_pglink) {
			if (mp->p_pgrp != pgid)
				continue;
			mp->p_flag |= SDETACHED;
			if (stopped) {
				psignal(mp, SIGHUP);
				psignal(mp, SIGCONT);
			}
		}
	}
}

/*
 * Add a process to a process group
 */
void
joinpg(p, pgid)
	register proc_t *p;
	pid_t pgid;
{
	register proc_t **pp;

	if (pgid == 0)
		return;

	pp = hashpg(pgid);
	p->p_pglink = *pp;
	*pp = p;
	p->p_pgrp = pgid;
	if (pgid <= SHRT_MAX)
		p->p_opgrp = (o_pid_t)pgid;
	else
		p->p_opgrp = (o_pid_t)NOPID;
}

void
attachpg(p)
	register proc_t *p;
{
	register proc_t *q;
	register pid_t pgid;

	pgid = p->p_pgrp;

	/* creating new process group */
	if (p->p_pid == pgid) {	
		for (q = p->p_parent; q->p_pgrp == pgid; q = q->p_parent)
			continue;
		if (q->p_sessp == p->p_sessp)
			p->p_flag &= ~SDETACHED;
		else
			p->p_flag |= SDETACHED;
		return;
	} 

	/* joining existing process group, find a member */
	for (q = *hashpg(pgid); q == p || q->p_pgrp != pgid; q = q->p_pglink)
		continue;

	/* if group is not detached, then neither is the newest member */
	if (!(q->p_flag & SDETACHED))
		p->p_flag &= ~SDETACHED;

	/* this process may be attaching the group */
	else if (linkedpg(p))
		do {
			q->p_flag &= ~SDETACHED;
			do q = q->p_pglink;
			while (q && q->p_pgrp != pgid);
		} while (q);

	else 
		p->p_flag |= SDETACHED;

}

/*
 * Remove a process from a process group
 * If it is the last member of the process group, it's parent (which
 *	may be waiting for a member of this process group to change
 *	state) is awaken.
 * If it is the last member of a controlling terminal's foreground
 *	process group, the controlling process's process group is made
 *	foreground process group
 * If it is not the last member of the process group, and this process
 *	will cause the process group to be orphaned, detach the process group
 */

void
leavepg(p)
	proc_t *p;
{
	register proc_t **pp, *t;
	int members, attached;
	pid_t pgid;

	if ((pgid = p->p_pgrp) == 0)
		return;

	for (members = 0, attached = 0, pp = hashpg(pgid); *pp; ) {
		t = *pp;
		if (t == p) {
			*pp = p->p_pglink;
			p->p_pglink = NULL;
		} else {
			if (t->p_pgrp == pgid) {
				members++;
				if (linkedpg(t))
					attached++;
			}
			pp = &t->p_pglink;
		}
	}

	if (members == 0) {
		register sess_t *sp = p->p_sessp;

		strclearpg(pgid);
		if (sp->s_vp && *sp->s_fgidp == pgid)
			*sp->s_fgidp = 0;
		wakeup((caddr_t)p->p_parent);
	} else if (attached == 0 && !(p->p_flag & SDETACHED)) {
		for (t = *hashpg(pgid); t; t = t->p_pglink) {
			if (t->p_pgrp != pgid)
				continue;
			t->p_flag |= SDETACHED;
		}
	}

	p->p_pgrp = 0;
	p->p_opgrp = 0;
}

/*
 * Return 1 if pgid is the process group ID of an existing process group
 * whose session ID is equal to sid.
 *
 * Otherwise, return 0
 */

int
checkpg(sid, pgid)
	register pid_t sid;
	register pid_t pgid;
{
	register proc_t *pp;

	for (pp = *hashpg(pgid); pp; pp = pp->p_pglink) {
		if (pp->p_pgrp == pgid) {
			if (pp->p_sessp->s_sid == sid)
				return 1;
			break;
		}
	}

	return 0;
}

/*
 * Return 1 if pgid is the process group ID of an existing process group
 * 	that has members not the process group leader in it.
 *
 * Otherwise, return 0.
 */
int
memberspg(pgid)
	register pid_t pgid;
{
	register proc_t *p;

	for (p = *hashpg(pgid); p; p = p->p_pglink) {
		if (p->p_pgrp != pgid)
			continue;
		if (p->p_pgrp == p->p_pid)
			continue;
		return 1;
	}

	return 0;
}

int
detachedpg(pgid)
	register pid_t pgid;
{
	register proc_t *p;

	if (pgid == 0)
		return 1;

	for (p = *hashpg(pgid); p; p = p->p_pglink) {
		if (p->p_pgrp == pgid) {
			if (p->p_flag & SDETACHED)
				return 1;
			else
				return 0;
		}
	}

	return 1;
}
