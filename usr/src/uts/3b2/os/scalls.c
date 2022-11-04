/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/scalls.c	1.74.3.1"
#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/tuneable.h"
#include "sys/signal.h"
#include "sys/sbd.h"
#include "sys/immu.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/evecb.h"
#include "sys/hrtcntl.h"
#include "sys/debug.h"
#include "sys/errno.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/file.h"
#include "sys/proc.h"
#include "sys/var.h"
#include "sys/clock.h"
#include "sys/inline.h"
#include "sys/conf.h"
#include "sys/uio.h"
#include "sys/uadmin.h"
#include "sys/utsname.h"
#include "sys/utssys.h"
#include "sys/ustat.h"
#include "sys/statvfs.h"
#include "sys/fault.h"
#include "sys/syscall.h"
#include "sys/ucontext.h"
#include "sys/procfs.h"
#include "sys/procset.h"
#include "sys/siginfo.h"
#include "sys/session.h"
#include "sys/class.h"
#include "sys/disp.h"
#include "sys/time.h"
#include "sys/sysconfig.h"
#include "sys/resource.h"
#include "sys/wait.h"
#include "sys/systeminfo.h"
#include "sys/ulimit.h"
#include "sys/todc.h"
#include "sys/kmem.h"

#include "vm/hat.h"
#include "vm/as.h"
#include "vm/seg.h"

extern	time_t	time; /* Defined in os/clock.c */

struct gtimea {
	int a;
};

/* ARGSUSED */
int
gtime(uap, rvp)
	struct gtimea *uap;
	rval_t *rvp;
{
	rvp->r_time = hrestime.tv_sec;
	return 0;
}

struct stimea {
	time_t	time;
};

/* ARGSUSED */
int
stime(uap, rvp)
	register struct stimea *uap;
	rval_t *rvp;
{
	extern int rf_stime();

	if (suser(u.u_cred)) {
		time = hrestime.tv_sec = uap->time;
		wtodc();
		(void)rf_stime(u.u_cred);	/* RFS */
		return 0;
	} else {
		return EPERM;
	}
}
struct sysconfiga {
	int which;
};

int
sysconfig(uap, rvp)
	register struct sysconfiga *uap;
	rval_t *rvp;
{
	switch (uap->which) {

	default:
		return EINVAL;

	case _CONFIG_CHOWN_RST:
		/*
		 * 1 if chown(2) is restricted to super-user, 0 otherwise.
		 */
		rvp->r_val1 = rstchown;
		break;

	case _CONFIG_NGROUPS:
		/*
		 * Maximum number of supplementary groups.
		 */
		rvp->r_val1 = ngroups_max;
		break;

	case _CONFIG_OPEN_FILES:
		/*
		 * Maxiumum number of open files (soft limit).
		 */
		rvp->r_val1 = u.u_rlimit[RLIMIT_NOFILE].rlim_cur;
		break;

	case _CONFIG_CHILD_MAX:
		/*
		 * Maxiumum number of processes.
		 */
		rvp->r_val1 = v.v_maxup;
		break;

	case _CONFIG_POSIX_VER:
		rvp->r_val1 = _POSIX_VERSION;	/* defined in param.h */
		break;

	case _CONFIG_PAGESIZE:
		rvp->r_val1 = PAGESIZE;
		break;
	}
	
	return 0;
}

struct adjtimea {
	struct timeval *delta;
	struct timeval *olddelta;
};

int tickadj = MICROSEC/HZ;		/* "standard" clock skew,
				         * msec per tick */
int tickdelta;				/* current clock skew,
				         * msecs per tick */
long timedelta;				/* unapplied time correction, msecs */
long bigadj = MICROSEC ;		/* bigger skew */
int doresettodr;			/* reset clock flag */

/* ARGSUSED */
int
adjtime(uap, rvp)
	register struct adjtimea *uap;
	rval_t *rvp;
{
	register long ndelta;
	register int s;
	struct timeval atv, oatv;

	if (!suser(u.u_cred))
		return EPERM;

	if (copyin((caddr_t)uap->delta, (caddr_t)&atv, sizeof(struct timeval)))
		return EFAULT;

	ndelta = atv.tv_sec * MICROSEC + atv.tv_usec;

	if (timedelta == 0)
		if (ndelta > bigadj)
			tickdelta = 10 * tickadj;
		else
			tickdelta = tickadj;

	if (ndelta % tickdelta)
		ndelta = ndelta / tickadj * tickadj;

	s = splhi();
	if (uap->olddelta) {
		oatv.tv_sec = timedelta / MICROSEC;
		oatv.tv_usec = timedelta % MICROSEC;
	}
	timedelta = ndelta;
	splx(s);

	if (uap->olddelta) {
		if (copyout((caddr_t)&oatv, (caddr_t)uap->olddelta,
		  sizeof (struct timeval)))
			return EFAULT;
	}
	doresettodr = 1;
	return 0;
}

struct setuida {
	uid_t	uid;
};

/* ARGSUSED */
int
setuid(uap, rvp)
	register struct setuida *uap;
	rval_t *rvp;
{
	register uid_t uid;
	int error = 0;

	if ((uid = uap->uid) >= MAXUID)
		return EINVAL;
	if (u.u_cred->cr_uid
	  && (uid == u.u_cred->cr_ruid || uid == u.u_cred->cr_suid)) {
		u.u_cred = crcopy(u.u_cred);
		u.u_cred->cr_uid = uid;
		if (uid < USHRT_MAX)
			u.u_uid = (o_uid_t) uid;
		else
			u.u_uid = (o_uid_t) UID_NOBODY;
	} else if (suser(u.u_cred)) {
		u.u_cred = crcopy(u.u_cred);
		u.u_cred->cr_uid = uid;
		u.u_cred->cr_ruid = uid;
		u.u_cred->cr_suid = uid;
		if ( uid < USHRT_MAX)
			u.u_uid = (o_uid_t) uid;
		else
			u.u_uid = (o_uid_t) UID_NOBODY;

		u.u_procp->p_uid = (o_uid_t) uid;		/* XXX */
	} else
		error = EPERM;

	return error;
}

