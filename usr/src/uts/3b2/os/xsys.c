/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)kernel:os/xsys.c	1.11"

/* #ifdef MERGE */

#include "sys/param.h"
#include "sys/types.h"
#include "sys/immu.h"
#include "sys/signal.h"
#include "sys/proc.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/time.h"
#include "sys/locking.h"
#include "sys/fcntl.h"
#include "sys/systm.h"
#include "sys/timeb.h"
#include "sys/flock.h"
#include "sys/conf.h"
#include "sys/fstyp.h"
#include "sys/sysmacros.h"
#include "sys/vnode.h"
#include "sys/file.h"
#include "sys/proctl.h"
#include "sys/var.h"
#include "sys/cmn_err.h"

#undef	wakeup
extern u_int	timer_resolution;
extern void	wakeup();	/* reference the function, not the macro */

/*
 *	Nap for the specified number of milliseconds.
 */
struct napa {
	long msec;
};

nap(uap, rvp)
	struct napa *uap;
	rval_t *rvp;
{
	extern clock_t  lbolt;
	clock_t fst = 0, lst;
	long togo;
	int ospl;

	/* Make sure no clock interrupt between reading time and lbolt.
	 * spl is not sufficient here, since we could be running on
	 * a 'slave' cpu, and the master could take a clock interrupt.
	 * We therefore check lbolt twice and make sure it is the same.
	 */
	while(fst != lbolt) 
		fst = lbolt;

	/* preclude overflow */
	if (uap->msec >= LONG_MAX/timer_resolution || uap->msec < 0)
		return EINVAL;
	/* togo gets time to nap in ticks */
	if ((togo = uap->msec * timer_resolution / 1000) < 0)
		return EINVAL;
	
	lst = fst + togo;

	/* nap, return time napped */
	while(togo > 0) {	/* now handle short part of nap */
		ospl = splhi();
		(void)timeout(wakeup, (caddr_t)u.u_procp, togo);
		(void)sleep((caddr_t)u.u_procp, PSLEP);
		splx(ospl);
		togo = lst - lbolt; 
	}
	rvp->r_time = (lbolt-fst) * 1000L / timer_resolution ;
	return 0;
}

/*
 * Return TOD with milliseconds, timezone, DST flag
 */
struct ftimea {
	struct timeb *tp;
};

/* ARGSUSED */
ftime(uap, rvp)
	struct ftimea *uap;
	rval_t *rvp;
{
	struct timeb t;
	register unsigned ms = 0;
	/* 
	 *	The meaning of lticks changed in 5.3. Used
	 *	to be the number of ticks until next second; is now
	 *	a rescheduling variable. This has been change to use
	 *	lbolt, which is the total time accumulation since startup
	 *	in ticks.
	 */
	extern time_t lbolt;

	/*	make sure no clock interrupt between reading time and lbolt,
		spl is not sufficient here, since we could be running on
		a 'slave' cpu, and the master could take a clock interrupt.
		We therefore check lbolt twice and make sure it is the same.
	*/
	while(ms != lbolt) {
		ms = lbolt;
		t.time = hrestime.tv_sec;
	}

	/* new calculation using lbolt.	*/
	t.millitm = (unsigned) (ms % timer_resolution)*(1000/timer_resolution);
	t.timezone = Timezone;
	t.dstflag = Dstflag;
	if (copyout((caddr_t) &t, (caddr_t)uap->tp, sizeof(t)) == -1)
		return EFAULT;
	return 0;
}


/*
 * proctl system call (process control)
 */
struct proctla {
	pid_t	pid;
	int	cmd;
	char	*arg;
};

/* ARGSUSED */
proctl(uap, rvp)
	struct proctla *uap;
	rval_t *rvp;
{
	register struct proc **p, *q;
	register pid_t pid;
	int found = 0;
	register struct cred *pcred, *ucred;

	
	pid = uap->pid;
	if (pid > 0)
		p = &nproc[1];
	else
		p = &nproc[2];
	q = u.u_procp;
	if (pid == 0 && q->p_pgrp == 0)
		return ESRCH;
	for(; p < v.ve_proc; p++) {
		if (*p == NULL || (*p)->p_stat == NULL)
			continue;
		if (pid > 0 && (*p)->p_pid != pid)
			continue;
		if (pid == 0 && (*p)->p_pgrp != q->p_pgrp)
			continue;
		if (pid < -1 && (*p)->p_pgrp != -pid)
			continue;
		ucred = u.u_cred;
		pcred = (*p)->p_cred;
			
		if (ucred->cr_uid != 0 && ucred->cr_uid != pcred->cr_uid && ucred->cr_ruid != pcred->cr_uid
			 && ucred->cr_uid != pcred->cr_suid && ucred->cr_ruid != pcred->cr_suid 
			 && u.u_procp != *p)
			if (pid > 0) {
				return EPERM;
			} else
				continue;
		found++;
		switch(uap->cmd) {
			case PRHUGEX:
			case PRNORMEX:
				/*
				 * A no-op.  Cannot really maintain backwards
				 * compatibility here without adding an
				 * unused field to the user struct, so
				 * we'll have to punt.
				 */
				break;
			default:
				return EINVAL;
		}
		if (pid > 0)
			break;
	}
	if (found == 0)
		return ESRCH;
	return 0;
}
