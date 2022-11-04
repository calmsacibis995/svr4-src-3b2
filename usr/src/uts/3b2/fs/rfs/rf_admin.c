/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/rfs/rf_admin.c	1.4.3.1"

/*
 * RFS administrative daemon and associated functions.
 */
#include "sys/list.h"
#include "sys/types.h"
#include "sys/sysinfo.h"
#include "sys/time.h"
#include "sys/fs/rf_acct.h"
#include "sys/systm.h"
#include "sys/param.h"
#include "sys/immu.h"
#include "sys/stream.h"
#include "sys/vnode.h"
#include "sys/cred.h"
#include "vm/seg.h"
#include "rf_admin.h"
#include "sys/rf_comm.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/rf_messg.h"
#include "sys/errno.h"
#include "sys/proc.h"
#include "sys/wait.h"
#include "sys/nserve.h"
#include "sys/rf_cirmgr.h"
#include "sys/rf_debug.h"
#include "sys/rf_sys.h"
#include "sys/inline.h"
#include "sys/rf_adv.h"
#include "sys/cmn_err.h"
#include "sys/debug.h"
#include "sys/vfs.h"
#include "sys/fs/rf_vfs.h"
#include "rf_serve.h"
#include "sys/stat.h"
#include "sys/statfs.h"
#include "rfcl_subr.h"
#include "sys/uio.h"
#include "rf_cache.h"
#include "vm/seg_kmem.h"
#include "sys/fcntl.h"
#include "sys/flock.h"
#include "sys/file.h"
#include "sys/session.h"

/* imports */
extern int	cleanlocks();
extern void	dst_clean();
extern int	nsrmount;
extern int	rf_state;
extern int	waitid();

proc_t		*rf_daemon_procp;
int		rf_daemon_flag;		/* reason for wakeup */
int		rf_daemon_lock;
rcvd_t		*rf_daemon_rd;		/* rd for rf_recovery */
proc_t		*rf_recovery_procp;	/* sleep address for rf_recovery */
int		rf_recovery_flag;	/* set to KILL when it's time to exit */

/* queue for user-level daemon */
ls_elt_t	rf_umsgq = { &rf_umsgq, &rf_umsgq };
int		rf_umsgcnt;

/* rf_async work queue */
STATIC ls_elt_t	rfa_workq = { &rfa_workq, &rfa_workq };

STATIC unsigned	n_rf_async = 3;
STATIC unsigned	nidle_rf_async;

STATIC void	rfadmin_reply();
STATIC void	rf_que_umsg();
STATIC void	rf_disable_cache();
STATIC void	rf_check_mount();
STATIC void	clean_SRD();
STATIC void	rf_srmntck();
STATIC void	clean_sndd();
STATIC void	dec_srmcnt();
STATIC void	rf_rec_cleanup();
STATIC void	rf_user_msg();
STATIC void	rf_cl_fumount();
STATIC void	clean_GRD();
STATIC void	rf_async();

/*
 * RFS administrative daemon, started when file sharing starts.
 * Forks new servers, calls rf_rec_cleanup() for rf_recovery.
 * To reduce stack size, rf_daemon returns when the invoking code
 * should become a server.  Otherwise it exits rather than returns.
 */