/* ARGSUSED */
int
getuid(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	rvp->r_val1 = (int) u.u_cred->cr_ruid;
	rvp->r_val2 = (int) u.u_cred->cr_uid;
	return 0;
}

struct setgida {
	gid_t	gid;
};

/* ARGSUSED */
int
setgid(uap, rvp)
	register struct setgida *uap;
	rval_t *rvp;
{
	register gid_t gid;
	int error = 0;

	if ((gid = uap->gid) >= MAXUID)
		return EINVAL;
	if (u.u_cred->cr_uid
	  && (gid == u.u_cred->cr_rgid || gid == u.u_cred->cr_sgid)) {
		u.u_cred = crcopy(u.u_cred);
		u.u_cred->cr_gid = gid;
		if ( gid < USHRT_MAX)
			u.u_gid = (o_gid_t) gid;			/* XXX */
		else
			u.u_gid = (o_gid_t) UID_NOBODY;
	} else if (suser(u.u_cred)) {
		u.u_cred = crcopy(u.u_cred);
		u.u_cred->cr_gid = gid;
		u.u_cred->cr_rgid = gid;
		u.u_cred->cr_sgid = gid;
		if ( gid < USHRT_MAX)
			u.u_gid = (o_gid_t) gid;			/* XXX */
		else
			u.u_gid = (o_gid_t) UID_NOBODY;
	} else
		error = EPERM;

	return error;
}

/* ARGSUSED */
int
getgid(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	rvp->r_val1 = u.u_cred->cr_rgid;
	rvp->r_val2 = u.u_cred->cr_gid;
	return 0;
}

/* ARGSUSED */
int
getpid(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	rvp->r_val1 = u.u_procp->p_pid;
	rvp->r_val2 = u.u_procp->p_ppid;
	return 0;
}

struct seteuida {
	int	uid;
};

/* ARGSUSED */
int
seteuid(uap, rvp)
	register struct seteuida *uap;
	rval_t *rvp;
{
	register unsigned uid;
	int error = 0;

	if ((uid = uap->uid) >= MAXUID)
		error = EINVAL;
	else if (uid == u.u_cred->cr_ruid 
	  || uid == u.u_cred->cr_uid
	  || uid == u.u_cred->cr_suid
	  || suser(u.u_cred)) {
		u.u_cred = crcopy(u.u_cred);
		u.u_cred->cr_uid = uid;
		if (uid < USHRT_MAX)
			u.u_uid = (o_uid_t) uid;
		else
			u.u_uid = (o_uid_t) UID_NOBODY;
	} else
		error = EPERM;

	return error;
}

struct setegida {
	int	gid;
};

/* ARGSUSED */
int
setegid(uap, rvp)
	register struct setegida *uap;
	rval_t *rvp;
{
	register unsigned gid;
	int error = 0;

	if ((gid = uap->gid) >= MAXUID)
		error = EINVAL;
	else if (gid == u.u_cred->cr_rgid 
	  || gid == u.u_cred->cr_gid
	  || gid == u.u_cred->cr_sgid
	  || suser(u.u_cred)) {
		u.u_cred = crcopy(u.u_cred);
		u.u_cred->cr_gid = gid;

		if ( gid < USHRT_MAX)
			u.u_gid = (o_gid_t) gid;		/* XXX */
		else
			u.u_gid = (o_gid_t) UID_NOBODY;

	} else
		error = EPERM;

	return error;
}

struct setgroupsa {
	u_int	gidsetsize;
	gid_t	*gidset;
};

/* ARGSUSED */
int
setgroups(uap, rvp)
	register struct setgroupsa *uap;
	rval_t *rvp;
{
	register cred_t *cr = u.u_cred, *newcr;
	register u_short i, n = uap->gidsetsize;

	if (!suser(cr))
		return EPERM;

	if (n > (u_short)ngroups_max)
		return EINVAL;

	newcr = crdup(u.u_cred);

	if (n != 0
	  && copyin((caddr_t)uap->gidset, (caddr_t)newcr->cr_groups,
	    n * sizeof(gid_t))) {
		crfree(newcr);
		return EFAULT;
	}

	for (i = 0; i < n; i++) {
		if (newcr->cr_groups[i] < 0 || newcr->cr_groups[i] >= MAXUID) {
			crfree(newcr);
			return EINVAL;
		}
	}
	newcr->cr_ngroups = n;

	u.u_cred = newcr;
	crfree(cr);

	return 0;
}

struct getgroupsa {
	u_int	gidsetsize;
	gid_t	*gidset;
};

int
getgroups(uap, rvp)
	register struct getgroupsa *uap;
	rval_t *rvp;
{
	register struct cred *cr = u.u_cred;
	register u_short n = cr->cr_ngroups;

	if (uap->gidsetsize != 0) {
		if (uap->gidsetsize < n)
			return EINVAL;
		if (copyout((caddr_t)cr->cr_groups, (caddr_t)uap->gidset, 
		  n * sizeof(gid_t)))
			return EFAULT;
	}

	rvp->r_val1 = n;
	return 0;
}
	
struct setpgrpa {
	int	flag;
	int	pid;
	int	pgid;
};

