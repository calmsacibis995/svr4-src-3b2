/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:disp/disp.c	1.28"
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
#include "sys/cred.h"
#include "sys/proc.h"
#include "sys/procset.h"
#include "sys/debug.h"
#include "sys/inline.h"
#include "sys/priocntl.h"
#include "sys/disp.h"
#include "sys/class.h"
#include "sys/bitmap.h"
#include "sys/kmem.h"

#include "vm/vm_hat.h"	/* XXX - ublock kludge */
#include "vm/as.h"

extern int	mau_present;
extern void	runqueues();
extern char	qrunflag;

#ifdef DEBUG
int idlecntdown = 60;
#endif

int		runrun;		/* scheduling flag - set to cause preemption */
int		kprunrun;	/* set to preempt at next krnl prmption point */
int		npwakecnt;	/* count of npwakeups since last pswtch() */
proc_t		*curproc;	/* currently running process */
int		curpri;		/* priority of current process */
int		maxrunpri;	/* priority of highest priority active queue */

STATIC ulong	*dqactmap;	/* bitmap to keep track of active disp queues */
STATIC dispq_t	*dispq;		/* ptr to array of disp queues indexed by pri */
STATIC int	srunprocs;	/* total number of loaded, runnable procs */


/*
 * Scheduler Initialization
 */
void
dispinit()
{
	register id_t	cid;
	register int	maxglobpri;
	int		cl_maxglobpri;

	maxglobpri = -1;

	/*
	 * Call the class specific initialization functions. We pass the size
	 * of a class specific parameter buffer to each of the initialization
	 * functions to try to catch problems with backward compatibility of
	 * class modules.  For example a new class module running on an old
	 * system which didn't provide sufficiently large parameter buffers
	 * would be bad news.  Class initialization modules can check for
	 * this and take action if they detect a problem.
	 */
	for (cid = 0; cid < nclass; cid++) {
		(*class[cid].cl_init)(cid, PC_CLPARMSZ, &class[cid].cl_funcs,
		    &cl_maxglobpri);
		if (cl_maxglobpri > maxglobpri)
			maxglobpri = cl_maxglobpri;
	}

	v.v_nglobpris = maxglobpri + 1;

	/*
	 * Allocate memory for the dispatcher queue headers
	 * and the active queue bitmap.
	 */
	if ((dispq = (dispq_t *)kmem_zalloc(v.v_nglobpris * sizeof(dispq_t),
	    KM_NOSLEEP)) == NULL)
		cmn_err(CE_PANIC,
		    "Can't allocate memory for dispatcher queues.");

	if ((dqactmap = (ulong *)kmem_zalloc(((v.v_nglobpris / BT_NBIPUL) + 1) *
	    sizeof(long), KM_NOSLEEP)) == NULL)
		cmn_err(CE_PANIC,
		    "Can't allocate memory for dispq active map.");

	srunprocs = 0;
	maxrunpri = -1;
}


/*
 * Preempt the currently running process in favor of the highest
 * priority process.  The class of the current process controls
 * where it goes on the dispatcher queues.
 */
void
preempt()
{
	CL_PREEMPT(u.u_procp, u.u_procp->p_clproc);
	swtch();
}