void
rf_daemon()
{
	int		s;
	int		i;
	sndd_t		reply_port;
	mblk_t		*bp;
	queue_t		*qp;
	rf_request_t	*request;
	rf_response_t	*response;
	rf_common_t	*cop;
	mblk_t		*resp_bp;
	char		usr_msg[ULINESIZ];	/* for user daemon message */
	int		error;
	int		nd;		/* n daemons */
	pid_t		xpid;		/* unwanted out param from newproc */

	/* disassociate this process from terminal */
	newsession();

	/* ignore all signals */
	for (i = 1; sigismember(&fillset, i); i++) {
		setsigact(i, SIG_IGN, 0, 0);
	}
	rf_daemon_flag = 0;
	u.u_procp->p_flag |= SNOWAIT;

	for (nd = error = 0; nd < n_rf_async && !error; nd++) {

		/* TO DO:  return to spawn these??? */

		switch (newproc(NP_FAILOK | NP_NOLAST | NP_SYSPROC, &xpid,
		  &error)) {
		case 0:
			break;
		case 1:
			rf_async();
		default:
			cmn_err(CE_WARN, "rf_async newproc failed");
		}
	}

	for (nd = error = 0; nd < minserve && !error; nd++) {
		switch (newproc(NP_FAILOK | NP_NOLAST | NP_SYSPROC, &xpid,
		  &error)) {
		case 0:
			break;
		case 1:
			return;
		default:
			/* Post signal to the last server so it won't sleep */
			if (rfsr_active_procp != NULL) {
				psignal(rfsr_active_procp, SIGUSR1);
			}
			break;
		}
	}
	if (nd != minserve) {
		minserve = nd;
		cmn_err(CE_WARN, "rf_daemon:  minserve reset to %d\n", nd);
	}

	/* Discard old user daemon messages. */
	while ((bp = (mblk_t *)LS_REMQUE(&rf_umsgq)) != NULL) {
		rf_freemsg(bp);
	}

	s = splstr();

	LS_INIT(&rf_umsgq);
	rf_umsgcnt = 0;
	reply_port.sd_stat = SDUSED;
	SDTOV(&reply_port)->v_count = 1;

	rf_state = RF_UP;
	/*
	 * end of critical section begun in rfstart
	 */
	wakeup((caddr_t)&rf_state);
	sleep((caddr_t)&rf_daemon_rd->rd_qslp, PRIBIO);
	for (;;) {
		register rf_message_t *msg;

		bp = rf_dequeue(rf_daemon_rd);
		if (!bp) {

			if (rf_daemon_flag & RFDDISCON) {
				register struct gdp *tmp;
				register struct gdp *endgdp = gdp + maxgdp;

				rf_daemon_flag &= ~RFDDISCON;
				splx(s);
				for (tmp = gdp; tmp < endgdp; tmp++) {
					if (tmp->constate == GDPDISCONN) {
						rf_rec_cleanup(tmp->queue);
						tmp->constate = GDPRECOVER;
					}
				}

				/* Give rf_async oppty to fail requests. */

				wakeup((caddr_t)&rfa_workq);

			} else if (rf_daemon_flag & RFDKILL) {
				k_siginfo_t	sink;

				splx(s);
				/*
				 * Signal all children and wait for them to die
				 */
				signal(rf_daemon_procp->p_pgrp, SIGKILL);
#ifdef DEBUG
				ASSERT(waitid(P_ALL, 0, &sink, WEXITED) ==
				  ECHILD);
#else
				(void)waitid(P_ALL, 0, &sink, WEXITED);
#endif
				rf_daemon_flag = 0;
				rf_daemon_procp = NULL;
				if (!rf_recovery_procp) {
					rf_commdinit();
					rf_state = RF_DOWN;
					wakeup((caddr_t)&rf_state);
				}
				exit(CLD_EXITED, 0);
			} else if (rf_daemon_flag & RFDSERVE) {
				rf_daemon_flag &= ~RFDSERVE;
				splx(s);
				switch (newproc(NP_FAILOK | NP_NOLAST |
				  NP_SYSPROC, &xpid, &error)) {
				case 0:
					break;
				case 1:
					return;
				default:
					/*
					 * Post signal to the last server so
					 * it won't sleep
					 */
					if (rfsr_active_procp != NULL) {
						psignal(rfsr_active_procp, 
						  SIGUSR1);
					}
					if (rfsr_nidle <= 1) {
						cmn_err(CE_NOTE,
						 "rf_daemon: out of servers\n");
					}
					break;
				}
			} else {
				sleep((caddr_t)&rf_daemon_rd->rd_qslp, PRIBIO);
				continue;
			}
			s = splstr();
			continue;
		}
		splx(s);
		msg = RF_MSG(bp);
		qp = (queue_t *)msg->m_queue;
		request = RF_REQ(bp);
		cop = RF_COM(bp);
		sndd_set(&reply_port, qp, msg->m_giftid);
		reply_port.sd_mntid = cop->co_mntid;
		switch ((int)cop->co_opcode) {
		case REC_FUMOUNT: {
			rf_vfs_t	*rf_vfsp = findrfvfs(cop->co_mntid);
			long		req_srmntid =
					  request->rq_rec_fumount.srmntid;

			rf_freemsg(bp);
			bp = NULL;
			/*
			 * We found an rf_vfs with the right mntid, now see
			 * if the resource is from the originating server.
			 * Note that this is still ONLY PROBABLISTIC, even
			 * though a little more cautious than SVR3.X.  If
			 * the rf_vfs has no rootvp, it's because the fumount
			 * or rf_recovery kicked off in the middle of the mount.
			 * It's still okay to go ahead, though, because we'll
			 * mark the rf_vfs fumounted, and will fail to find
			 * sndds or rcvds linked into the rf_vfs.
			 * Send message to user level only if mount is fully
			 * defined.
			 * Reply to the fumount request in any case.
			 */
			if (rf_vfsp) {
				if (rf_vfsp->rfvfs_rootvp &&
				  VTOSD(rf_vfsp->rfvfs_rootvp)->sd_queue ==
				    qp &&
				  VTOSD(rf_vfsp->rfvfs_rootvp)->sd_mntid ==
				    req_srmntid) {
					rf_cl_fumount(rf_vfsp);
					rf_user_msg(RFUD_FUMOUNT,
					    rf_vfsp->rfvfs_name,
					    (size_t)strlen(rf_vfsp->rfvfs_name));
				} else if(!rf_vfsp->rfvfs_rootvp) {
					rf_cl_fumount(rf_vfsp);
				}
			}
			rfadmin_reply(&reply_port, REC_FUMOUNT);
			break;
		}
		case REC_MSG: {
			size_t	datasz = (size_t)request->rq_rec_msg.count;
			size_t	hdrsz = msg->m_size - datasz;

			/*
			 * Got a message for user-level daemon.
			 * Enque message and wake up daemon.
			 */

			if (pullupmsg(bp, hdrsz, datasz)) {
				gdp_discon("rf_daemon bad REC_MSG", QPTOGP(qp));
				rf_freemsg(bp);
			} else {
				strncpy(usr_msg, rf_msgdata(bp, hdrsz), datasz);
				rf_freemsg(bp);
				rfadmin_reply(&reply_port, REC_MSG);
				rf_user_msg(RFUD_GETUMSG, usr_msg, datasz);
			}
			bp = NULL;
			break;
		}
		case RFSYNCTIME: {
			gdp_t	*gp = QPTOGP(qp);

			rfsr_adj_timeskew(gp, request->rq_synctime.time, 0);
			(void)rf_allocmsg(RFV1_MINRESP, (size_t)0,
			  BPRI_MED, FALSE, NULLCADDR, NULLFRP, &resp_bp);
			ASSERT(resp_bp);
			response = RF_RESP(resp_bp);
			cop = RF_COM(resp_bp);
			response->rp_synctime.time = hrestime.tv_sec;
			response->rp_errno = 0;
			cop->co_type = RF_RESP_MSG;
			cop->co_opcode = RFSYNCTIME;
			rf_clrrpsigs(response, gp->version);
			rf_freemsg(bp);
			bp = NULL;
			/* TO DO:  failure??? */
			(void)rf_sndmsg(&reply_port, resp_bp, RFV1_MINRESP,
			  (rcvd_t *)NULL, FALSE);
			break;
		}
		case RFCACHEDIS: {
			register queue_t *qp = (queue_t *)RF_MSG(bp)->m_queue;
			size_t respsize = RF_MIN_RESP(QPTOGP(qp)->version);
			int vcver = QPTOGP(qp)->version;
			/* vcode is defined only from newer servers */
			int vcode = vcver >= RFS2DOT0 ?
					request->rq_cachedis.vcode : 0;


			rf_disable_cache(qp, request->rq_cachedis.fhandle,
				      (ulong)vcode);
			rfc_info.rfci_rcv_dis++;
			rf_freemsg(bp);
			bp = NULL;
			(void)rf_allocmsg(respsize, (size_t)0, BPRI_MED,
			  FALSE, NULLCADDR, NULLFRP, &resp_bp);
			ASSERT(resp_bp);
			response = RF_RESP(resp_bp);
			cop = RF_COM(resp_bp);
			response->rp_errno = 0;
			cop->co_type = RF_RESP_MSG;
			cop->co_opcode = RFCACHEDIS;
			rf_clrrpsigs(response, vcver);
			(void)rf_sndmsg(&reply_port, resp_bp,
			     respsize, (rcvd_t *)NULL, FALSE);
			/*
			 * Link down is the only case that rf_sndmsg
			 * can fail, if it happens, server rf_recovery
			 * should take care of losing the message
			 */
			break;
		}
		default:
			rf_freemsg(bp);
			bp = NULL;
		}
		s = splstr();
	}
	/* NOTREACHED */
}