/* ARGSUSED */
int
setpgrp(uap, rvp)
	register struct setpgrpa *uap;
	rval_t *rvp;
{
	register proc_t *p = u.u_procp;

	switch (uap->flag) {

	case 1: /* setpgrp() */
		if (p->p_sessp->s_sid != p->p_pid && !memberspg(p->p_pid))
			newsession();
		rvp->r_val1 = p->p_sessp->s_sid;
		return 0;

	case 3: /* setsid() */
		if (p->p_pgrp == p->p_pid || memberspg(p->p_pid))
			return EPERM;
		newsession();
		rvp->r_val1 = p->p_sessp->s_sid;
		return 0;

	case 5: /* setpgid() */
	{
		register pid_t pgid = uap->pgid;
		register pid_t pid = uap->pid;
		if (pid < 0 || pgid < 0 
		  || pid >= MAXPID || pgid >= MAXPID)
			return EINVAL;
		if (pid != p->p_pid && pid != 0) {
			for (p = p->p_child; ; p = p->p_sibling) {
				if (p == NULL)
					return ESRCH;
				if (p->p_pid == pid)
					break;
			}
			if (p->p_flag & SEXECED)
				return EACCES;
		}
		if (pgid == 0)
			pgid = p->p_pid;
		if (p->p_pgrp == pgid)
			break;
		if (p->p_sessp->s_sid == p->p_pid
		  || (p->p_pgrp == p->p_pid && memberspg(p->p_pid))
		  || p->p_sessp != u.u_procp->p_sessp
		  || pgid != p->p_pid && !checkpg(p->p_sessp->s_sid, pgid))
			return EPERM;
		leavepg(p);
		joinpg(p, pgid);
		attachpg(p);
		break;
	}

	case 0: /* getpgrp() */
		rvp->r_val1 = p->p_pgrp;
		break;

	case 2: /* getsid() */
	case 4: /* getpgid() */
		if (uap->pid < 0 || uap->pid >= MAXPID)
			return EINVAL;
		else if (uap->pid != 0 
		  && p->p_pid != uap->pid
		  && (p = prfind(uap->pid)) == NULL)
			return ESRCH;
		if (uap->flag == 2)
			rvp->r_val1 = p->p_sessp->s_sid;
		else
			rvp->r_val1 = p->p_pgrp;
		break;
		
	}
	return 0;
}

/*
 * Indefinite wait.  No one should wakeup(&u).
 */

void
pause()
{
	for (;;)
		sleep((caddr_t)&u, PSLEP);
	/* NOTREACHED */
}

/*
 * ssig() is the common entry for signal, sigset, sighold, sigrelse,
 * sigignore and sigpause.
 *
 * for implementations that don't require binary compatibility,
 * signal, sigset, sighold, sigrelse, sigignore and sigpause may
 * be made into library routines that call sigaction, sigsuspend and sigprocmask
 */

struct siga {
	int	signo;
	void	(*fun)();
};

int
ssig(uap, rvp)
	struct siga *uap;
	rval_t *rvp;
{
	register sig;
	register struct proc *pp;
	register void (*func)();
	register flags;

	sig = uap->signo & SIGNO_MASK;

	if (sig <= 0 || sig >= NSIG || sigismember(&cantmask, sig))
		return EINVAL;

	pp = u.u_procp;
	func = uap->fun;

	switch (uap->signo & ~SIGNO_MASK) {

	case SIGHOLD:	/* sighold */
		sigaddset(&pp->p_hold, sig);
		return 0;

	case SIGRELSE:	/* sigrelse */
		sigdelset(&pp->p_hold, sig);
		return 0;

	case SIGPAUSE:	/* sigpause */
		sigdelset(&pp->p_hold, sig);
		pause();
		/* NOTREACHED */

	case SIGIGNORE:	/* signore */
		sigdelset(&pp->p_hold, sig);
		func = SIG_IGN;
		flags = 0;
		break;

	case SIGDEFER:		/* sigset */
		if (sigismember(&pp->p_hold, sig))
			rvp->r_val1 = (int) SIG_HOLD;
		else
			rvp->r_val1 = (int) u.u_signal[sig-1];
		if (func == SIG_HOLD) {
			sigaddset(&pp->p_hold, sig);
			return 0;
		}
		sigdelset(&pp->p_hold, sig);
		flags = 0;
		break;

	case 0:	/* signal */
		rvp->r_val1 = (int) u.u_signal[sig-1];
		flags = SA_RESETHAND|SA_NODEFER;
		break;

	default:		/* error */
		return EINVAL;
	}

	if (sigismember(&stopdefault, sig))
		flags |= SA_RESTART;
	else if (sig == SIGCLD) {
		flags |= SA_NOCLDSTOP;
		if (func == SIG_IGN)
			flags |= SA_NOCLDWAIT;
		else if (func != SIG_DFL) {
			register proc_t *cp;
			for (cp = pp->p_child; cp; cp = cp->p_sibling) {
				if (cp->p_stat == SZOMB) {
					sigaddset(&pp->p_sig, SIGCLD);
					break;
				}
			}
		}
	}

	setsigact(sig, func, (k_sigset_t)0, flags);
	
	return 0;
}

struct sigsuspenda {
	sigset_t *set;
};

/* ARGSUSED */
int
sigsuspend(uap, rvp)
	register struct sigsuspenda *uap;
	rval_t *rvp;
{
	sigset_t set;
	k_sigset_t kset;

	if (copyin((caddr_t)uap->set, (caddr_t)&set, sizeof(sigset_t)))
		return EFAULT;
	sigutok(&set, &kset);
	sigdiffset(&kset, &cantmask);
	u.u_sigoldmask = u.u_procp->p_hold;
	u.u_procp->p_hold = kset;
	u.u_sigflag |= SOMASK;
	pause();
	/* NOTREACHED */
}

struct sigaltstacka {
	struct sigaltstack *ss;
	struct sigaltstack *oss;
};

/* ARGSUSED */
int
sigaltstack(uap, rvp)
	struct sigaltstacka *uap;
	rval_t *rvp;
{
	struct sigaltstack ss;

	/*
	 * User's oss and ss might be the same address, so copyin first and
	 * save before copying out.
	 */
	if (uap->ss) {
		if (u.u_altflags & SS_ONSTACK)
			return EPERM;
		if (copyin((caddr_t) uap->ss, (caddr_t) &ss, sizeof(ss)))
			return EFAULT;
		if (ss.ss_flags & ~(SS_ONSTACK|SS_DISABLE))
			return EINVAL;
		if (!(ss.ss_flags & SS_DISABLE) && ss.ss_size < MINSIGSTKSZ)
			return ENOMEM;
	}

	if (uap->oss) {
		if (copyout((caddr_t) &u.u_sigaltstack, (caddr_t) uap->oss,
		  sizeof(struct sigaltstack)))
			return EFAULT;
	}

	if (uap->ss)
		u.u_sigaltstack = ss;

	return 0;
}

struct sigpendinga {
	int flag;
	sigset_t *set;
};

