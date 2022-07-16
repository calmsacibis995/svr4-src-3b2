/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/pgrp.c	1.24"

#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/file.h"
#include "sys/cred.h"
#include "sys/vnode.h"
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

/* 
 * Return 1 if process pointed to by 'cp' has a parent that would
 * prevent its process group from being orphaned, 0 otherwise
 */

STATIC int
pglinked(cp)
	register proc_t *cp;
{
	register proc_t *pp = cp->p_parent;

	if (pp->p_pgidp != cp->p_pgidp && pp->p_sessp == cp->p_sessp)
		return 1;
	return 0;
}

/*
 * Send the specified signal to all processes whose process group ID is
 * equal to 'pgid'
 */

void
pgsignal(pidp, sig)
	register struct pid *pidp;
	int sig;
{
	register proc_t *prp;

 	for (prp = pidp->pid_pglink; prp; prp = prp->p_pglink)
		psignal(prp, sig);
}

/*
 * Add a process to a process group
 */

void
pgjoin(p, pgp)
	register proc_t *p;
	register struct pid *pgp;
{

	ASSERT(pgp->pid_pglink != NULL);
	ASSERT(p->p_pgidp == NULL);
	ASSERT(p->p_pglink == NULL);

	if (pgp->pid_pgorphaned && pglinked(p))
		pgp->pid_pgorphaned = 0;

	p->p_pgidp = pgp;
	if (pgp->pid_id <= SHRT_MAX)
		p->p_opgrp = (o_pid_t)pgp->pid_id;
	else
            	p->p_opgrp = (o_pid_t)NOPID;
	p->p_pglink = pgp->pid_pglink;
	pgp->pid_pglink = p;
}

void
pgcreate(p)
	register proc_t *p;
{
	register proc_t *prp;
	register struct pid *pgp;

	ASSERT(p->p_pidp != NULL);
	ASSERT(p->p_pgidp == NULL);
	ASSERT(p->p_pglink == NULL);

	pgp = p->p_pidp;

	PID_HOLD(pgp);

	pgp->pid_pglink = p;
	p->p_pgidp = pgp;
	if (pgp->pid_id <= SHRT_MAX)
		p->p_opgrp = (o_pid_t)pgp->pid_id;
	else
            	p->p_opgrp = (o_pid_t)NOPID;

	for (prp = p->p_parent; prp->p_pgidp == pgp; prp = prp->p_parent) {
		ASSERT(prp != NULL);
		continue;
	}

	if (prp->p_sessp == p->p_sessp)
		pgp->pid_pgorphaned = 0;
	else
		pgp->pid_pgorphaned = 1;

}

void
pgexit(prp)
	proc_t *prp;
{
	register proc_t *p;
	register proc_t **pp;
	register struct pid *pgp;

	pgp = prp->p_pgidp;

	for (pp = &pgp->pid_pglink; ; pp = &(*pp)->p_pglink) {
		ASSERT(*pp != NULL);
		if (*pp == prp) {
			*pp = prp->p_pglink;
			break;
		}
	}

	prp->p_pgidp = NULL;
	prp->p_pglink = NULL;
	prp->p_opgrp = 0;

	if ((p = pgp->pid_pglink) == NULL)
		PID_RELE(pgp);
	else if (pgp->pid_pgorphaned == 0) {
		do {
			if (pglinked(p))
				return;
		} while ((p = p->p_pglink) != NULL);
		pgp->pid_pgorphaned = 1;
	}

}

/*
 * process 'pp' is exiting or execing - check to see if this will
 * orphan its children's process groups
 */

void
pgdetach(pp)
	proc_t *pp;
{
	int stopped;
	register proc_t *cp;
	register proc_t *mp;
	register struct pid *pgp;

	for (cp = pp->p_child; cp; cp = cp->p_sibling) {
		if ((pgp = cp->p_pgidp)->pid_pgorphaned)
			continue;
		stopped = 0;
		mp = pgp->pid_pglink;
		ASSERT(mp != NULL);
		for (;;) {
			if (mp != pp && mp->p_parent != pp && pglinked(mp))
				break;
			if (mp->p_stat == SSTOP
			    && mp->p_whystop == PR_JOBCONTROL)
				stopped++;
			if ((mp = mp->p_pglink) == NULL) {
				pgp->pid_pgorphaned = 1;
				if (stopped) {
					pgsignal(pgp, SIGHUP);
					pgsignal(pgp, SIGCONT);
				}
				break;
			}
		}
	}
}

/*
 * Return 1 if pgid is the process group ID of an existing process group
 *	that has members not the process group leader in it.
 *
 * Otherwise, return 0.
 */

int
pgmembers(pgid)
	register pid_t pgid;
{
 	register proc_t *prp;

        for (prp = pgfind(pgid); prp; prp = prp->p_pglink)
		if (prp->p_pid != pgid)
			return 1;

	return 0;
}