/*
 * Handle asynchronous RF_INACTIVE requests and other drudgery.
 * TO DO:  We currently have only one of these.  Expand interface
 * to support an arbitrary number, and then asynchronous read/page
 * ahead.  Keep nidle_rf_async current.
 */
STATIC void
rf_async()
{
	int		s;
	rfa_work_t	*wp;

	bcopy("rf_async", u.u_comm, sizeof("rf_async"));
	bcopy("rf_async", u.u_psargs, sizeof("rf_async"));

	/*
	 * rf_daemon may have issued a SIGKILL, but that would be discarded
	 * if this process was still in newproc() since the parent (rf_daemon)
	 * ignores all signals.	 Exit if RFS is stopping.
	 */
	if (rf_daemon_flag & RFDKILL) {
		exit(CLD_EXITED, 0);
	}

	nidle_rf_async++;

	setsigact(SIGKILL, SIG_DFL, 0, 0);

	do {

		--nidle_rf_async;
		s = splstr();
		while ((wp = (rfa_work_t *)LS_REMQUE(&rfa_workq)) != NULL) {
			ASSERT(wp->rfaw_func);
			splx(s);
			(*wp->rfaw_func)(wp);
			s = splstr();
		}
		splx(s);
		nidle_rf_async++;

	} while (!sleep((caddr_t)&rfa_workq, PREMOTE | PCATCH | PNOSTOP));

	s = splstr();
	while ((wp = (rfa_work_t *)LS_REMQUE(&rfa_workq)) != NULL) {
		splx(s);
		(*wp->rfaw_func)(wp);
		s = splstr();
	}
	splx(s);
			
	--nidle_rf_async;
	exit(CLD_EXITED, 0);
}