/* ARGSUSED */
int
sigpending(uap, rvp)
	register struct sigpendinga *uap;
	rval_t *rvp;
{
	sigset_t set;
	k_sigset_t kset;

	switch (uap->flag) {
	case 1: /* sigpending */
		kset = u.u_procp->p_sig;
		sigandset(&kset, &u.u_procp->p_hold);
		break;
	case 2: /* sigfillset */
		kset = fillset;
		break;
	default:
		return EINVAL;
	}

	sigktou(&kset, &set);
	if (copyout((caddr_t) &set, (caddr_t) uap->set, sizeof(sigset_t)))
		return EFAULT;
	return 0;
}

struct sigprocmaska {
	int how;
	sigset_t *set;
	sigset_t *oset;
};

/* ARGSUSED */
int
sigprocmask(uap, rvp)
	register struct sigprocmaska *uap;
	rval_t *rvp;
{
	k_sigset_t kset;

	/*
	 * User's oset and set might be the same address, so copyin first and
	 * save before copying out.
	 */
	if (uap->set) {
		sigset_t set;
		if (copyin((caddr_t)uap->set, (caddr_t)&set, sizeof(sigset_t)))
			return EFAULT;
		sigutok(&set, &kset);
	}

	if (uap->oset) {
		sigset_t set;
		sigktou(&u.u_procp->p_hold, &set);
		if (copyout((caddr_t)&set, (caddr_t)uap->oset,
		  sizeof(sigset_t)))
			return EFAULT;
	}
	
	if (uap->set) {
		sigdiffset(&kset, &cantmask);
		switch (uap->how) {
		case SIG_BLOCK:
			sigorset(&u.u_procp->p_hold, &kset);
			break;
		case SIG_UNBLOCK:
			sigdiffset(&u.u_procp->p_hold, &kset);
			break;
		case SIG_SETMASK:
			u.u_procp->p_hold = kset;
			break;
		default:
			return EINVAL;
		}
	}
	return 0;
}

struct sigactiona {
	int sig;
	struct sigaction *act;
	struct sigaction *oact;
};

/* ARGSUSED */
int
sigaction(uap, rvp)
	register struct sigactiona *uap;
	rval_t *rvp;
{
	struct sigaction act;
	k_sigset_t set;
	register int sig;

	sig = uap->sig;

	if (sig <= 0 || sig >= NSIG || sigismember(&cantmask, sig))
		return EINVAL;

	/* act and oact might be the same address, so copyin act first */
	if (uap->act) {
		if (copyin((caddr_t)uap->act, (caddr_t)&act, sizeof(act)))
			return EFAULT;
	}

	if (uap->oact) {

		struct sigaction oact;
		register flags;
		register void (*disp)();

		disp = u.u_signal[sig - 1];	

		flags = 0;
		if (disp != SIG_DFL && disp != SIG_IGN) {
			set = u.u_sigmask[sig-1];
			if (sigismember(&u.u_procp->p_siginfo, sig))
				flags |= SA_SIGINFO;
			if (sigismember(&u.u_sigrestart, sig))
				flags |= SA_RESTART;
			if (sigismember(&u.u_sigonstack, sig))
				flags |= SA_ONSTACK;
			if (sigismember(&u.u_sigresethand, sig))
				flags |= SA_RESETHAND;
			if (sigismember(&u.u_signodefer, sig))
				flags |= SA_NODEFER;
		} else
			sigemptyset(&set);

		if (sig == SIGCLD) {
			if (u.u_procp->p_flag & SNOWAIT)
				flags |= SA_NOCLDWAIT;
			if (!(u.u_procp->p_flag & SJCTL))
				flags |= SA_NOCLDSTOP;
		}

		oact.sa_handler = disp;
		oact.sa_flags = flags;
		sigktou(&set, &oact.sa_mask);

		if (copyout((caddr_t)&oact, (caddr_t) uap->oact, sizeof(oact)))
			return EFAULT;
	}

	if (uap->act) {
		sigutok(&act.sa_mask, &set);
		setsigact(sig, act.sa_handler, set, act.sa_flags);
	}

	return 0;
}

struct killa {
	pid_t	pid;
	int	sig;
};

/*
 * for implementations that don't require binary compatibility,
 * the kill system call may be made into a library call to the
 * sigsend system call
 */

/* ARGSUSED */
int
kill(uap, rvp)
	register struct killa *uap;
	rval_t *rvp;
{
	register id_t id;
	register idtype_t idtype;

	if (uap->sig < 0 || uap->sig >= NSIG)
		return EINVAL;

	if (uap->pid > 0) {
		idtype = P_PID;
		id = (id_t)uap->pid;
	} else if (uap->pid == 0) {
		idtype = P_PGID;
		id = (id_t)u.u_procp->p_pgrp;
	} else if (uap->pid == -1) {
		idtype = P_ALL;
		id = P_MYID;
	} else { /* uap->pid < -1 */
		idtype = P_PGID;
		id = (id_t)(-uap->pid);
	}

	return sigsend(idtype, id, uap->sig);
}

/*
 * Device driver interface to sigsend.
 */
int
sigsend(idtype, id, sig)
	register idtype_t idtype;
	register id_t id;
	register sig;
{
	procset_t set;

	setprocset(&set, POP_AND, idtype, id, P_ALL, P_MYID);
	return sigsendset(&set, sig);
}

struct sigsenda {
	procset_t *psp;
	int	   sig;
};

/* ARGSUSED */
int
sigsendsys(uap, rvp)
	register struct sigsenda *uap;
	rval_t *rvp;
{
	procset_t set;

	if (uap->sig < 0 || uap->sig >= NSIG)
		return EINVAL;

	if (copyin((caddr_t)uap->psp, (caddr_t)&set, sizeof(procset_t)))
		return EFAULT;
	return sigsendset(&set, uap->sig);
}

/*
 * Return system and user times.
 * Times() assumes that p_utime is adjacent to p_stime and 
 * p_cutime is adjacent to p_cstime.
 */

struct timesa {
	time_t	(*times)[4];
};

int
times(uap, rvp)
	register struct timesa *uap;
	rval_t *rvp;
{
	register time_t	*times;

	times = (time_t *)uap->times; 
	if (copyout((caddr_t)&u.u_procp->p_utime, (caddr_t) times,
	    sizeof(time_t) * 2))
		return EFAULT;

	if (copyout((caddr_t)&u.u_procp->p_cutime,
	    (caddr_t) &times[2], sizeof(time_t) * 2))
		return EFAULT;
	spl7();
	rvp->r_time = lbolt;
	spl0();
	return 0;
}