void
pswtch()
{
	register proc_t		*pp;
	register proc_t		*rp;
	register dispq_t	*dq;
	register proc_t		*old_curproc;
	register int		maxrunword;

	sysinfo.pswitch++;
	old_curproc = pp = curproc;

	switch (pp->p_stat) {
	case SZOMB:

		/*
		 * Free up remaining memory used by the
		 * zombie process here.  This is just
		 * the u-block and the sdt's.
		 */
		segu_release(pp);
		if (pp->p_parent->p_flag & SNOWAIT)
			freeproc(pp);
		break;

	case SONPROC:
		ASSERT(pp->p_wchan == 0);
		pp->p_stat = SRUN;
		break;
	}

	/*
	 * Find the highest priority loaded, runnable process.
	 */
	splhi();
	runrun = kprunrun = npwakecnt = 0;
	while (maxrunpri == -1) {
		if (qrunflag) {
			runqueues();
			continue; /* might have made someone runnable */
		}
		curpri = 0;
		curproc = nproc[0];
#ifdef KPERF
		if (kpftraceflg) {
			asm(" MOVAW 0(%pc),Kpc ");
			kperf_write(KPT_IDLE, Kpc, curproc);
		}
#endif	/* KPERF */
		idle();
		splhi();
		runrun = kprunrun = npwakecnt = 0;
	}
	dq = &dispq[maxrunpri];
	pp = dq->dq_first;
	ASSERT(pp != NULL && pp->p_stat == SRUN);
	while ((pp->p_flag & (SLOAD|SPROCIO)) != SLOAD
	  && (pp->p_flag & SSYS) == 0) {
		rp = pp;
		pp = pp->p_link;
		ASSERT(pp != NULL && pp->p_stat == SRUN);
	}

	/*
	 * Found it so remove it from queue.
	 */
	if (pp == dq->dq_first) {

		/*
		 * We are dequeuing the first proc on the list.
		 * Check for creating a null list.
		 */
		if ((dq->dq_first = pp->p_link) == NULL)
			dq->dq_last = NULL;
	} else {

		/*
		 * We are not dequeuing the first proc on the list.
		 * Check for dequeuing the last one.
		 */
		if ((rp->p_link = pp->p_link) == NULL)
			dq->dq_last = rp;
	}
	srunprocs--;
	if (--dq->dq_sruncnt == 0) {
		maxrunword = maxrunpri >> BT_ULSHIFT;
		dqactmap[maxrunword] &= ~BT_BIW(maxrunpri);
		if (srunprocs == 0)
			maxrunpri = -1;
		else
			bt_gethighbit(dqactmap, maxrunword, &maxrunpri);
	}

	pp->p_stat = SONPROC;	/* process p will be running */
	curpri = pp->p_pri;
	curproc = pp;
#ifdef DEBUG
	if (pp != nproc[3])		/* skip bdflush */
		idlecntdown = 60;
#endif
	
	/*
	 * Switch context only if really process switching.
	 */
	if (pp != old_curproc) {
		if (mau_present)
			mau_save();

		/*
		 * Initialize MMU SRAMS for chosen process.
		 * Note that we're already at splhi here.
		 */
		if (pp->p_as == (struct as *)NULL) {
			srama[SCN2] = mmu_invalid;
			((int *)sramb)[SCN2] = 0;
			((sde_t *)dflt_sdt_p)->wd2.address =
				phys_ubptbl(pp->p_ubptbl);
			srama[SCN3] = dflt_sdt_p;
			((int *)sramb)[SCN3] = 0;
		} else {
			register struct hat *hatp = &pp->p_as->a_hat;

			((sde_t *)hatp->hat_srama[1])->wd2.address =
				phys_ubptbl(pp->p_ubptbl);
			srama[SCN2] = hatp->hat_srama[0];
			sramb[SCN2] = hatp->hat_sramb[0];
			srama[SCN3] = hatp->hat_srama[1];
			sramb[SCN3] = hatp->hat_sramb[1];
		}

		if (mau_present)
			mau_restore();
	}

#ifdef KPERF
	if (kpftraceflg) {
		asm(" MOVAW 0(%pc),Kpc ");
		kperf_write(KPT_PSWTCH, Kpc, curproc);
	}
#endif	/* KPERF */

	(void) spl1();
}


/*
 * Put the specified process on the back of the dispatcher
 * queue corresponding to its current priority.
 */