/*
 * Can return a nonzero errno iff wp->rfaw.canfail.  In that case,
 * the caller must dispose of its wp and otherwise see that its
 * work gets done (probably by calling wp->rfaw_func directly).
 */
int
rfa_workenq(wp)
	rfa_work_t	*wp;
{
	int		s;

	s = splstr();
	if (wp->rfaw_canfail && !LS_ISEMPTY(&rfa_workq) && !nidle_rf_async) {
		splx(s);
		return ENOMEM;
	}		
	LS_INSQUE(&rfa_workq, &wp->rfaw_elt);
	splx(s);
	wakeup((caddr_t)&rfa_workq);
	return 0;
}

/*
 * Recovery daemon, awakened by rf_rec_cleanup to clean up after
 * fumount or disconnect.  This part of rf_recovery calls
 * routines that can sleep.
 */
void
rf_recovery()
{
	int	sig;

	rf_recovery_flag = 0;

	/*
	 * Disassociate this process from terminal
	 * and ignore all signals.
	 */
	newsession();
	for (sig = 1; sigismember(&fillset, sig); sig++) {
		setsigact(sig, SIG_IGN, 0, 0);
	}

	for (;;) {
		while (rf_recovery_flag) {
			if (rf_recovery_flag & RFRECDISCON) {
				register gdp_t *endgdp = gdp + maxgdp;
				register gdp_t *gdpp;

				rf_recovery_flag &= ~RFRECDISCON;
				for (gdpp = gdp; gdpp < endgdp; gdpp++)
					if (gdpp->constate == GDPRECOVER)  {
						rf_check_mount(gdpp->queue);
						rf_srmntck();
						gdp_put_circuit(&gdpp->queue);
					}
			} else if (rf_recovery_flag & RFRECFUMOUNT) {
				rf_recovery_flag &= ~RFRECFUMOUNT;
				rf_srmntck();
			} else if (rf_recovery_flag & RFRECKILL) {
				/* RFS stop */
				rf_recovery_flag &= ~RFRECKILL;
				rf_recovery_procp = NULL;
				if (!rf_daemon_procp) {
					rf_commdinit();
					rf_state = RF_DOWN;
					wakeup((caddr_t)&rf_state);
				}
				exit(CLD_EXITED, 0);
			}
		}
		sleep((caddr_t)&rf_recovery_procp, PREMOTE);
	}
}

