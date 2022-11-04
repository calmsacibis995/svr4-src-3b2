/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/clock.c	1.31.1.10"

#include "sys/param.h"
#include "sys/types.h"
#include "sys/tuneable.h"
#include "sys/psw.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/sysinfo.h"
#include "sys/callo.h"
#include "sys/signal.h"
#include "sys/sbd.h"
#include "sys/pcb.h"
#include "sys/immu.h"
#include "sys/user.h"
#include "sys/evecb.h"
#include "sys/hrtcntl.h"
#include "sys/conf.h"
#include "sys/proc.h"
#include "sys/var.h"
#include "sys/cmn_err.h"
#include "sys/map.h"
#include "sys/swap.h"
#include "sys/inline.h"
#include "sys/disp.h"
#include "sys/class.h"
#include "sys/fs/rf_acct.h"
#include "sys/time.h"
#include "sys/debug.h"

#include "vm/anon.h"
#include "vm/rm.h"

/*
 * clock is called straight from
 * the real time clock interrupt.
 *
 * Functions:
 *	reprime clock
 *	implement callouts
 *	maintain user/system times
 *	maintain date
 *	profile
 *	alarm clock signals
 *	jab the scheduler
 */

#define	PRF_ON	01
unsigned	prfstat;
extern int	fsflush();
extern struct buf *bclnlist;
extern int desfree;

extern int	tickdelta;
extern long	timedelta;
extern int	doresettodr;

extern int (*io_poll[])();	/* driver entry points to poll every tick */

time_t	time;		/* time in seconds since 1970 */
			/* is here only for compatibility */
timestruc_t hrestime;	/* time in seconds and nsec since since 1970 */
clock_t	lbolt;		/* time in HZ since last boot */

int	one_sec = 1;
int	fsflushcnt;	/* counter for t_fsflushr */

int	calllimit	= -1;	/* index of last valid entry in table */

#ifdef DEBUG
int	catchmenowcnt;		/* counter for debuging interrupt */
int	catchmestart = 60;	/* counter for debuging interrupt */
#endif
#ifdef DEBUG
int idlecntdown;
int idlemsg;
#endif