void
setbackdq(pp)
register proc_t	*pp;
{
	register dispq_t	*dq;
#ifdef DEBUG
	register proc_t		*rp;
#endif
	register int		ppri;
	register int		oldlvl;

	ASSERT(pp->p_stat == SRUN || pp->p_stat == SONPROC);

	oldlvl = splhi();


#ifdef DEBUG
	for (dq = &dispq[0] ; dq < &dispq[v.v_nglobpris] ; dq++) {
		ASSERT(dq->dq_last == NULL || dq->dq_last->p_link == NULL);
		for (rp = dq->dq_first; rp; rp = rp->p_link) {
			if (pp == rp)
				cmn_err(CE_PANIC, "setbackdq - proc on q.");
		}
	}
#endif

	ppri = pp->p_pri;
	dq = &dispq[ppri];
	if (dq->dq_last == NULL) {
		ASSERT(dq->dq_first == NULL);
		pp->p_link = NULL;
		dq->dq_first = dq->dq_last = pp;
	} else {
		ASSERT(dq->dq_first != NULL);
		pp->p_link = NULL;
		dq->dq_last->p_link = pp;
		dq->dq_last = pp;
	}

	if ((pp->p_flag & (SLOAD|SPROCIO)) == SLOAD || (pp->p_flag & SSYS)) {
		srunprocs++;
		if (++dq->dq_sruncnt == 1) {
			BT_SET(dqactmap, ppri);
			if (ppri > maxrunpri)
				maxrunpri = ppri;
		}
	}
	splx(oldlvl);
}


/*
 * Put the specified process on the front of the dispatcher
 * queue corresponding to its current priority.
 */
void
setfrontdq(pp)
register proc_t	*pp;
{
	register dispq_t	*dq;
#ifdef DEBUG
	register proc_t		*rp;
#endif
	register int		ppri;
	register int		oldlvl;

	ASSERT(pp->p_stat == SRUN || pp->p_stat == SONPROC);

	oldlvl = splhi();

#ifdef DEBUG
	for (dq = &dispq[0] ; dq < &dispq[v.v_nglobpris] ; dq++) {
		ASSERT(dq->dq_last == NULL || dq->dq_last->p_link == NULL);
		for (rp = dq->dq_first; rp; rp = rp->p_link) {
			if (pp == rp)
				cmn_err(CE_PANIC, "setfrontdq - proc on q.");
		}
	}
#endif

	ppri = pp->p_pri;
	dq = &dispq[ppri];
	if (dq->dq_first == NULL) {
		ASSERT(dq->dq_last == NULL);
		pp->p_link = NULL;
		dq->dq_first = dq->dq_last = pp;
	} else {
		ASSERT(dq->dq_last != NULL);
		pp->p_link = dq->dq_first;
		dq->dq_first = pp;
	}

	if ((pp->p_flag & (SLOAD|SPROCIO)) == SLOAD || (pp->p_flag & SSYS)) {
		srunprocs++;
		if (++dq->dq_sruncnt == 1) {
			BT_SET(dqactmap, ppri);
			if (ppri > maxrunpri)
				maxrunpri = ppri;
		}
	}
	splx(oldlvl);
}


/*
 * Remove a process from the dispatcher queue if it is on it.
 * It is not an error if it is not found but we return whether
 * or not it was found in case the caller wants to check.
 */
boolean_t
dispdeq(pp)
register proc_t		*pp;
{
	register dispq_t	*dq;
	register proc_t		*rp;
	register proc_t		*prp;
	register int		ppri;
	int			oldlvl;

	oldlvl = splhi();
	ppri = pp->p_pri;
	dq = &dispq[ppri];
	rp = dq->dq_first;
	prp = NULL;

	ASSERT(dq->dq_last == NULL  ||  dq->dq_last->p_link == NULL);

	while (rp != pp && rp != NULL) {
		prp = rp;
		rp = prp->p_link;
	}
	if (rp == NULL) {

#ifdef DEBUG
		for(dq = &dispq[0] ; dq < &dispq[v.v_nglobpris]; dq++) {
			ASSERT(dq->dq_last == NULL
			  || dq->dq_last->p_link == NULL);
			for(rp = dq->dq_first; rp; rp = rp->p_link) {
				if (pp == rp)
					cmn_err(CE_PANIC,
					"dispdeq - proc %x on wrong q %x.",
						pp, dq);
			}
		}
#endif
		splx(oldlvl);
		return(B_FALSE);
	}

	/*
	 * Found it so remove it from queue.
	 */
	ASSERT(dq - dispq == rp->p_pri);

	if (prp == NULL) {

		/*
		 * We are dequeueing the first proc on the list.
		 * Check for creating a null list.
		 */
		if ((dq->dq_first = rp->p_link) == NULL)
			dq->dq_last = NULL;
	} else {

		/*
		 * We are not dequeueing the first proc on the list.
		 * Check for dequeueing the last one.
		 */
		if ((prp->p_link = rp->p_link) == NULL)
			dq->dq_last = prp;
	}

	if ((rp->p_flag & (SLOAD|SPROCIO)) == SLOAD || (rp->p_flag & SSYS)) {
		srunprocs--;
		if (--dq->dq_sruncnt == 0) {
			dqactmap[ppri >> BT_ULSHIFT] &= ~BT_BIW(ppri);
			if (srunprocs == 0)
				maxrunpri = -1;
			else if (ppri == maxrunpri)
				bt_gethighbit(dqactmap, maxrunpri >> BT_ULSHIFT,
				    &maxrunpri);
		}
	}

	splx(oldlvl);
	return(B_TRUE);
}