/*
 * Non-sleeping part of rf_recovery:  mark the resources
 * that need to be cleaned up, and awaken the recover
 * daemon to clean them.
 *
 * This routine is called by the rf_daemon when a circuit
 * gets disconnected and when a resource is fumounted
 * (server side of fumount). THIS ROUTINE MUST NOT SLEEP.
 * It must always be runnable to wake up servers
 * sleeping in resources that have been disconnected.  Otherwise
 * these servers and the recovery daemon can deadlock.
 */

STATIC void
rf_rec_cleanup(bad_q)
	queue_t	*bad_q;		/* stream that has gone away */
{
	register rf_resource_t *rsrcp = rf_resource_head.rh_nextp;
	sr_mount_t *srmp;
	sysid_t bad_sysid;

	clean_sndd(bad_q);
	clean_SRD(bad_q);

	/* Wakeup procs sleeping on stream head */
	wakeup((caddr_t)bad_q->q_ptr);

	bad_sysid = QPTOGP(bad_q)->sysid;

	/*
	 * Mark bad sr_mount entries on the
	 * resource that has mount from bad system and
	 * fumount is not in progress on that mount
	 */
	while (rsrcp != (rf_resource_t *)&rf_resource_head) {
		if ((srmp = id_to_srm(rsrcp, bad_sysid)) != 0 &&
		     !(srmp->srm_flags & SRM_FUMOUNT)) {
			rf_signal_serve(rsrcp->r_mntid, srmp);
			srmp->srm_flags |= SRM_LINKDOWN;
		}
		rsrcp = rsrcp->r_nextp;
	}
	rf_recovery_flag |= RFRECDISCON;
	wakeup((caddr_t)&rf_recovery_procp);
	return;
}


/*
 * Go through vfs list looking for remote mounts over bad stream.
 * Send message to user-level daemon for every mount with bad link.
 * (Kernel rf_recovery works without this routine.)
 */
STATIC void
rf_check_mount(bad_q)
	queue_t	*bad_q;
{
	register rf_vfs_t *rfvfsp = rf_head.rfh_next;

	do {
		/*
		 * rfvfsp->rfvfs_mntproc will be non-NULL iff
		 * a mount is in progress on that rf_vfs.
		 * In this case simply mark the associated rd LINKDOWN
		 * and signal the process sleeping waiting for
		 * the mount to complete.
		 * Else, the rf_vfs->rfvfs_rootvp will be set.
		 * Then the user must be notified of the dropped
		 * link and extra cleanup must be done.
		 */
		if (rfvfsp->rfvfs_mntproc) {
			clean_SRD(bad_q);
		} else if (VTOSD(rfvfsp->rfvfs_rootvp)->sd_queue == bad_q) {
			rfc_mountinval(rfvfsp);
			rf_user_msg(RFUD_DISCONN, rfvfsp->rfvfs_name,
			  (size_t)RFS_NMSZ);
		}
		rfvfsp = rfvfsp->rfvfs_next;
	} while (rfvfsp != rf_head.rfh_next);
}


/*
 *	Cleanup RDSPECIFIC RDs:
 *	Wakeup procs waiting for reply over stream that went bad.
 */

STATIC void
clean_SRD(bad_q)
	queue_t	*bad_q;		/* stream that has gone away */
{
	register rcvd_t		*rd;
	register rcvd_t		*endrcvd = rcvd + nrcvd;
	sndd_t			*sd;
	long			srm_mntid;

	for (rd = rcvd; rd < endrcvd; rd++)  {
		if (ACTIVE_SRD(rd) &&
		   (sd = rd->rd_sdp) != NULL &&
	    	   sd->sd_queue == bad_q) {
			srm_mntid = rd->rd_sdp->sd_mntid;
			rf_checkq(rd, srm_mntid);
			rd->rd_stat |= RDLINKDOWN;
			wakeup((caddr_t)&rd->rd_qslp);
		}
	}
}


/*
 * Check server mounts.  Wake server procs sleeping
 * in resource that went bad.  Pretend client gave up
 * references to rf_rsrc.
 */