int
clock(pc, ps)
	caddr_t pc;
	psw_t ps;
{
	extern	void	clkreld();
	register struct proc *pp,**ppp;
	register int	retval;
	register rlim_t rlim_cur;
	static rqlen, sqlen;
	extern caddr_t waitloc;
	register int (**pollptr)();	/* pointer to next poll point */

	retval = 0;

	/*
	 * If panic, clock should be stopped.
	 */
	ASSERT(panicstr == NULL);

	/*
	 * XENIX Compatibility Change:
	 *  Call the device driver entries for poll on clock ticks,
	 *  if there are any.  This table (io_poll) is created by
	 *  "cunix" for drivers that contain a "poll" routine.
	 */
	for (pollptr = &io_poll[0];  *pollptr;  pollptr++)
		(**pollptr)();

	/*
	 * Service timeout() requests if any are due at this time.
	 * This code is simpler than the original array-based callout
	 * table since we are using absolute times in the table.
	 * No need to decrement relative times; merely see if the first
	 * (earliest) entry is due to be called.
	 */

	if ((calllimit >= 0) && (callout[0].c_time <= lbolt))
		timepoke();

	if (prfstat & PRF_ON)
		prfintr((u_int)pc, ps);
	pp = u.u_procp;
	if (USERMODE(ps)) {
		sysinfo.cpu[CPU_USER]++;
		pp->p_utime++;
		if (u.u_prof.pr_scale & ~1)
			retval = 1;
		/*
		 * Enforce CPU rlimit.
		 */
		rlim_cur = u.u_rlimit[RLIMIT_CPU].rlim_cur;
		if ((rlim_cur != RLIM_INFINITY) &&
		    ((pp->p_utime/HZ) + (pp->p_stime/HZ) > rlim_cur))
			psignal(pp, SIGXCPU);
	} else {
		if (pc == waitloc) {
			if (syswait.iowait+syswait.swap+syswait.physio) {
				sysinfo.cpu[CPU_WAIT]++;
				if (syswait.iowait)
					sysinfo.wait[W_IO]++;
				if (syswait.swap)
					sysinfo.wait[W_SWAP]++;
				if (syswait.physio)
					sysinfo.wait[W_PIO]++;
			} else {
				sysinfo.cpu[CPU_IDLE]++;
			}
		} else {
			sysinfo.cpu[CPU_KERNEL]++;
			pp->p_stime++;
			/*
			 * Enforce CPU rlimit.
			 */
			rlim_cur = u.u_rlimit[RLIMIT_CPU].rlim_cur;
			if ((rlim_cur != RLIM_INFINITY) &&
			    ((pp->p_utime/HZ) + (pp->p_stime/HZ) > rlim_cur))
				psignal(pp, SIGXCPU);
		}
	}

	/*
	 * Update memory usage for the currently running process.
	 */
	if (pp->p_stat == SONPROC) {
		u.u_mem = rm_asrss(pp->p_as);

		/*
		 * Call the class specific function to do it's 
	 	 * once-per-tick processing for the current process.
	 	 */
		CL_TICK(pp, pp->p_clproc);
	}

	lbolt++;	/* time in ticks */

	/*
	 * "double" long arithmetic for minfo.freemem.
	 */
	if (!BASEPRI(ps)) {
		unsigned long ofrmem;

		ofrmem = minfo.freemem[0];
		minfo.freemem[0] += freemem;
		if (minfo.freemem[0] < ofrmem)
			minfo.freemem[1]++;
	}
	
	/*
	 * Increment the time-of-day
         */
	if (timedelta == 0) {
		BUMPTIME(&hrestime, TICK, one_sec);
	} else {
		/*
		 * Drift clock if necessary.
		 */
		register long delta;

		if (timedelta < 0) {
			/*
			 * Want more ticks per second, because
			 * we want the clock to advance more
			 * slowly.
			 */
			delta = (MICROSEC/HZ) - tickdelta;
			timedelta += tickdelta;
		} else {
			/*
			 * Speed up clock: fewer ticks
			 * per second.
			 */
			delta = (MICROSEC/HZ) + tickdelta;
			timedelta -= tickdelta;
		}	
		/*
		 * Convert from msecs to nsecs
 		 */
		delta *= (NANOSEC/MICROSEC);
		BUMPTIME(&hrestime, delta, one_sec);
		if (-tickdelta < timedelta && timedelta < tickdelta) {
			/*
			 * We're close enough.
			 */
			timedelta = 0;
			if (doresettodr) {
				doresettodr = 0;
				wtodc();
			}
		}
	}
	if (one_sec) {

		time++;

		if (BASEPRI(ps))
			return retval;

#ifdef DEBUG
            	if (idlemsg && --idlecntdown == 0)
                        cmn_err(CE_WARN, "System is idle\n");
#endif

		minfo.freeswap = anoninfo.ani_free;

		rqlen = 0;
		sqlen = 0;
		for (ppp = &nproc[0]; ppp < v.ve_proc; ppp++) {
			if (*ppp == NULL)	/*not used */
				continue;

			if ((*ppp)->p_stat) {
				if ((*ppp)->p_clktim)
					if (--(*ppp)->p_clktim == 0)
						psignal((*ppp), SIGALRM);
				(*ppp)->p_cpu >>= 1;
	
				if ((*ppp)->p_stat == SRUN || (*ppp)->p_stat == SONPROC)
					if ((*ppp)->p_flag & SLOAD)
						rqlen++;
					else
						sqlen++;
			}
		}
		if (rqlen) {
			sysinfo.runque += rqlen;
			sysinfo.runocc++;
		}
		if (sqlen) {
			sysinfo.swpque += sqlen;
			sysinfo.swpocc++;
		}
#ifdef DEBUG
		/*
                 * call this routine at regular intervals
                 * to allow debugging.
                 */
                if (--catchmenowcnt <= 0) {
                        catchmenowcnt = catchmestart;
                        catchmenow();
		}
#endif

		/*
		 * Wake up fsflush to write out DELWRI
		 * buffers, dirty pages and other cached
		 * administrative data, e.g. inodes.
		 */
		if (--fsflushcnt <= 0) {
			fsflushcnt = tune.t_fsflushr;
			wakeup((caddr_t)fsflush);
		}
		/*
		 * XXX
		 * All VFSs should have a VFS_CLOCK operation called from
		 * here.
		 */
		rf_clock(pc, ps);
		vmmeter();
		if (runin != 0) {
			runin = 0;
			setrun(nproc[0]);
		}
                if (((freemem <= tune.t_gpgslo) || sqlen) && runout != 0) {
                        runout = 0;
                        setrun(nproc[0]);
		}
		if (bclnlist != NULL || freemem < desfree) {
			wakeup((caddr_t)nproc[2]);
		}
		one_sec = 0;
	}
	return retval;
}