/*
 * dq_sruninc and dq_srundec are public functions for
 * incrementing/decrementing the sruncnts when a process on
 * a dispatcher queue is made schedulable/unschedulable by
 * resetting the SLOAD or SPROCIO flags.
 * The caller MUST set splhi() such that the operation which changes
 * the flag, the operation that checks the status of the process to
 * determine if it's on a disp queue AND the call to this function
 * are one atomic operation with respect to interrupts.
 * We don't set splhi() only because we trust that the caller has.
 */
void
dq_sruninc(pri)
int	pri;
{
	register dispq_t	*dq;

	srunprocs++;
	dq = &dispq[pri];
	if (++dq->dq_sruncnt == 1) {
		BT_SET(dqactmap, pri);
		if (pri > maxrunpri)
			maxrunpri = pri;
	}
}


/*
 * See comment on calling conventions above.
 */
void
dq_srundec(pri)
int	pri;
{
	register dispq_t	*dq;

	srunprocs--;
	dq = &dispq[pri];
	if (--dq->dq_sruncnt == 0) {
		dqactmap[pri >> BT_ULSHIFT] &= ~BT_BIW(pri);
		if (srunprocs == 0)
			maxrunpri = -1;
		else if (pri == maxrunpri)
			bt_gethighbit(dqactmap, maxrunpri >> BT_ULSHIFT,
			    &maxrunpri);
	}
}


/*
 * Get class ID given class name.
 */
int
getcid(clname, cidp)
char	*clname;
id_t	*cidp;
{
	register class_t	*clp;

	for (clp = &class[0]; clp < &class[nclass]; clp++) {
		if (strcmp(clp->cl_name, clname) == 0) {
			*cidp = clp - &class[0];
			return(0);
		}
	}
	return(EINVAL);
}


/*
 * Get the global scheduling priority associated with a set of
 * scheduling parameters.  The global priority is returned
 * in *globprip.  As you can see the class specific code does
 * the work.  This function simply provides a class independent
 * interface.
 */
void
getglobpri(parmsp, globprip)
pcparms_t	*parmsp;
int		*globprip;
{
	CL_GETGLOBPRI(&class[parmsp->pc_cid], parmsp->pc_clparms, globprip);
}


/*
 * Get the scheduling parameters of the process pointed to by
 * pp into the buffer pointed to by parmsp.
 */
void
parmsget(pp, parmsp)
proc_t		*pp;
pcparms_t	*parmsp;
{
	parmsp->pc_cid = pp->p_cid;
	CL_PARMSGET(pp, pp->p_clproc, parmsp->pc_clparms);
}


/*
 * Check the validity of the scheduling parameters in the buffer
 * pointed to by parmsp. If our caller passes us non-NULL process
 * pointers we are also being asked to verify that the requesting
 * process (pointed to by reqpp) has the necessary permissions to
 * impose these parameters on the target process (pointed to by
 * targpp).
 * We check validity before permissions because we assume the user
 * is more interested in finding out about invalid parms than a
 * permissions problem.
 * Note that the format of the parameters may be changed by class
 * specific code which we call.
 */