/*
 * Profiling.
 */

struct profila {
	short	*bufbase;
	unsigned bufsize;
	unsigned pcoffset;
	unsigned pcscale;
};

/* ARGSUSED */
int
profil(uap, rvp)
	register struct profila *uap;
	rval_t *rvp;
{
	u.u_prof.pr_base = uap->bufbase;
	u.u_prof.pr_size = uap->bufsize;
	u.u_prof.pr_off = uap->pcoffset;
	u.u_prof.pr_scale = uap->pcscale;
	return 0;
}

/*
 * Alarm clock signal.
 */

struct alarma {
	int	deltat;
};

int
alarm(uap, rvp)
	register struct alarma *uap;
	rval_t *rvp;
{
	register struct proc *p;
	register c;

	p = u.u_procp;
	c = p->p_clktim;
	p->p_clktim = uap->deltat;
	rvp->r_val1 = c;
	return 0;
}

/*
 * Mode mask for creation of files.
 */

struct umaska {
	int	mask;
};

int
umask(uap, rvp)
	register struct umaska *uap;
	rval_t *rvp;
{
	register mode_t t;

	t = u.u_cmask;
	u.u_cmask =  (mode_t) (uap->mask & PERMMASK);
	rvp->r_val1 = (int) t;
	return 0;
}

/*
 * ulimit could be moved into a user library, as calls to getrlimit and
 * setrlimit, were it not for binary compatibility restrictions
 */

struct ulimita {
	int	cmd;
	long	arg;
};

int
ulimit(uap, rvp)
	register struct ulimita *uap;
	rval_t *rvp;
{
	register rlim_t lim;
	register int error = 0;

	switch (uap->cmd) {

	case UL_SFILLIM: /* Set new file size limit. */
		lim = uap->arg << SCTRSHFT;
		if (error = rlimit(RLIMIT_FSIZE, lim, lim))
			return error;
		/* FALLTHROUGH */

	case UL_GFILLIM: /* Return current file size limit. */
		rvp->r_off = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
		break;


	case UL_GMEMLIM: /* Return maximum possible break value. */
	{
		register struct seg *seg, *sseg;
		register struct seg *nextseg;
		register struct proc *p = u.u_procp;
		register struct as *as = p->p_as;
		register caddr_t brkend = (caddr_t) p->p_brkbase + p->p_brksize;
		uint size;

		/*
		 * Find the segment with a virtual address
		 * greater than the end of the current break.
		 */
		nextseg = NULL;
		sseg = seg = as->a_segs;
		if (seg != NULL) {
			do {
                                if (seg->s_base >= brkend) {
                                        nextseg = seg;
                                        break;
                                }
                                seg = seg->s_next;
			} while (seg != sseg);
		}

		/*
		 * We reduce the max break value (base+rlimit[DATA]
		 * if we run into another segment, the ublock or
		 * the end of memory.  We also are limited by
		 * rlimit[VMEM].
		 */
		rvp->r_off = (off_t)
			(p->p_brkbase + u.u_rlimit[RLIMIT_DATA].rlim_cur);
		if (nextseg != NULL)
			rvp->r_off = min(rvp->r_off, (off_t)nextseg->s_base);
		if (brkend <= (caddr_t)UVUBLK)
			rvp->r_off = min(rvp->r_off, (off_t)UVUBLK);
		else
			rvp->r_off = min(rvp->r_off, (off_t)UVEND);

		/*
		 * Also handle case where rlimit[VMEM] has been 
		 * lowered below the current address space size.
		 */

		size = as->a_size;
		if (size < u.u_rlimit[RLIMIT_VMEM].rlim_cur)
			size = u.u_rlimit[RLIMIT_VMEM].rlim_cur - size;
		else
			size = 0;
		rvp->r_off = min(rvp->r_off, (off_t)(brkend + size));

		return 0;
	}

	case UL_GDESLIM: /* Return approximate number of open files */
		rvp->r_off = u.u_rlimit[RLIMIT_NOFILE].rlim_cur;
		break;

	case UL_GTXTOFF: /* 64 - for XENIX compatibility */
		/* Return number of bytes between the beginning of
		 * user text and the text address given by 'arg'.
		 * Only valid for 386 binaries.  286 XENIX binaries
		 * will have this ulimit() call handled by the emulator.
		 *
		 * Just return the text offset the user sent as the argument,
		 * since we're small model...
		 */
		rvp->r_off = uap->arg;
		break;

	default:
		error = EINVAL;
		break;

	}

	return error;

}

struct rlimita {
	int	resource;
	struct rlimit *rlp;
};

/* ARGSUSED */
int
getrlimit(uap, rvp)
	register struct rlimita *uap;
	rval_t *rvp;
{
	if (uap->resource < 0 || uap->resource >= RLIM_NLIMITS)
		return EINVAL;
	if (copyout((caddr_t)&u.u_rlimit[uap->resource], 
	  (caddr_t)uap->rlp, sizeof(struct rlimit)))
		return EFAULT;
	return 0;
}

/* ARGSUSED */
int
setrlimit(uap, rvp)
	register struct rlimita *uap;
	rval_t *rvp;
{
	struct rlimit rlim;

	if (uap->resource < 0 || uap->resource >= RLIM_NLIMITS)
		return EINVAL;
	if (copyin((caddr_t)uap->rlp, (caddr_t)&rlim, sizeof(rlim)))
		return EFAULT;
	return rlimit(uap->resource,rlim.rlim_cur,rlim.rlim_max);
}

/*
 * utssys()
 */
 
struct utssysa {
	union {
		char *cbuf;
		struct ustat *ubuf;
	} ub;
	union {
		int	mv;		/* for USTAT */
		int	flags;		/* for FUSERS */
	} un;
	int	type;
	char	*outbp;			/* for FUSERS */
};