STATIC void
rf_srmntck()
{
	rf_resource_t *rsrcp = rf_resource_head.rh_nextp;
	register rcvd_t *rd;
	queue_t * bad_q, *gdp_sysidtoq();

	while (rsrcp != (rf_resource_t *)&rf_resource_head) {
		register sr_mount_t *srmp = rsrcp->r_mountp;
		register rf_resource_t *rsrcnextp = rsrcp->r_nextp;

		while (srmp) {
			/* srmp becomes dangling reference in clean_GRD
			 */
			register sr_mount_t *srmnextp = srmp->srm_nextp;

			if (srmp->srm_flags & (SRM_LINKDOWN | SRM_FUMOUNT)) {
				register rcvd_t *endrcvd = rcvd + nrcvd;

				/* Wait for servers to wake, leave resource. */
				while (srmp->srm_slpcnt) {
					sleep((caddr_t)srmp, PZERO);
				}
				bad_q = gdp_sysidtoq(srmp->srm_sysid);
				ASSERT(bad_q);
				/* Hold vnode so it won't go away -
				 * VN_RELE in dec_srmcnt when doing unmount
				 */
				VN_HOLD(rsrcp->r_rootvp);
				/* Now clean up RDGENERAL RDs */
				for (rd = rcvd; rd < endrcvd; rd++) {
					if (ACTIVE_GRD(rd))
						clean_GRD(&rsrcp, rd,
							  bad_q, srmp);
					if (!rsrcp)
						break;
				}
			}
			srmp = srmnextp;
		}
		/* free this resource if unadvertised and not mounted */
		if (rsrcp && !rsrcp->r_mountp && (rsrcp->r_flags & R_UNADV)) {
			freersc(&rsrcp);
		}
		rsrcp = rsrcnextp;
	}
}

/*
 * On the server side, signal any server process sleeping
 * in this sr_mount. Count the servers we signal - we must
 * wait for them to finish.
 */
void
rf_signal_serve(srmntid, srmp)
	register long srmntid;
	register sr_mount_t *srmp;
{
	register sndd_t *sd;
	register sndd_t *endsndd = sndd + nsndd;
	register sysid_t sysid = srmp->srm_sysid;

	for (sd = sndd; sd < endsndd; sd++)
		/* TO DO:  make sure there is no race with srmount
		 */
		if (sd->sd_stat & SDSERVE
		    && sd->sd_stat & SDUSED
		    && sd->sd_srvproc
		    && sd->sd_mntid == srmntid
		    && QPTOGP(sd->sd_queue)->sysid == sysid) {
			sd->sd_stat |= SDLINKDOWN;
			psignal(sd->sd_srvproc, SIGTERM);
			srmp->srm_slpcnt++;
		}
}

/*
 * Link is down.  Trash send-descriptors that use it.
 */
STATIC void
clean_sndd(bad_q)
	queue_t	*bad_q;
{
	register sndd_t *sd;
	register sndd_t *endsndd = sndd + nsndd;

	for (sd = sndd; sd < endsndd; sd++) {
		if (sd->sd_stat & SDUSED && !(sd->sd_stat & SDSERVE) &&
		  sd->sd_queue == bad_q) {
			vnode_t	*vp = SDTOV(sd);

			/* CONSTCOND */
			rfc_disable(sd, (ulong)0);
			sd->sd_stat |= SDLINKDOWN;
			if (vp->v_flag & VROOT) {
				rfc_sdabort(VFTORF(vp->v_vfsp));
			}
		}
	}
}

/*
 * Clean RDGENERAL RDs.
 *
 * Traverse rd_user list of this RD.  For each rd_user from
 * this sr_mount index, pretend that client gave up all refs
 * to this RD.
 *
 * (Need bad_q to get sysid for cleanlocks.)
 */