/*
 * Timeout(), untimeout(), timein(), heap_up(), heap_down():
 *
 * These routines manage the callout table as a heap.  The interfaces
 * and table structure are identical to the standard array-based version;
 * the routines impose the heap structure internally to improve 
 * the overhead of using timein(), timeout(), and untimeout() when the
 * table has more than 2 or 3 entries.
 */

int	timeid		= 0;	/* unique sequence number for entry id */

int
timeout(fun, arg, tim)
	void (*fun)();
	caddr_t arg;
	long tim;
/*
 * Timeout() is called to arrange that fun(arg) be called in tim/HZ seconds.
 * An entry is added to the callout heap structure.  The time in each structure
 * entry is the absolute time at which the function should be called (compare
 * with the relative timing scheme used in the standard array-based version).
 *
 * The panic is there because there is nothing intelligent to be done if
 * an entry won't fit.
 */
{
	register struct	callo	*p1;	/* pointer to entry we are adding */
	register int	j;		/* index to entry we are adding */
	int	t;			/* absolute time fun should be called */
	int	id;			/* id of the entry added */
	int	s;			/* temp variable for spl() */

	t = lbolt + tim;		/* absolute time in the future */

	s = spl7();

	if ((j = calllimit + 1) == v.v_call)
		cmn_err(CE_PANIC,"Timeout table overflow");

	/*
	 * We add the new entry into the next empty slot in the
	 * array representation of the heap.  heap_up() will
	 * restore the heap by moving the new entry up until
	 * it lies in a valid position.
	 */

	calllimit = j;
	j = heap_up(t, j);

	/*
	 * j is the index of the new entry in the correct
	 * (legal heap) position.  Fill in the particulars
	 * of the request.
	 */

	p1		= &callout[j];
	p1->c_time	= t;
	p1->c_func	= fun;
	p1->c_arg	= arg;
	p1->c_id	= id = ++timeid;

	splx(s);

	/*
	 * Return the id, suitable for a call to untimeout().
	 */

	return id;
}

int
ttimeout(fun, arg, tim)
	void (*fun)();
	caddr_t  arg;
{
	int	s;
	int	id;

	s = spl7();

	if (calllimit + 1 == v.v_call) {
		splx(s);
		return -1;
	} else {
		id = timeout(fun, arg, (long)tim);
		splx(s);
		return id;
	}
}

/*
 * untimeout(id) is called to remove an entry in the callout
 * table that was originally placed there by a call to timeout().
 * id is the unique identifier returned by the timeout() call.
 */