int
utssys(uap, rvp)
	register struct utssysa *uap;
	rval_t *rvp;
{
	register int error = 0;

	switch (uap->type) {

	case UTS_UNAME:
	{
		char *buf = uap->ub.cbuf;

		if (copyout(utsname.sysname, buf, 8)) {
			error = EFAULT;
			break;
		}
		buf += 8;
		if (subyte(buf, 0) < 0) {
			error = EFAULT;
			break;
		}
		buf++;
		if (copyout(utsname.nodename, buf, 8)) {
			error = EFAULT;
			break;
		}
		buf += 8;
		if (subyte(buf, 0) < 0) {
			error = EFAULT;
			break;
		}
		buf++;
		if (copyout(utsname.release, buf, 8)) {
			error = EFAULT;
			break;
		}
		buf += 8;
		if (subyte(buf, 0) < 0) {
			error = EFAULT;
			break;
		}
		buf++;
		if (copyout(utsname.version, buf, 8)) {
			error = EFAULT;
			break;
		}
		buf += 8;
		if (subyte(buf, 0) < 0) {
			error = EFAULT;
			break;
		}
		buf++;
		if (copyout(utsname.machine, buf, 8)) {
			error = EFAULT;
			break;
		}
		buf += 8;
		if (subyte(buf, 0) < 0) {
			error = EFAULT;
			break;
		}
		rvp->r_val1 = 1;
		break;
	}

	case UTS_USTAT:
	{
		register struct vfs *vfsp;
		struct ustat ust;
		struct statvfs stvfs;
		extern int rf_ustat();

		/*
		 * RFS HOOK.
		 */
		if (uap->un.mv < 0)
			return rf_ustat((dev_t)uap->un.mv, uap->ub.ubuf);
		/*
		 * Search vfs list for user-specified device.
		 */
		for (vfsp = rootvfs; vfsp != NULL; vfsp = vfsp->vfs_next)
			if (vfsp->vfs_dev == uap->un.mv || 
				cmpdev(vfsp->vfs_dev) == uap->un.mv)

				break;
		if (vfsp == NULL) {
			error = EINVAL;
			break;
		}
		if (error = VFS_STATVFS(vfsp, &stvfs))
			break;
		if (stvfs.f_ffree > USHRT_MAX) {
			error = EOVERFLOW;
			break;
		}

		ust.f_tfree = (daddr_t) (stvfs.f_bfree * (stvfs.f_frsize/512));
		ust.f_tinode = (o_ino_t) stvfs.f_ffree;
		bcopy(&stvfs.f_fstr[0], ust.f_fpack, sizeof(ust.f_fpack));
		bcopy(&stvfs.f_fstr[sizeof(ust.f_fpack)], ust.f_fname,
		  sizeof(ust.f_fname));
		if (copyout((caddr_t)&ust, uap->ub.cbuf, sizeof(ust)))
			error = EFAULT;
		break;
	}

	case UTS_FUSERS:
	{
		STATIC int uts_fusers();
		return uts_fusers(uap->ub.cbuf, uap->un.flags, uap->outbp, rvp);
	}

	default:
		error = EINVAL;		/* ? */
		break;
	}

	return error;
}

/*
 * Determine the ways in which processes are using a named file or mounted
 * file system (path).  Normally return 0 with rvp->rval1 set to the number of 
 * processes found to be using it.  For each of these, fill a f_user_t to
 * describe the process and its useage.  When successful, copy this list
 * of structures to the user supplied buffer (outbp).
 *
 * In error cases, clean up and return the appropriate errno.
 */

STATIC int
uts_fusers(path, flags, outbp, rvp)
	char *path;
	int flags;
	char *outbp;
	rval_t *rvp;
{
        vnode_t *fvp = NULL;
	int error;
	extern int lookupname();

        int dofusers();

        if ((error = lookupname(path, UIO_USERSPACE, FOLLOW, NULLVPP, &fvp))
	  != 0) {
		return error;
	}
	ASSERT(fvp);
	error = dofusers(fvp, flags, outbp, rvp);
	VN_RELE(fvp);
	return error;
}

int
dofusers(fvp, flags, outbp, rvp)
	vnode_t *fvp;
	int flags;
	char *outbp;
	rval_t *rvp;
{
	register proc_t **prpp;
	register int pcnt = 0;		/* number of f_user_t's copied out */
	int error = 0;
	register int contained = (flags == F_CONTAINED);
	register vfs_t *cvfsp;
	register int use_flag = 0;
	register int fd;
	register struct ufchunk *ufp;
	file_t *fp;
	f_user_t *fuentry, *fubuf;	/* accumulate results here */

	if ((fuentry = fubuf = 
	  (f_user_t *)kmem_alloc(v.v_proc * sizeof(f_user_t), 0)) == NULL) {
		return ENOMEM;
	}
	if (contained && !(fvp->v_flag & VROOT)) {
		error = EINVAL;
		goto out;
	}
	if (fvp->v_count == 1) {	/* no other active references */
		goto out;
	}
	cvfsp = fvp->v_vfsp;
	ASSERT(cvfsp);
	for (prpp = nproc; prpp < v.ve_proc; prpp++) {
		register user_t *up;
		register proc_t *procp;

		if ((procp = *prpp) == NULL || procp->p_stat == 0 ||
		  procp->p_stat == SZOMB || procp->p_stat == SIDL) {
			continue;
		}
		up = (user_t *)KUSER(procp->p_segu);
		if (up->u_cdir && (VN_CMP(fvp, up->u_cdir) || contained && 
		  up->u_cdir->v_vfsp == cvfsp)) {
			use_flag |= F_CDIR;
		}
		if (up->u_rdir && (VN_CMP(fvp, up->u_rdir) || contained && 
		  up->u_rdir->v_vfsp == cvfsp)) {
			use_flag |= F_RDIR;
		}
		if (up->u_exdata.vp && (VN_CMP(fvp, up->u_exdata.vp) ||
		  contained && up->u_exdata.vp->v_vfsp == cvfsp)) {
			use_flag |= F_TEXT;
		}
		if (procp->p_trace && (VN_CMP(fvp, procp->p_trace) ||
		  contained && procp->p_trace->v_vfsp == cvfsp)) {
			use_flag |= F_TRACE;
		}
		if (procp->p_sessp && (VN_CMP(fvp,procp->p_sessp->s_vp) ||
		  contained && procp->p_sessp->s_vp && 
		  procp->p_sessp->s_vp->v_vfsp == cvfsp)) {
			use_flag |= F_TTY;
		}
		ufp = &(up->u_flist);
		for (fd = 0; fd < up->u_nofiles; fd++) {
			fp = ufp->uf_ofile[fd % NFPCHUNK];
			if (fp != NULLFP && fp->f_vnode && 
			  (VN_CMP(fvp, fp->f_vnode) ||
			  contained && fp->f_vnode->v_vfsp == cvfsp)) {
				use_flag |= F_OPEN;
				break;		/* we don't count fds */
			}
			if ((fd + 1) % NFPCHUNK == 0) {
				ufp = ufp->uf_next;
			}
		}
		/*
		 * mmap usage??
		 */
		if (use_flag) {
			fuentry->fu_pid = procp->p_pid;
			fuentry->fu_flags = use_flag;
			fuentry->fu_uid = (uid_t) procp->p_uid;
			fuentry++;
			pcnt++;
			use_flag = 0;
		}
	}
	if (copyout((caddr_t)fubuf, outbp, pcnt * sizeof(f_user_t))) {
		error = EFAULT;
	}
out:
	kmem_free(fubuf, v.v_proc * sizeof(f_user_t));
	rvp->r_val1 = pcnt;
	return error;
}