/* ARGSUSED */
STATIC void
clean_GRD(rsrcpp, rd, bad_q, srmp)
	rf_resource_t **rsrcpp;
	rcvd_t	*rd;
	queue_t	*bad_q;
	sr_mount_t *srmp;
{
	register int vcount;
	vnode_t *vp;
	rd_user_t *rduptr;
	rd_user_t *rdup_next;
	long srm_mntid = (*rsrcpp)->r_mntid;
	cred_t cred;
	sysid_t badsysid = QPTOGP(bad_q)->sysid;

	bzero((caddr_t)&cred, sizeof(cred_t));
	crhold(&cred);
	rduptr = rd->rd_user_list;
	ASSERT(rduptr);

	rf_checkq(rd, srm_mntid); /* get rid of old messages */

	vp = rd->rd_vp;

	while (rduptr) {
		ASSERT(rd);
		/*
		 * Save the next rduser pointer in case the current
		 * rduser is freed.
		 */
		rdup_next = rduptr->ru_next;
		if (rduptr->ru_srmntid != srm_mntid
		    || QPTOGP(rduptr->ru_queue)->sysid != badsysid) {
			rduptr = rdup_next;
			continue;
		}

		/*
		 * Mimic what a server would do to get rid of reference.
		 */

		/* Undo opens and creats for this resource. */
		if (rduptr->ru_fcount > 0) {
			/* One VN_RELE because each rduser structure
			 * denotes a single vnode reference, given the
			 * present implementation of rfcl_sdrele.  This
			 * should not be a problem for VOP_CLOSEs(?)
			 */
			(void)cleanlocks(vp, IGN_PID, badsysid);
			while (rduptr->ru_frcnt) {
				VOP_CLOSE(vp, FREAD, rduptr->ru_fcount,
					0, &cred);
				rduptr->ru_frcnt--;
				rduptr->ru_fcount--;
			}
			while (rduptr->ru_fwcnt) {
				VOP_CLOSE(vp, FWRITE, rduptr->ru_fcount,
					0, &cred);
				rduptr->ru_fwcnt--;
				rduptr->ru_fcount--;
			}
			while (rduptr->ru_fcount) {
				VOP_CLOSE(vp, FWRITE | FREAD, rduptr->ru_fcount,
					0, &cred);
				rduptr->ru_fcount--;
			}
		}
		ASSERT(!rduptr->ru_fcount);

		/* Clean up references to this resource. */
		for (vcount = rduptr->ru_vcount; vcount > 0; --vcount) {
			dec_srmcnt(rsrcpp, srmp, bad_q);
			rcvd_delete(&rd, QPTOGP(bad_q)->sysid, srm_mntid);
			VN_RELE(vp);
		}
		rduptr = rdup_next;
	} /* end while */
}


/*
 * Decrement the reference count in the sr_mount structure.
 * If it goes to zero, do the unmount.
 */
STATIC void
dec_srmcnt(rsrcpp, srmp, bad_q)
	rf_resource_t **rsrcpp;
	sr_mount_t *srmp;
	queue_t *bad_q;
{
	vnode_t *vp;
	gdp_t *gdpp;

	if (srmp->srm_refcnt > 1) { /* srumount wants refcnt of 1 */
		--srmp->srm_refcnt;
		return;
	}

	/*
	 * Giving up last ref for this sr_mount entry - free it.
	 * The vnode we're working on is NOT necessarily
	 * the root of the resource we're unmounting.
	 */
	vp = (*rsrcpp)->r_rootvp;

	(void)srm_remove(rsrcpp, srmp);

	/* One extra VN_RELE for VN_HOLD in rf_srmntck and fumount. */
	VN_RELE(vp);

	/* Client usually cleans up circuit, but client is gone. */
	gdpp = QPTOGP(bad_q);
	--gdpp->mntcnt;
}

/*
 *  An sr_mount entry went bad (disconnect or fumount),
 *  and rf_recovery is cleaning it up.  Throw away any old
 *  messages that are on this rd for the bad entry.
 */
void
rf_checkq(rd, srmid)
	rcvd_t	*rd;
	long srmid;
{
	register int cnt, qcnt, slevel;	/* locals */
	mblk_t  * bp;
	long msg_srmid;

	slevel = splstr();

	qcnt = rd->rd_qcnt;
	for (cnt = 0; cnt < qcnt; cnt++) {
		bp = (mblk_t *)LS_REMQUE(&rd->rd_rcvdq);
		ASSERT(bp);
		msg_srmid = RF_COM(bp)->co_mntid;
		if (msg_srmid == srmid) {
			/* don't service this message - toss it */
			rf_freemsg(bp);
			bp = NULL;
			rd->rd_qcnt--;
			if (RCVDEMP(rd)) {
				rfsr_rmmsg(rd);
				break;
			}
		} else {
			/* message OK - put it back */
			LS_INSQUE(&rd->rd_rcvdq, bp);
		}
	}
	splx(slevel);
}

/*
 * Create message for local user-level daemon, and enque it.
 */
STATIC void
rf_user_msg(opcode, name, size)
	int		opcode;
	char		*name;
	size_t		size;
{
	mblk_t		*bp;
	struct 	u_d_msg	*request;

	if ((bp = allocb(sizeof(struct u_d_msg), BPRI_MED)) == NULL) {
		cmn_err(CE_NOTE, "rf_user_msg allocb fails: ");
		cmn_err(CE_CONT, "resource %s disconnected", name);
		return;
	}
	request = (struct u_d_msg *)bp->b_wptr;
	request->opcode = opcode;
	strcpy(request->res_name, name);
	request->count = size;
	rf_que_umsg(bp);
}

