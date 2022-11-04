/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/session.c	1.27"

#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/vfs.h"
#include "sys/cred.h"
#include "sys/vnode.h"
#include "sys/signal.h"
#include "sys/evecb.h"
#include "sys/hrtcntl.h"
#include "sys/priocntl.h"
#include "sys/events.h"
#include "sys/evsys.h"
#include "sys/file.h"
#include "sys/errno.h"
#include "sys/sbd.h"
#include "sys/immu.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/buf.h"
#include "sys/var.h"
#include "sys/conf.h"
#include "sys/debug.h"
#include "sys/proc.h"
#include "sys/stream.h"
#include "sys/strsubr.h"
#include "sys/session.h"
#include "sys/priocntl.h"
#include "sys/kmem.h"

sess_t session0 = {
	0,	/* s_procs */
	0555,	/* s_mode  */
	0,	/* s_sid   */
	0,	/* s_uid   */
	0,	/* s_gid   */
	NODEV,	/* s_dev   */
	0,	/* s_sidp  */
	0,	/* s_fgidp */
	0	/* s_vp    */
};

void
freectty(sp)
register sess_t *sp;
{
	register vnode_t *vp = sp->s_vp;
	register i;
	extern void ssinvalidate();

	ASSERT(vp != NULL);

	ssinvalidate(sp);

	for (i = vp->v_count; i; i--) {
		VOP_CLOSE(vp, 0, 1, (off_t)0, nproc[1]->p_cred);
		VN_RELE(vp);
	}

	*sp->s_sidp = 0;
	*sp->s_fgidp = 0;
	sp->s_vp = NULL;
}

void
forksession(pp,sp)
proc_t *pp;
sess_t *sp;
{
	pp->p_sessp = sp;
	++(sp->s_procs);
}

void
exitsession(pp)
proc_t *pp;
{
	register sess_t *sp;
	sp = pp->p_sessp;
	pp->p_sessp = NULL;

	ASSERT(sp);
	ASSERT(sp->s_procs > 0);

	if (--(sp->s_procs) == 0) {
		ASSERT(sp != &session0);
		strclearsid(sp->s_sid);
		kmem_free(sp,sizeof(sess_t));
	}
}

void
newsession()
{
	register proc_t *pp;
	register sess_t *sp;

	pp = u.u_procp;
	sp = pp->p_sessp;

	ASSERT(pp->p_pid != sp->s_sid);

	sp = (sess_t *)kmem_alloc(sizeof (sess_t), 0);

	leavepg(pp);
	joinpg(pp,pp->p_pid);

	exitsession(pp);

	sp->s_procs = 1;
	sp->s_sid = pp->p_pid;
	sp->s_dev = NODEV;
	sp->s_vp = NULL;

	pp->p_sessp = sp;
	pp->p_flag |= SDETACHED;
}

dev_t
cttydev(pp)
proc_t *pp;
{
	register sess_t *sp = pp->p_sessp;
	if (sp->s_vp == NULL)
		return NODEV;
	return sp->s_dev;
}

void
alloctty(pp,vp,sidp,fgidp)
register proc_t *pp;
vnode_t *vp;
pid_t *sidp;
pid_t *fgidp;
{
	extern vnode_t *makectty();
	register sess_t *sp = pp->p_sessp;
	register struct user *up = PTOU(pp);

	sp->s_vp = makectty(vp->v_rdev);
	sp->s_dev = vp->v_rdev;
	sp->s_sidp = sidp;
	sp->s_fgidp = fgidp;
	*sidp = pp->p_pid;
	*fgidp = pp->p_pid;
	sp->s_uid = pp->p_cred->cr_uid;
	if (session0.s_mode & VSGID)
		sp->s_gid = session0.s_gid;
	else
		sp->s_gid = pp->p_cred->cr_gid;
	sp->s_mode = (0666 & ~(up->u_cmask));

	/* populate the user area for binary compatibility */
	/* NODEV is assigned for device numbers that don't fit in o_dev_t. */

	up->u_ttyp = (pid_t *)sp->s_sidp;
	up->u_ttyd = (o_dev_t)cmpdev(sp->s_dev);

}

#define cantsend(pp,sig) \
	(sigismember(&pp->p_ignore,sig) || sigismember(&pp->p_hold,sig))

/*
 * Perform job control discipline access checks.
 * Return 0 for success and the errno for failure.
 */

int
accsctty(vp, mode, tostop)
vnode_t *vp;
int tostop;
enum jcaccess mode;
{
	register proc_t *pp;
	register sess_t *sp;

	pp = u.u_procp;
	sp = pp->p_sessp;

	for (;;) {

		/* 
		 * if this is not the calling process's controlling terminal
		 * or the calling process is already in the foreground
		 * then allow access
		 */

		if (sp->s_dev != vp->v_rdev || pp->p_pgrp == *sp->s_fgidp)
			return 0;

		/*
		 * if calling process's session ID is not the terminal's
		 * session ID, then either the session leader exited
		 * or allocated a new controlling terminal
		 */

		if (sp->s_sid != *sp->s_sidp) {	/* caller lost ctty */
			if (cantsend(pp,SIGHUP))
				return EIO;
			signal(pp->p_pgrp, SIGHUP);
		} 

		else if (mode == JCGETP)
			return 0;

		else if (mode == JCREAD) {
			if (cantsend(pp,SIGTTIN) || (pp->p_flag & SDETACHED))
				return EIO;
			signal(pp->p_pgrp,SIGTTIN);
		} 

		else {  /* mode == JCWRITE or JCSETP */
			if (mode == JCWRITE && !tostop || cantsend(pp,SIGTTOU))
				return 0;
			if (pp->p_flag & SDETACHED)
				return EIO;
			signal(pp->p_pgrp, SIGTTOU);
		}

		/*
		 * This mimics code in sleep().
		 * This is not a real sleep, but it should
		 * look like one to the debugger interface.
		 */
		pp->p_flag |= SASLEEP;
		u.u_sysabort = 0;
		if (ISSIG(pp, FORREAL) || u.u_sysabort || EV_ISTRAP(pp)) {
			/*
			 * Signal pending or debugger aborted the syscall.
			 */
			pp->p_flag &= ~SASLEEP;
			u.u_sysabort = 0;
			longjmp(&u.u_qsav);
		}
		pp->p_flag &= ~SASLEEP;
	}
}

void
realloctty(vp)
struct vnode *vp;
{
	register proc_t *pp;
	register sess_t *sp;

	pp = u.u_procp;
	sp = pp->p_sessp;

	/* the controlling terminal should be allocated by now */
	/* if so, at least the following must be true	       */

	if (u.u_ttyp == NULL || *u.u_ttyp != pp->p_pid)
		return;

	if (pp->p_pid != sp->s_sid) {
		*u.u_ttyp = 0;
		u.u_ttyp = NULL;
		sp->s_vp = NULL;
		return;
	}

	alloctty(u.u_procp, vp, u.u_ttyp, u.u_ttyp);
}