int
parmsin(parmsp, reqpp, targpp)
register pcparms_t	*parmsp;
register proc_t		*reqpp;
register proc_t		*targpp;
{
	register int		error;
	id_t			reqpcid;
	register cred_t		*reqpcredp;
	id_t			targpcid;
	register cred_t		*targpcredp;
	register caddr_t	targpclpp;

	if (parmsp->pc_cid >= nclass || parmsp->pc_cid < 1)
		return(EINVAL);

	if (reqpp != NULL && targpp != NULL) {
		reqpcid = reqpp->p_cid;
		reqpcredp = reqpp->p_cred;
		targpcid = targpp->p_cid;
		targpcredp = targpp->p_cred;
		targpclpp = targpp->p_clproc;
	} else {
		reqpcredp = targpcredp = NULL;
		targpclpp = NULL;
	}

	/*
	 * Call the class specific routine to validate class
	 * specific parameters.  Note that the data pointed to
	 * by targpclpp is only meaningful to the class specific
	 * function if the target process belongs to the class of
	 * the function.
	 */
	error = CL_PARMSIN(&class[parmsp->pc_cid], parmsp->pc_clparms,
		reqpcid, reqpcredp, targpcid, targpcredp, targpclpp);
	if (error)
		return(error);

	if (reqpcredp != NULL)
		/*
		 * Check the basic permissions required for all classes.
		 */
		if (!hasprocperm(targpcredp, reqpcredp))
			return(EPERM);
	return(0);
}

	
/*
 * Call the class specific code to do the required processing
 * and permissions checks before the scheduling parameters
 * are copied out to the user.
 * Note that the format of the parameters may be changed by the
 * class specific code.
 */
int
parmsout(parmsp, reqpp, targpp)
register pcparms_t	*parmsp;
register proc_t		*reqpp;
register proc_t		*targpp;
{
	register int	error;
	id_t		reqpcid;
	register cred_t	*reqpcredp;
	id_t		targpcid;
	register cred_t	*targpcredp;

	reqpcid = reqpp->p_cid;
	reqpcredp = reqpp->p_cred;
	targpcid = targpp->p_cid;
	targpcredp = targpp->p_cred;

	error = CL_PARMSOUT(&class[parmsp->pc_cid], parmsp->pc_clparms,
		reqpcid, reqpcredp, targpcredp);

	return(error);
}


/*
 * Set the scheduling parameters of the process pointed to by
 * targpp to those specified in the pcparms structure pointed
 * to by parmsp.  If reqpp is non-NULL it points to the process
 * that initiated the request for the parameter change and indicates
 * that our caller wants us to verify that the requesting process
 * has the appropriate permissions.
 */
int
parmsset(parmsp, reqpp, targpp)
register pcparms_t	*parmsp;
register proc_t		*reqpp;
register proc_t		*targpp;
{
	caddr_t			clprocp;
	register int		error;
	register id_t		reqpcid;
	register cred_t		*reqpcredp;
	int			oldlvl;

	if (reqpp != NULL) {
		reqpcid = reqpp->p_cid;
		reqpcredp = reqpp->p_cred;

		/*
		 * Check basic permissions.
		 */
		if (!hasprocperm(targpp->p_cred, reqpcredp))
			return(EPERM);
	} else {
		reqpcredp = NULL;
	}
	
	if (parmsp->pc_cid != targpp->p_cid) {

		/*
		 * Target process must change to new class.
		 */
		error = CL_ENTERCLASS(&class[parmsp->pc_cid],
		    parmsp->pc_clparms, targpp, &targpp->p_stat, &targpp->p_pri,
		    &targpp->p_flag, &targpp->p_cred, &clprocp, reqpcid,
		    reqpcredp);
		if (error)
			return(error);
		else {

			/*
			 * Change to new class successful so release resources
			 * for old class and complete change to new one.
			 */
			oldlvl = splhi();
			CL_EXITCLASS(targpp, targpp->p_clproc);
			targpp->p_cid = parmsp->pc_cid;
			targpp->p_clfuncs = class[parmsp->pc_cid].cl_funcs;
			targpp->p_clproc = clprocp;
			splx(oldlvl);
		}
	} else {

		/*
		 * Not changing class
		 */
		error = CL_PARMSSET(targpp, parmsp->pc_clparms,
		    targpp->p_clproc, reqpcid, reqpcredp);
		if (error)
			return(error);
	}
	return(0);
}