int
untimeout(id)
	int id;
{
	register struct	callo	*p1;	/* pointer to entry with proper id */
	register struct	callo	*pend;	/* pointer to last valid table entry */
	register int	f;		/* index to entry with proper id */
	int	s;			/* temp variable for spl() */
	int	t;			/* temp variable for time for reheap */
	int	j;			/* index for last element in reheap */ 

	s = splhi();
	
	/*
	 * Linear search through table looking for an entry
	 * with an id that matches the id requested for removal.
	 */

	f = -1;
	pend = &callout[calllimit];
	for (p1 = &callout[0]; p1 <= pend; p1++) {
		++f;
		if (p1->c_id == id)
			goto found;
	}
	f = -1;
	goto badid;

	/*
	 * We have the entry at f; delete it, move the last entry
	 * of the table into this location, and reheap.
	 */

found:
	if (f == calllimit--)	/* last entry in table; no reheap necessary */
		goto done;

	if (calllimit >= 0) {
		t = pend->c_time;
		j = (f-1) >> 1;	/* j is the parent of f */

		if (f > 0 && callout[j].c_time > t)
			j = heap_up(t, f);
		else
			j = heap_down(t, f);

		callout[j] = *pend;
	}
badid:
done:
	splx(s);

	/*
	 * Failure returns -1.
	 */

	return f;
}

/*
 * timein() is called via a PIR9 which was set by timepoke()
 * in clock()
 */
int
timein()
{
	register struct	callo	*p0;	/* pointer to first entry in table */
	register struct	callo	*plast;	/* pointer to last entry in table */
	struct	callo	svcall;		/* pointer to current entry */
	int	t;			/* time of current entry */
	int	j;			/* index of current entry */
	int	s;			/* temp variable for spl() */

	s = splhi();

	p0 = &callout[0];
	while ((p0->c_time <= lbolt) && (calllimit >= 0)) {

		svcall = *p0;

		/*
		 * timein() deletes the first entry in the table.
		 * Move the last entry up, and reheap.
		 */

		plast = &callout[calllimit--];
		if (calllimit >= 0) {
			t = plast->c_time;
			j = heap_down(t,0);
			callout[j] = *plast;
		}

		(svcall.c_func)(svcall.c_arg);
	}
	splx(s);
	return 0;
}

/*
 * heap_up and heap_down are internal support functions that
 * maintain the heap structure of the callout table.
 * heap_up will take an illegal entry and percolate it up until
 * it lies in a legal position; heap_down will percolate an
 * illegal entry down until it falls in a legal position.
 */

STATIC int
heap_up(t, j)
	int t, j;
{
	register int	k;		/* index of parent of entry j */
	register struct	callo *p1;	/* pointer to parent of entry j */

	while (j-- > 0) {

		k = j >> 1;
		p1 = &callout[k];

		if (p1->c_time > t) {
			callout[++j] = *p1;
			j = k;
		} else
			break;
	}

	return 1+j;
}

STATIC int
heap_down(t, j)
	int t, j;
{
	register int	k;		/* index of child to exchange */
	register struct callo	*pk;	/* pointer to child to exchange */

	for (;;) {

	  	if ((k = 1 + (j << 1)) > calllimit)	/* left child ? */
			break;

		pk = &callout[k];

		if ((k < calllimit)			/* right child? */
		    && ((pk + 1)->c_time < pk->c_time)) {
			pk++;
			k++;
		}

		if (pk->c_time >= t)
			break;

		callout[j] = *pk;
		j = k;
	}
	return j;
}

#define	PDELAY	(PZERO-1)
#undef wakeup	/* we want to refer to the function, not the macro */

extern void	wakeup();

void
delay(ticks)
	long ticks;
{
	int s;

	if (ticks <= 0)
		return;
	s = splhi();
	(void)timeout((void(*)())wakeup, (caddr_t)u.u_procp+1, ticks);
	sleep((caddr_t)u.u_procp+1, PDELAY);
	splx(s);
}

/*
 * SunOS function to generate monotonically increasing time values.
 */
void
uniqtime(tv)
	register struct timeval *tv;
{
 	static struct timeval last;

	if (last.tv_sec != (long)hrestime.tv_sec) {
		last.tv_sec = (long)hrestime.tv_sec;
		last.tv_usec = (long)0;
	} else {
		last.tv_usec++;
	}
	*tv = last;
}