struct uname {
	struct utsname *cbuf;
};

/* ARGSUSED */
int
nuname(uap, rvp)
	register struct uname *uap;
	rval_t *rvp;
{
	register int error = 0;
        register struct utsname *buf = uap->cbuf;

        if (copyout(utsname.sysname, buf->sysname, strlen(utsname.sysname)+1))
{
                error = EFAULT;
                return error;
        }
        if (copyout(utsname.nodename, buf->nodename, strlen(utsname.nodename)+1)) {
                error = EFAULT;
                return error;
        }
        if (copyout(utsname.release, buf->release, strlen(utsname.release)+1))
{
                error = EFAULT;
                return error;
        }
        if (copyout(utsname.version, buf->version, strlen(utsname.version)+1))
{
                error = EFAULT;
                return error;
        }
        if (copyout(utsname.machine, buf->machine, strlen(utsname.machine)+1))
{
                error = EFAULT;
                return error;
        }
	rvp->r_val1 = 1;
	return error;
}

/*
 * Administrivia system call.
 */

struct uadmina {
	int	cmd;
	int	fcn;
	int	mdep;
};

/* ARGSUSED */
int
uadmin(uap, rvp)
	register struct uadmina *uap;
	rval_t *rvp;
{
	static ualock;
	register struct proc **p;
	int error = 0;

	if (ualock)
		return 0;
	if ((uap->cmd != A_SWAPCTL) && !suser(u.u_cred))
		return EPERM;
	ualock = 1;
	switch (uap->cmd) {

	case A_SHUTDOWN:
	{
		p = &nproc[1];
		for (; p < v.ve_proc; p++) {
			if (*p == NULL || (*p)->p_stat == 0)
				continue;
			if ((*p) != u.u_procp)
				psignal(*p, SIGKILL);
		}
		delay(HZ);	/* allow other procs to exit */
		(void) VFS_MOUNTROOT(rootvfs, ROOT_UNMOUNT);
		/* FALLTHROUGH */
	}

	case A_REBOOT:
	{
		extern void mdboot();
		mdboot(uap->fcn, uap->mdep);
		/* no return expected */
		break;
	}

	case A_REMOUNT:
		/* remount root file system */
		(void) VFS_MOUNTROOT(rootvfs, ROOT_REMOUNT);
		break;

	case A_SWAPCTL:
	{
		extern int swapctl();
		error = swapctl(uap, rvp);
		break;
	}

	default:
		error = EINVAL;
	}
	ualock = 0;
	return error;
}

struct setcontexta {
	int flag;
	caddr_t *ucp;
};

/* ARGSUSED */
setcontext(uap, rvp)
	register struct setcontexta *uap;
	rval_t *rvp;
{
	ucontext_t uc;

	/*
	 * In future releases, when the ucontext structure grows,
	 * getcontext should be modified to only return the fields
	 * specified in the uc_flags.
	 * That way, the structure can grow and still be binary
	 * compatible will all .o's which will only have old fields
	 * defined in uc_flags
	 */

	switch (uap->flag) {

	default:
		return EINVAL;

	case GETCONTEXT:
		savecontext(&uc, u.u_procp->p_hold);
		if (copyout((caddr_t)&uc,(caddr_t)uap->ucp,sizeof(ucontext_t)))
			return EFAULT;
		return 0;

	case SETCONTEXT:
		if (copyin((caddr_t)uap->ucp,(caddr_t)&uc,sizeof(ucontext_t)))
			return EFAULT;
		restorecontext(&uc);
	/* 
	 * On return from system calls, r0 and r1 are overwritten with 
	 * r_val1 and r_val2 respectively, so set r_val1 and r_val2 to 
	 * r0 and r1 here.
	 */
		rvp->r_val1 = u.u_pcb.regsave[K_R0];
		rvp->r_val2 = u.u_pcb.regsave[K_R1];
		return 0;
	}
}

struct systeminfoa {
	int command;
	char *buf;
	long count;
};