/*
 * Send reply with opcode over destination SD.
 */
STATIC void
rfadmin_reply(dest, opcode)
	sndd_t		*dest;		/* reply path */
	int		opcode;		/* what we did */
{
	mblk_t		*resp_bp;
	rf_response_t	*response;
	rf_common_t	*cop;
	size_t		respsize = RF_MIN_RESP(QPTOGP(dest->sd_queue)->version);

	(void)rf_allocmsg(respsize, (size_t)0, BPRI_MED, FALSE,
	  NULLCADDR, NULLFRP, &resp_bp);
	ASSERT(resp_bp);
	response = RF_RESP(resp_bp);
	cop = RF_COM(resp_bp);
	response->rp_errno = 0;
	cop->co_type = RF_RESP_MSG;
	cop->co_opcode = opcode;
	rf_clrrpsigs(response, QPTOGP(dest->sd_queue)->version);
	/* TO DO:  failure??? */
	(void)rf_sndmsg(dest, resp_bp, respsize, (rcvd_t *)NULL, FALSE);
}

/*
 * If there's room on user_daemon queue, enque message and wake daemon.
 */
STATIC void
rf_que_umsg(bp)
	mblk_t	*bp;
{
	register int	s = splstr();

	++rf_umsgcnt;
	LS_INIT(bp);
	LS_INSQUE(&rf_umsgq, bp);
	splx(s);
	wakeup((caddr_t)&rf_daemon_lock);
}

/*
 * vcode should be 0 if unknown
 */
STATIC void
rf_disable_cache(qp, fhandle, vcode)
	register queue_t *qp;
	register long	fhandle;
	register ulong	vcode;
{
	register sndd_t *sdp;
	register sndd_t	*endsdp = sndd + nsndd;

	for (sdp = sndd; sdp < endsdp; sdp ++) {
		if ((sdp->sd_stat & SDUSED || !LS_ISEMPTY(&sdp->sd_hash)) &&
		  sdp->sd_queue == qp &&
		  sdp->sd_fhandle == fhandle) {
			rfc_disable(sdp, vcode);
			break;
		}
	}
}

/*
 * Client side of forced unmount.
 * Set MFUMOUNT flag in rf_vfs, in case fumount message preceeded completion
 * of another message defining a gift.  Code setting up gift sndds is
 * responsible for checking this flag.
 *
 * Mark SDs that point into resource being force-unmounted.  Wake up
 * processes waiting for reply over this mount point.
 *
 */
STATIC void
rf_cl_fumount(rf_vfsp)
	register rf_vfs_t *rf_vfsp;
{
	register sndd_t *sd;
	register sndd_t *endsndd = sndd + nsndd;
	register rcvd_t *rd;
	register rcvd_t *endrcvd = rcvd + nrcvd;
	register vfs_t *vfsp = RFTOVF(rf_vfsp);	/* upper level */

	rf_vfsp->rfvfs_flags |= MFUMOUNT;
	DUPRINT2(DB_MNT_ADV, "rf_cl_fumount: rf_vfsp %x\n", rf_vfsp);
	rfc_mountinval(rf_vfsp);
	for (sd = sndd; sd < endsndd; sd++) {
		if (sd->sd_stat & SDUSED && SDTOV(sd)->v_vfsp == vfsp) {
			DUPRINT2(DB_MNT_ADV, "sd %x fumounted\n", sd);
			sd->sd_stat |= SDLINKDOWN;
			dst_clean(sd);
		}
	}
	/*
	 * RD waiting for reply has SD in rd_sdp field
	 */
	for (rd = rcvd; rd < endrcvd; rd++)  {
		sd = rd->rd_sdp;
		if (ACTIVE_SRD(rd) && sd && SDTOV(sd)->v_vfsp == vfsp) {
			DUPRINT2(DB_MNT_ADV,
			    "fumount: waking SRD for rf_vfsp %x\n", rf_vfsp);
			rd->rd_stat |= RDLINKDOWN;
			wakeup((caddr_t)&rd->rd_qslp);
		}
	}
	wakeup((caddr_t)&rfa_workq);
}
