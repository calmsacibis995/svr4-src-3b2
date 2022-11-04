/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/vm_meter.c	1.11"

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/debug.h"
#include "sys/systm.h"
#include "sys/time.h"
#include "sys/proc.h"
#include "vm/kernel.h"
#include "sys/vmsystm.h"
#include "sys/vm.h"

#include "vm/hat.h"
#include "vm/as.h"
#include "vm/rm.h"
#include "sys/var.h"

/*
 * This define represents the number of
 * useful pages transferred per paging i/o operation, under the assumption
 * that half of the total number is actually useful.  However, if there's
 * only one page transferred per operation, we assume that it's useful.
 */

/* insure non-zero */
#define	nz(x)	((x) != 0 ? (x) : 1)

#ifdef	lint
#define	UsefulPagesPerIO	1
#else	/* lint */
#define	UsefulPagesPerIO	nz((MAXBSIZE/PAGESIZE)/2)
#endif	/* lint */

extern int dopageout;
extern int lotsfree;

#if defined(__STDC__)
STATIC void loadav(long *, int);
#else
STATIC void loadav();
#endif


/* called once a second (by schedcpu) to gather statistics */

void
vmmeter()
{
	register unsigned *cp, *rp, *sp;

	if (!lotsfree || !dopageout) return;
	/*
	 * Decay deficit by the expected number of pages brought in since
	 * the last call (i.e., in the last second).  The calculation
	 * assumes that one half of the pages brought in are actually
	 * useful (see comment above), and that half of the overall
	 * paging i/o activity is pageins as opposed to pageouts (the
	 * trailing factor of 2)  It also assumes that paging i/o is done
	 * in units of MAXBSIZE bytes, which is a dubious assumption to
	 * apply to all file system types.
	 */
	deficit -= MIN(deficit,
	    MAX(deficit / 10, UsefulPagesPerIO * maxpgio / 2));

	ave(avefree, freemem, 5);
	ave(avefree30, freemem, 30);
	cp = &cnt.v_first; rp = &rate.v_first; sp = &sum.v_first;
	while (cp <= &cnt.v_last) {
		ave(*rp, *cp, 5);
		*sp += *cp;
		*cp = 0;
		rp++, cp++, sp++;
	}
#ifdef VAC
	/* compute flush_rate and flush_sum */
	cp = &flush_cnt.f_first;
	rp = &flush_rate.f_first;
	sp = &flush_sum.f_first;
	while (cp <= &flush_cnt.f_last) {
		ave(*rp, *cp, 5);
		*sp += *cp;
		*cp = 0;
		rp++, cp++, sp++;
	}
#endif /* VAC */
	if (hrestime.tv_sec % 5 == 0) {
		vmtotal();
		rate.v_swpin = cnt.v_swpin;
		sum.v_swpin += cnt.v_swpin;
		cnt.v_swpin = 0;
		rate.v_swpout = cnt.v_swpout;
		sum.v_swpout += cnt.v_swpout;
		cnt.v_swpout = 0;
	}
	if (avefree < minfree && runout
	   /* || nproc[0]->p_slptime > maxslp/2 */
	    ) {
		runout = 0;
		runin = 0;
		wakeup((caddr_t)&runin);
		wakeup((caddr_t)&runout);
	}
}

/* Capture (expensive) statistics every 5 seconds.
 * Note that process state accounting is a little muddy and incomplete --
 *  eg, we consider processes to be in the run queue (nrun++)
 *  even if they're in diskwait, and we ignore long stopped processes.
 * Note that there are statistics in vmtotal that are no longer meaningful
 *  (eg, t_vmtxt), or are not yet (re)implemented (eg, t_vm).
 * Note that there are two run queues: avenrun (reported by, eg, w),
 *  and t_rq (reported by vmstat).
 * And as vmstat(8) points out, "active" means the process
 *  has been active in the last 20 (maxslp) seconds.
 */
void
vmtotal()
{
	register struct proc **p;
	int nrun = 0;

	bzero((caddr_t)&total, sizeof (total));
	for (p = &nproc[0]; p < v.ve_proc; p++) {
		if ( *p == NULL )	/* NOT USED */
			continue;
		if ((*p)->p_flag & SSYS)
			continue;
		total.t_rm += rm_asrss((*p)->p_as);
		switch ((*p)->p_stat) {

		case SSLEEP:
		case SSTOP:
			if ((*p)->p_pri <= PZERO)
				nrun++;
			if ((*p)->p_flag & SLOAD) {
				if ((*p)->p_pri <= PZERO)
					total.t_dw++;
				else /* if ((*p)->p_slptime < maxslp) */
					total.t_sl++;
			} else /* if ((*p)->p_slptime < maxslp) */
				total.t_sw++;
			/* if ((*p)->p_slptime < maxslp)
				goto active;
			*/
			break;

		case SRUN:
		case SIDL:
			nrun++;
			if ((*p)->p_flag & SLOAD)
				total.t_rq++;
			else
				total.t_sw++;
active:
			total.t_arm += rm_asrss((*p)->p_as);
			break;
		}
	}
	total.t_vm += total.t_vmtxt;
	total.t_avm += total.t_avmtxt;
	total.t_rm += total.t_rmtxt;
	total.t_arm += total.t_armtxt;
	total.t_free = (short)avefree;
	loadav(avenrun, nrun);
}

/*
 * Constants for averages over 1, 5, and 15 minutes
 * when sampling at 5 second intervals.
 *
 * Although the constants are chosen for the indicated exponential
 * decay, they are very close approximations to the values
 * of another (obvious) running average approximation of size N,
 *   avg[new] ==  ((N-1)/N)*avg[old] + (1/N)*value,
 * since exp(-1/N) and ((N-1)/N) are very close to (within 1% of)
 * each other for N > 7.
 */

/* defines that Sun has in param.h but we don't right now */
#define FSHIFT	8		/* bit to right of fixed binary point */
#define FSCALE	(1<<FSHIFT)

STATIC long cexp[3] = {
#ifdef vax
/* vax C compiler won't convert float to int for initialization */
	92*FSCALE/100,	/* exp(-1/12) */
	98*FSCALE/100,	/* exp(-1/60) */
	99*FSCALE/100,	/* exp(-1/180) */
#else
	0.9200444146293232 * FSCALE,	/* exp(-1/12) */
	0.9834714538216174 * FSCALE,	/* exp(-1/60) */
	0.9944598480048967 * FSCALE,	/* exp(-1/180) */
#endif
};

/*
 * Compute a tenex style load average of a quantity on
 * 1, 5 and 15 minute intervals.
 * NB: avg is kept as a scaled (by FSCALE) long as well as is cexp.
 */
STATIC void
loadav(avg, n)
	register long *avg;
	int n;
{
	register int i;

	for (i = 0; i < 3; i++)
		avg[i] = (cexp[i] * avg[i] + n * FSCALE * (FSCALE - cexp[i])) >>
		    FSHIFT;
}