/* ARGSUSED */
int
systeminfo(uap, rvp)
	register struct systeminfoa *uap;
	rval_t *rvp;
{
	register int error = 0;
	register int strcnt, getcnt;

	switch (uap->command) {

	case SI_SYSNAME:
		getcnt = ((strcnt = strlen(utsname.sysname)) >= uap->count) ?
		  uap->count : strcnt + 1;
		if (copyout(utsname.sysname, uap->buf, getcnt)) {
			error = EFAULT;
			break;
		}
		if ((strcnt >= uap->count) 
		  && subyte(uap->buf+uap->count-1, 0) < 0) {
			error = EFAULT;
		}
		rvp->r_val1 = strcnt + 1;
		break;

	case SI_HOSTNAME:
		getcnt = ((strcnt = strlen(utsname.nodename)) >= uap->count) ?
		  uap->count : strcnt + 1;
		if (copyout(utsname.nodename, uap->buf, getcnt)) {
			error = EFAULT;
			break;
		}
		if ((strcnt >= uap->count) 
		  && subyte(uap->buf+uap->count-1, 0) < 0) {
			error = EFAULT;
		}
		rvp->r_val1 = strcnt + 1;
		break;

	case SI_RELEASE:
		getcnt = ((strcnt = strlen(utsname.release)) >= uap->count) ?
		  uap->count : strcnt + 1;
		if (copyout(utsname.release, uap->buf, getcnt)) {
			error = EFAULT;
			break;
		}
		if ((strcnt >= uap->count) 
		  && subyte(uap->buf+uap->count-1, 0) < 0) {
			error = EFAULT;
		}
		rvp->r_val1 = strcnt + 1;
		break;

	case SI_VERSION:
		getcnt = ((strcnt = strlen(utsname.version)) >= uap->count) ?
		  uap->count : strcnt + 1;
		if (copyout(utsname.version, uap->buf, getcnt)) {
			error = EFAULT;
			break;
		}
		if ((strcnt >= uap->count) 
		  && subyte(uap->buf+uap->count-1, 0) < 0) {
			error = EFAULT;
		}
		rvp->r_val1 = strcnt + 1;
		break;

	case SI_MACHINE:
		getcnt = ((strcnt = strlen(utsname.machine)) >= uap->count) ?
		  uap->count : strcnt + 1;
		if (copyout(utsname.machine, uap->buf, getcnt)) {
			error = EFAULT;
			break;
		}
		if ((strcnt >= uap->count) 
		  && subyte(uap->buf+uap->count-1, 0) < 0) {
			error = EFAULT;
		}
		rvp->r_val1 = strcnt + 1;
		break;

	case SI_ARCHITECTURE:
		getcnt = ((strcnt = strlen(architecture)) >= uap->count) ?
		  uap->count : strcnt + 1;
		if (copyout(architecture, uap->buf, getcnt)) {
			error = EFAULT;
			break;
		}
		if ((strcnt >= uap->count) 
		  && subyte(uap->buf+uap->count-1, 0) < 0) {
			error = EFAULT;
		}
		rvp->r_val1 = strcnt + 1;
		break;

	case SI_HW_SERIAL:
		getcnt = ((strcnt = strlen(hw_serial)) >= uap->count) ?
		  uap->count : strcnt + 1;
		if (copyout(hw_serial, uap->buf, getcnt)) {
			error = EFAULT;
			break;
		}
		if ((strcnt >= uap->count) 
		  && subyte(uap->buf+uap->count-1, 0) < 0) {
			error = EFAULT;
		}
		rvp->r_val1 = strcnt + 1;
		break;

	case SI_HW_PROVIDER:
		getcnt = ((strcnt = strlen(hw_provider)) >= uap->count) ?
		  uap->count : strcnt + 1;
		if (copyout(hw_provider, uap->buf, getcnt)) {
			error = EFAULT;
			break;
		}
		if ((strcnt >= uap->count) 
		  && subyte(uap->buf+uap->count-1, 0) < 0) {
			error = EFAULT;
		}
		rvp->r_val1 = strcnt + 1;
		break;

	case SI_GET_INET_DOMAIN:
		getcnt = ((strcnt = strlen(srpc_domain)) >= uap->count) ?
		  uap->count : strcnt + 1;
		if (copyout(srpc_domain, uap->buf, getcnt)) {
			error = EFAULT;
			break;
		}
		if ((strcnt >= uap->count) 
		  && subyte(uap->buf+uap->count-1, 0) < 0) {
			error = EFAULT;
		}
		rvp->r_val1 = strcnt + 1;
		break;

	case SI_SET_HOSTNAME:
		{
			struct vnode	*vp;
			int		len = 0;
			int		limit;
			char 		name[SYS_NMLN];
			char *c = name;

			if (!suser(u.u_cred)) {
				error = EPERM;
				break;
			}

			do {
				if (copyin(uap->buf++, c, 1)) {
					error = EFAULT;
					break;
				}
				len++;
			} while ((len < SYS_NMLN) && *c++);
			if (error)
				break;

			/* 
			 * must be non-NULL string and string
			 * must be less than SYS_NMLN chars.
			 */
			if ((len < 2) || ((len == SYS_NMLN) && (*c != '\0'))) {
				error = EINVAL;
				break;
			}
					
			/*
			 * Copy name into file /etc/nodename.
		 	 * NOTE:
			 * The name of the system is stored in a file for use
			 * when booting because the non-volatile RAM on the
			 * porting base will not allow storage of the full
			 * internet standard nodename.
			 * If sufficient non-volatile RAM is available on
			 * the hardware, however, storing the name there would
			 * be preferable to storing it in a file.
			 */
			limit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur >> SCTRSHFT;
			if (vn_open("/etc/nodename", UIO_SYSSPACE, 
		  	  FWRITE|FCREAT|FTRUNC, 0, &vp) != 0) {
				error = EFAULT;
				break;
			} else if (vn_rdwr(UIO_WRITE, vp, (caddr_t)name, 
	     	  	  len, 0, UIO_SYSSPACE, 0, limit, u.u_cred, 
			  NULL) != 0) {
				error = EFAULT;
				break;
			}
			/*
			 * Copy the name into the global utsname structure.
			 */
			strcpy(utsname.nodename, name);
			rvp->r_val1 = len;
			break;
        	}
			
	case SI_SET_INET_DOMAIN:
		{
			char name[SYS_NMLN];
			char *c = name;
			int len = 0;

			if (!suser(u.u_cred)) {
				error = EPERM;
				break;
			}
			do {
				if (copyin(uap->buf++, c, 1)) {
					error = EFAULT;
					break;
				}
				len++;
			} while ((len < SYS_NMLN) && *c++);
			if (error)
				break;
			/*
			 * If string passed in is longer than length
			 * allowed for domain name, fail.
			 */
			if ((len == SYS_NMLN) && (*c != '\0')) {
				error = EINVAL;
				break;
			}
			strcpy(srpc_domain, name);
			rvp->r_val1 = len;
			break;
		}

	default:
		error = EINVAL;
		break;
	}

	if (error)
		rvp->r_val1 = -1;
	return error;
}
