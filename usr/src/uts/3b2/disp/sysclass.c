/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:disp/sysclass.c	1.11"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/psw.h"
#include "sys/sysmacros.h"
#include "sys/fs/s5dir.h"
#include "sys/signal.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/immu.h"
#include "sys/systm.h"
#include "sys/sysinfo.h"
#include "sys/var.h"
#include "sys/errno.h"
#include "sys/cmn_err.h"
#include "sys/proc.h"
#include "sys/debug.h"
#include "sys/inline.h"
#include "sys/disp.h"
#include "sys/class.h"


/*
 * Class specific code for the sys class. There are no
 * class specific data structures associated with
 * the sys class and the scheduling policy is trivially
 * simple. There is no time slicing and priority is
 * only changed when the process requests this through the
 * disp argument to sleep().
 */

void		sys_init();
STATIC int	sys_admin(), sys_enterclass(), sys_fork(), sys_getclinfo();
STATIC int	sys_parmsin(), sys_parmsout(), sys_parmsset(), sys_proccmp();
STATIC void	sys_forkret(), sys_nullclass(), sys_preempt(), sys_setrun();
STATIC void	sys_sleep(), sys_wakeup();

STATIC struct classfuncs sys_classfuncs = {
	sys_admin,
	sys_enterclass,
	sys_nullclass,
	sys_fork,
	sys_forkret,
	sys_getclinfo,
	sys_nullclass,
	sys_nullclass,
	sys_parmsin,
	sys_parmsout,
	sys_parmsset,
	sys_preempt,
	sys_proccmp,
	sys_setrun,
	sys_sleep,
	sys_nullclass,
	sys_nullclass,
	sys_nullclass,
	sys_nullclass,
	sys_nullclass,
	sys_wakeup,
	sys_nullclass,
	sys_nullclass,
	sys_nullclass,
	sys_nullclass,
	sys_nullclass,
	sys_nullclass,
	sys_nullclass,
	sys_nullclass,
	sys_nullclass,
	sys_nullclass,
	sys_nullclass
};


/*
 * The following several functions are no-ops for the sys class.
 * They will never be called but they make things build nicely.
 */
STATIC int
sys_admin()
{
	return(0);
}


STATIC int
sys_enterclass()
{
	return(0);
}

	
STATIC int
sys_getclinfo()
{
	return(0);
}

	
STATIC int
sys_parmsin()
{
	return(0);
}


STATIC int
sys_parmsout()
{
	return(0);
}


STATIC int
sys_parmsset()
{
	return(0);
}


STATIC int
sys_proccmp()
{
	return(0);
}


/* ARGSUSED */
void
sys_init(cid, clparmsz, clfuncspp, maxglobprip)
id_t		cid;
int		clparmsz;
classfuncs_t	**clfuncspp;
int		*maxglobprip;
{
	*clfuncspp = &sys_classfuncs;

	if (v.v_maxsyspri < PSLEP) {
		cmn_err(CE_WARN, "Max system class priority must be >= %d, configured value is %d\n- resetting v.v_maxsyspri to %d\n", PSLEP, v.v_maxsyspri, PSLEP);
		v.v_maxsyspri = PSLEP;
	}
	*maxglobprip = v.v_maxsyspri;
}


/* ARGSUSED */
STATIC int
sys_fork(pprocp, cprocp, cpstatp, cpprip, cpflagp, cpcredpp, procpp)
proc_t	*pprocp;
proc_t	*cprocp;
char	*cpstatp;
short	*cpprip;
uint	*cpflagp;
struct cred	**cpcredpp;
proc_t	**procpp;
{
	/*
	 * No class specific data structure so make the proc's
	 * class specific data pointer point back to the
	 * generic proc structure.
	 */
	*procpp = cprocp;
	return(0);
}


/* ARGSUSED */
STATIC void
sys_forkret(cprocp, pprocp)
proc_t	*cprocp;
proc_t	*pprocp;
{
	setbackdq(cprocp);
}


STATIC void
sys_nullclass()
{
}


STATIC void
sys_preempt(pp)
proc_t	*pp;
{
	setfrontdq(pp);
}


STATIC void
sys_setrun(pp)
proc_t	*pp;
{
	setbackdq(pp);
}


/* ARGSUSED */
STATIC void
sys_sleep(pp, chan, disp)
proc_t	*pp;
caddr_t	chan;
int	disp;
{
	register int	tmpdisp;

	tmpdisp = disp & PMASK;
	pp->p_pri = v.v_maxsyspri - tmpdisp;
}


/* ARGSUSED */
STATIC void
sys_wakeup(pp, preemptflg)
proc_t	*pp;
int	preemptflg;
{
	setbackdq(pp);
}
