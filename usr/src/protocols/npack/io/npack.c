/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)npack:io/npack.c	1.7.1.7"

/*
 *	PACK - Positive ACKnowledgement protocol
 * 
 *	Supports TLI Interface for Connection Oriented Transport Service
 */
#include "sys/types.h"
#include "sys/sysmacros.h"
#ifdef uts
#include "sys/mplock.h"
#endif
#ifndef u3b2
#include "sys/dir.h"
#endif
#include "sys/param.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/strlog.h"
#include "sys/log.h"
#include "sys/tihdr.h"
#include "sys/tiuser.h"
#include "sys/debug.h"
#include "sys/signal.h"
#include "sys/psw.h"
#ifdef u3b2
#include "sys/sbd.h"
#include "sys/pcb.h"
#include "sys/cmn_err.h"
#endif
#include "sys/user.h"
#ifdef uts
#include "sys/k200user.h"
#endif
#include "sys/npack.h"
#include "sys/errno.h"
#include "sys/dlpi.h"
#include "sys/rf_debug.h"

#ifdef uts
/* Get rid of spl's for uts only */
#define splstr()	1
#define splx(s)
#endif

/*
 *	These are defined in master file
 */
extern	int PKTIME;
extern	int NRETRNS;
extern	int ACKTIME;
extern	int CREDIT;
extern	int MONITORTIME;
extern	int nvc;
extern	struct pck_vc pck_vc[];
extern	long pckminor[];

/*
 *  stream data structure definitions
 */
STATIC int pckopen(), pckclose(), pckuwput(), pckuwsrv();
STATIC int pcklwsrv(), pcklrput(), pckursrv();

#define TEMPSIZE	1024		/* gets updated during dl_bind */

STATIC struct module_info pck_uinfo = {
	PACK_ID, "pack", 0, TEMPSIZE, TEMPSIZE, TEMPSIZE/2
};
STATIC struct module_info pck_linfo = {
	PACK_ID, "pack", 0, TEMPSIZE, TEMPSIZE*3, TEMPSIZE
};
STATIC struct qinit pckurinit = {
	NULL, pckursrv, pckopen, pckclose, NULL, &pck_uinfo, NULL
};
STATIC struct qinit pckuwinit = {
	pckuwput, pckuwsrv, NULL, NULL, NULL, &pck_uinfo, NULL
};
STATIC struct qinit pcklrinit = {
	pcklrput, NULL, NULL, NULL, NULL, &pck_linfo, NULL
};
STATIC struct qinit pcklwinit = {
	NULL, pcklwsrv, NULL, NULL, NULL, &pck_linfo, NULL
};
struct streamtab pckinfo = {
	&pckurinit, &pckuwinit, &pcklrinit, &pcklwinit
};

STATIC ushort	pck_prottype = 0x8010;		/* pack protocol type */
STATIC mblk_t	*iocmp;				/* msg blk for M_IOCACK */
STATIC struct	pckdev pckdev;
STATIC ushort	seq_counter;

/*
 *	TLI state transition table
 */
extern char ti_statetbl[TE_NOEVENTS][TS_NOSTATES];
#define NEXTSTATE(X, Y)		ti_statetbl[X][Y]
#define BADSTATE		127		/* unreachable state */

/*
 *   Functions that syntax check the TLI request primitives
 *   and completely execute any local mgmt primitives
 */
STATIC void pck_chk_creq(), pck_chk_cres(), pck_chk_discon(), pck_chk_data(),
	pck_chk_info(), pck_chk_bind(), pck_chk_unbind(), pck_chk_optmgmt();

STATIC void (*chk_request[])() = {
	pck_chk_creq,	/* 0 - T_CONN_REQ */
	pck_chk_cres,	/* 1 - T_CONN_RES */
	pck_chk_discon,	/* 2 - T_DISCON_REQ */
	pck_chk_data,	/* 3 - T_DATA_REQ */
	pck_chk_data,	/* 4 - T_EXDATA_REQ */
	pck_chk_info,	/* 5 - T_INFO_REQ */
	pck_chk_bind,	/* 6 - T_BIND_REQ */
	pck_chk_unbind,	/* 7 - T_UNBIND_REQ */
	NULL,		/* 8 - T_UNITDATA_REQ:  not supported */
	pck_chk_optmgmt,	/* 9 - T_OPTMGMT_REQ */
};

/*
 *   Functions that send the TLI request primitives
 *	to a remote user.
 */
STATIC int pck_snd_creq(), pck_snd_cres(), pck_snd_discon(), pck_snd_data();

STATIC int (*snd_request[])() = {
	pck_snd_creq,	/* 0 - T_CONN_REQ */
	pck_snd_cres,	/* 1 - T_CONN_RES */
	pck_snd_discon,	/* 2 - T_DISCON_REQ */
	pck_snd_data,	/* 3 - T_DATA_REQ */
	pck_snd_data,	/* 4 - T_EXDATA_REQ */
};

/*
 *   Functions that process incoming PACK Control Packets
 */
STATIC int pck_rcv_connect(), pck_rcv_accept(), pck_rcv_discon();

STATIC int (*rcv_ctrl[])() = {
	pck_rcv_connect,	/* 0 - PM_CONNECT */
	pck_rcv_accept,	/* 1 - PM_ACCEPT */
	pck_rcv_discon,	/* 2 - PM_DISCONNECT */
};
#define NCTRL	PM_DISCONNECT+1

/*
 * Other functions
 */
STATIC long pck_findlink();
STATIC void pck_recvpkt();
STATIC void pck_term();
STATIC int pck_addrcmp();
STATIC void pck_updisconn();
STATIC int pck_send_flush();
STATIC void pck_send_ok_ack();
STATIC void pck_send_error_ack();
STATIC void pck_send_fatal();
STATIC void pck_putwq_inorder();
STATIC void pck_finished();
STATIC int pck_adjctrl();
STATIC int pck_ctrlpkt();
STATIC void pck_senddisconn();
STATIC void pck_sendconnect();
STATIC void pck_retrns();
STATIC void pck_sendout();
STATIC void pck_sendack();
STATIC int pck_sendpkt();
STATIC void packmonitor();

#ifdef uts
mblk_t *pck_recvack();
void pck_acktimeout();
mblk_t *pck_deque();
void pck_lnktimeout();
void pck_enque();
#else
STATIC mblk_t *pck_recvack();
STATIC void pck_acktimeout();
STATIC mblk_t *pck_deque();
STATIC void pck_lnktimeout();
STATIC void pck_enque();
#endif

/*
 * Mapping of TLI T_primitive types to state machine events
 *
 * T_INFO_REQ is removed since it doesn't affect state, and
 *	T_UNITDATA_REQ and T_ORDREL_REQ are not supported.
 */
#define NPRIM T_ORDREL_REQ+1
#define BADEVENT TE_NOEVENTS
#define NONEVENT 0

STATIC int prim_to_event[NPRIM] = {
	TE_CONN_REQ,		/*  0 - T_CONN_REQ */
	TE_CONN_RES,		/*  1 - T_CONN_RES */
	TE_DISCON_REQ,		/*  2 - T_DISCON_REQ */
	TE_DATA_REQ,		/*  3 - T_DATA_REQ */
	TE_EXDATA_REQ,		/*  4 - T_EXDATA_REQ */
	NONEVENT,		/*  5 - T_INFO_REQ */
	TE_BIND_REQ,		/*  6 - T_BIND_REQ */
	TE_UNBIND_REQ,		/*  7 - T_UNBIND_REQ */
	BADEVENT,		/*  8 - T_UNITDATA_REQ */
	TE_OPTMGMT_REQ,		/*  9 - T_OPTMGMT_REQ */
	BADEVENT,		/* 10 - T_ORDREL_REQ */
};

#ifdef DEBUG
int pcklog = 0;
#endif

int
pckinit()
{
	int i;

	pckdev.qbot = NULL;
	pckdev.linkstate = UNLINKED;
	for (i = 0; i < nvc; i++)
		pckminor[i] = -1;
	iocmp = NULL;
	return(0);
}

STATIC int
pckopen(q, dev, flag, sflag)
register queue_t *q;
{
	register struct pck_vc *vcp;
	register struct pckseq *seq;
	register struct stroptions *sop;
	register mblk_t *mp;

	PCKLOG(PACK_ID, VC_NUM(q->q_ptr), 0, SL_TRACE,
		       "pckopen: called with dev %d\n", dev, 0, 0);

	/* is it already opened */
	if (q->q_ptr) {
		PCKLOG(PACK_ID, VC_NUM(q->q_ptr), 0, SL_TRACE,
		       "pckopen: opened already, open succeeded\n", 0, 0, 0);
		return(VC_NUM(q->q_ptr));
	}

	if (sflag == CLONEOPEN) {
		for (vcp = pck_vc; vcp < &pck_vc[nvc]; vcp++)
			if (vcp->vc_rdq == NULL)
				break;
		if (vcp >= &pck_vc[nvc]) {
			PCKLOG(PACK_ID, -1, 0, SL_TRACE|SL_ERROR,
			       "pckopen: opened failed: couldn't allocate pck_vc data structure for q = %x\n", q, 0, 0);
			u.u_error = ENXIO;
			return(OPENFAIL);
		}
	} else {
		dev = minor(dev);
		if (dev >= nvc) {
			PCKLOG(PACK_ID, -1, 0, SL_TRACE|SL_ERROR,
			       "pckopen: opened failed: bad dev = %d\n", dev, 0, 0);
			u.u_error = ENXIO;
			return(OPENFAIL);
		}
		vcp = &pck_vc[dev];
	}

	/* initialize data structure */
	vcp->vc_state = TS_UNBND;
	vcp->vc_flags = 0;
	vcp->vc_rdq = q;
	vcp->vc_qlen = 0;
	for (seq = vcp->vc_seq; seq < &vcp->vc_seq[MAX_CONN_IND]; seq++)
		seq->srclink = -1;	/* mark unused */
	vcp->vc_seqcnt = 0;
	vcp->vc_retrnsq.rq_head = NULL;
	vcp->vc_qcnt = 0;
	vcp->vc_cpsn = 0;
	vcp->vc_epsn = 0;
	vcp->vc_rackno = 0;
	vcp->vc_sackno = 0;
	vcp->vc_lnktimer = -1;
	vcp->vc_acktimer = -1;
	vcp->vc_dstlink = -1;
	vcp->vc_creqmp = NULL;
	vcp->vc_reason = 0;
	q->q_ptr = (caddr_t)vcp;
	WR(q)->q_ptr = (caddr_t)vcp;
	/* pckursrv should only be run by being back enabled - i.e. getq */
	noenable(q);

	/*
	 * Allocate buffer for DL_UNITDATA_REQ messages.
	 */
	if ((mp = allocb(DATREQSIZE, BPRI_MED)) == NULL) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pckopen: can't allocb dl_unitdata_req msg\n", 0, 0, 0);
		q->q_ptr = NULL;
		WR(q)->q_ptr = NULL;
		vcp->vc_rdq = NULL;
		u.u_error = EAGAIN;
		return(OPENFAIL);
	}
	vcp->vc_datreq = mp;

	/*
	 *  Set up hi-lo water marks and min-max pkt size
	 *  at stream head to correspond to pack values
	 */
	if ((mp = allocb(sizeof(struct stroptions), BPRI_HI)) == NULL) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pckopen: can't allocb stroptions msg\n", 0, 0, 0);
		q->q_ptr = NULL;
		WR(q)->q_ptr = NULL;
		vcp->vc_rdq = NULL;
		u.u_error = EAGAIN;
		return(OPENFAIL);
	}
	mp->b_datap->db_type = M_SETOPTS;
	mp->b_wptr += sizeof(struct stroptions);
	sop = (struct stroptions *)mp->b_rptr;
	sop->so_flags = SO_MINPSZ | SO_MAXPSZ | SO_HIWAT | SO_LOWAT;
	sop->so_minpsz = q->q_minpsz;
	sop->so_maxpsz = q->q_maxpsz;
	sop->so_lowat = q->q_lowat;
	sop->so_hiwat = q->q_hiwat;
	putnext(q, mp);

	PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		"pckopen: open succeeded\n", 0, 0, 0);
	return(VC_NUM(vcp));
}

STATIC int
pckclose(q)
register queue_t *q;
{
	register struct pck_vc *vcp = (struct pck_vc *)q->q_ptr;
	register i;
	mblk_t *mp;

	ASSERT(vcp != NULL);

	pck_finished(q, VC_CLOSE);

	for (i = 0; i < nvc; i++)
		if (pckminor[i] == VC_NUM(vcp)) {
			pckminor[i] = -1;
			break;
		}
	vcp->vc_flags = 0;
	vcp->vc_rdq = NULL;

	while (mp = (mblk_t *) pck_deque(&vcp->vc_retrnsq))
		freemsg(mp);
	vcp->vc_qcnt = 0;
	if ((pckdev.respq == q) && (pckdev.linkstate == WAITLINK)) {
		pckdev.respq = NULL;
		pckdev.qbot = NULL;
		pckdev.linkstate = UNLINKED;
	}
	freemsg(vcp->vc_datreq);
	vcp->vc_datreq = NULL;

	PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
	       "pckclose: close succeeded\n", 0, 0, 0);
	return(0);
}

/*
 * pckuwput - Upper side write queue put procedure.
 *
 *	Performs syntax checking and processes local
 *	management primitves.  Anything going to remote
 *	user is queued for processing by svc procedure.
 */
STATIC int
pckuwput(q, mp)
register queue_t *q;
register mblk_t *mp;
{
	register struct pck_vc *vcp;
	register union T_primitives *prim;
	struct iocblk *iocp;
	register mblk_t *amp;

	vcp = (struct pck_vc *)q->q_ptr;
	ASSERT(vcp != NULL);

	switch(mp->b_datap->db_type) {

	case M_DATA:
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
	       		"pckuwput: M_DATA type received mp = %x\n", mp, 0, 0);
		if (pckdev.linkstate != LINKED) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
				"pckuwput: mux not linked\n", 0, 0, 0);
			pck_send_fatal(q, mp);
			return(0);
		}
		putq(q, mp);
		return(0);

	case M_PROTO:
	case M_PCPROTO:
		if (pckdev.linkstate != LINKED) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
				"pckuwput: mux not linked\n", 0, 0, 0);
			pck_send_fatal(q, mp);
			return(0);
		}

		/*
		 *  Perform syntax checks on all requests, and fully
		 *  process any local mgmt requests.
		 */
		ASSERT((mp->b_wptr - mp->b_rptr) >= sizeof(long));
		prim = (union T_primitives *)mp->b_rptr;
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		       "pckuwput: TImsg type = %d received\n", prim->type, 0, 0);

		if (prim->type >= NPRIM) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		       	   "pckuwput: bad primitive type %d\n", prim->type, 0, 0);
			pck_send_fatal(q, mp);
			return(0);
		}
		if (prim_to_event[prim->type] == BADEVENT) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		       		"pckuwput: unsupported primitive type %d\n",
				prim->type, 0, 0);
			pck_send_error_ack(q, mp, TNOTSUPPORT, 0);
			return(0);
		}

		(*chk_request[prim->type])(q, mp);
		return(0);

	case M_IOCTL:
		ASSERT((mp->b_wptr - mp->b_rptr) == sizeof(struct iocblk));
		iocp = (struct iocblk *)mp->b_rptr;
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
	       		"pckuwput: M_IOCTL cmd %o recieved mp = %x\n",
	       		iocp->ioc_cmd, mp, 0);
		switch (iocp->ioc_cmd) {

		case I_LINK:
			ASSERT((mp->b_cont->b_wptr - mp->b_cont->b_rptr)
				== sizeof(struct linkblk));
			if (pckdev.linkstate != UNLINKED) {
				iocp->ioc_error = EINVAL;
				mp->b_datap->db_type = M_IOCNAK;
			} else {
				register struct linkblk *lp;
				dl_info_req_t *reqp;

				lp = (struct linkblk *)mp->b_cont->b_rptr;
				if (!canput(lp->l_qbot)) {
					PCKLOG(PACK_ID, -1, 0, SL_TRACE,
					    "pckuwput: lower queue full\n", 0, 0, 0);
					iocp->ioc_error = EAGAIN;
					mp->b_datap->db_type = M_IOCNAK;
					break;
				}

				/*
				 * We know sizeof(struct linkblk) is greater
				 * than sizeof(dl_info_req_t).
				 */
				pckdev.qbot = lp->l_qbot;
				pckdev.linkstate = WAITLINK;
				pckdev.respq = RD(q);
				iocp->ioc_count = 0;
				amp = mp->b_cont;
				mp->b_cont = NULL;
				iocmp = mp;
				amp->b_datap->db_type = M_PROTO;
				amp->b_rptr = amp->b_datap->db_base;
				amp->b_wptr = amp->b_rptr + DL_INFO_REQ_SIZE;
				reqp = (dl_info_req_t *)amp->b_rptr;
				reqp->dl_primitive = DL_INFO_REQ;
				PCKLOG(PACK_ID, -1, 0, SL_TRACE,
				    "pckuwput: sending DL_INFO_REQ to driver\n",
				    0, 0, 0);
				putnext(pckdev.qbot, amp);
				return(0);
			}
			iocp->ioc_count = 0;
			freemsg(unlinkb(mp));
			break;

		case I_UNLINK:
			if (pckdev.linkstate != LINKED) {
				iocp->ioc_error = EINVAL;
				mp->b_datap->db_type = M_IOCNAK;
			} else {
				mp->b_datap->db_type = M_IOCACK;
				pck_term(q, pckdev.qbot);
				pckdev.qbot = NULL;
				pckdev.linkstate = UNLINKED;
			}
			iocp->ioc_count = 0;
			freemsg(unlinkb(mp));
			break;

		default:
			mp->b_datap->db_type = M_IOCNAK;
			break;
		}
		qreply(q, mp);
		return(0);

 	case M_FLUSH:
		
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
	       		"pckuwput: M_FLUSH type recieved mp = %x\n", mp, 0, 0);
		/*
		 * Flush both queues as appropriate and turn
		 * back upstream (not sending on to driver)
		 */
		if (*mp->b_rptr & FLUSHW) {
			flushq(q, FLUSHALL);
			*mp->b_rptr &= ~FLUSHW;
		}
		if (*mp->b_rptr & FLUSHR) {
			flushq(RD(q), FLUSHALL);
			qreply(q, mp);
			vcp->vc_flags &= ~(VC_LOCALFC|VC_SENDSTOP);
			vcp->vc_flags |= VC_SENDACK;
			/*
		 	*	send back ack immediately
		 	*/
			if (pckdev.linkstate == LINKED)
				pck_sendack(vcp, 0);
		} else
			freemsg(mp);
		return(0);
		
	default:
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
	       		"pckuwput: unknown STR msg type %o received mp = %x\n",
	       		mp->b_datap->db_type, mp, 0);
		freemsg(mp);
		return(0);
	}
}

/*
 *  The receiver has drained the upper queues.  Push as many messages
 *  as possible upstream.  The first data message will force an ACKP
 *  packet back to the transmitter.
 */
STATIC int
pckursrv(q)
register queue_t *q;
{
	register mblk_t *mp;
	register struct pck_vc *vcp;

	vcp = (struct pck_vc *)q->q_ptr;

	PCKLOG(FLOW_ID, VC_NUM(vcp), 0, SL_TRACE,
		"pckursrv: local receiver freed up\n", 0, 0, 0);

	vcp->vc_flags &= ~VC_LOCALFC;
	vcp->vc_flags |= VC_URSRV;

	/* process data queued on vc_rdq */
	while (canput(q->q_next) && (mp = getq(q)) != NULL)
		pck_recvpkt(mp);

	if (q->q_first) {
		/* Could not push all local data upstream */
		PCKLOG(FLOW_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pckursrv: could not send all data upstream\n", 0, 0, 0);

		/* Ack all currently received data */
		if ((vcp->vc_flags & VC_SENDACK) && (pckdev.linkstate == LINKED))
			pck_sendack(vcp, 0);

		vcp->vc_flags |= VC_LOCALFC;
	}
	vcp->vc_flags &= ~VC_URSRV;
}

/*
 *  pckuwsrv - pack upper service routine for write queue
 *
 *	Process all outgoing messages (only M_DATA's or M_PROTO's)
 */
STATIC int
pckuwsrv(q)
register queue_t *q;
{
	register mblk_t *mp;
	register struct pck_vc *vcp;
	register s;
	union T_primitives *prim;

	vcp = (struct pck_vc *)q->q_ptr;
	if (pckdev.linkstate != LINKED) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pckuwsrv: mux not linked\n", 0, 0, 0);
		flushq(q, FLUSHALL);
		return(0);
	}

	if (vcp->vc_flags & VC_LNKTOUT) {
		vcp->vc_flags &= ~VC_LNKTOUT;
		pck_retrns(vcp);
	}
	if (vcp->vc_flags & VC_ACKTOUT) {
		vcp->vc_flags &= ~VC_ACKTOUT;
		pck_sendack(vcp, 0);
	}
	if (vcp->vc_flags & VC_SANITY) {
		if (!canput(pckdev.qbot->q_next))
			return(FLOWCNTL);
		if ((mp = allocb(MINBUFSZ, BPRI_MED)) == NULL) {
			bufcall(MINBUFSZ, BPRI_MED, pckuwsrv, q);
			return(0);
		}
		vcp->vc_flags &= ~VC_SANITY;
		mp->b_wptr += MINPKSIZE;
		if (pck_sendpkt(mp, vcp, SANITYP) != TRUE)
			freemsg(mp);
	}
	if (vcp->vc_flags & VC_REMFC)
		/* Remote receiver is flow controlled */
		return(0);

	while((mp = getq(q)) != NULL) {
		if (!canput(pckdev.qbot->q_next)) {
			putbq(q, mp);
			return(FLOWCNTL);
		}

		s = splstr();
		if (mp->b_datap->db_type == M_DATA) {
			if ((vcp->vc_state == TS_IDLE) || (vcp->vc_flags & VC_FATAL)) {
				PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
	       			   "pckuwsrv: data IDLE or FATAL - free msg\n", 0, 0, 0);
				freemsg(mp);
				splx(s);
				continue;
			}
			if (NEXTSTATE(TE_DATA_REQ, vcp->vc_state) == BADSTATE) {
				PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
	       			    "pckuwsrv: data message out of state\n", 0, 0, 0);
				pck_send_fatal(q, mp);
				splx(s);
				continue;
			}
			if (pck_snd_data(q, mp) == FALSE) {
				putbq(q, mp);
				splx(s);
				return(0);
			}
		} else {
			prim = (union T_primitives *)mp->b_rptr;
			if ((prim->type == T_DATA_REQ) || (prim->type == T_EXDATA_REQ)) {
				if ((vcp->vc_state == TS_IDLE) || (vcp->vc_flags & VC_FATAL)) {
					PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
	       				   "pckuwsrv: data IDLE or FATAL - free msg\n", 0, 0, 0);
					freemsg(mp);
					splx(s);
					continue;
				}
				if (NEXTSTATE(TE_DATA_REQ, vcp->vc_state) == BADSTATE) {
					PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
	       				   "pckuwsrv: data message out of state\n", 0, 0, 0);
					pck_send_fatal(q, mp);
					splx(s);
					continue;
				}
			} else {
				if (NEXTSTATE(prim_to_event[prim->type], vcp->vc_state) == BADSTATE) {
					pck_send_error_ack(q, mp, TOUTSTATE, 0);
					splx(s);
					continue;
				}
			}
			if ((*snd_request[prim->type])(q, mp) == FALSE) {
				putbq(q, mp);
				splx(s);
				return(0);
			}
		}
		splx(s);
	}
	return(0);
}

/*
 *  pcklwsrv - pack lower write service procedure.
 *
 *	No messages are ever placed on this queue.
 *	This will get scheduled when the downstream device queue
 *	reaches its low water mark, and it will back-enable
 *	all virtual circuits linked through the device.
 *
 *	Virtual circuits are scheduled in a round-robin fashion.
 */
STATIC int
pcklwsrv(q)
register queue_t *q;
{
	register i;

	PCKLOG(PACK_ID, -1, 0, SL_TRACE, "pcklwsrv: q %x\n", q, 0, 0);
	for (i = pckdev.lastrun + 1; i < nvc; i++) {
		if (pck_vc[i].vc_rdq)
			if (pckuwsrv(WR(pck_vc[i].vc_rdq)) == FLOWCNTL) {
				pckdev.lastrun = i - 1;
				return(0);
			}
	}
	for (i = 0; i <= pckdev.lastrun; i++) {
		if (pck_vc[i].vc_rdq)
			if (pckuwsrv(WR(pck_vc[i].vc_rdq)) == FLOWCNTL)
				break;
	}
	pckdev.lastrun = i - 1; 
	return(0);
}

/*
 *  pcklrput - pack lower read put procedure.
 *
 *	Process incoming M_PROTO's and place M_DATA msgs
 *	in stream head read queue of appropriate VC.
 */
STATIC int
pcklrput(q, mp)
register struct queue *q;
register mblk_t *mp;
{
	register struct pck_vc *vcp;
	register PACKHDR *pktp;
	union DL_primitives *p;
	mblk_t	*amp;
	int	l;
	mblk_t *tmp;
	struct iocblk *iocp;

	PCKLOG(PACK_ID, -1, 0, SL_TRACE, "pcklrput: q %x mp %x type %o\n",
		q, mp, mp->b_datap->db_type);

	switch (mp->b_datap->db_type) {
	case M_PCPROTO:
	case M_PROTO:
		p = (union DL_primitives *)mp->b_rptr;
		switch (p->dl_primitive) {
		case DL_UNITDATA_IND:
			PCKLOG(PACK_ID, -1, 0, SL_TRACE,
			    "pcklrput: got DL_UNITDATA_IND", 0, 0, 0);
			tmp = mp->b_cont;
			mp->b_cont = NULL;
			freeb(mp);
			mp = tmp;
			mp->b_rptr += HDROFFSET;
			if (pullupmsg(mp, -1) == 0) {
				PCKLOG(PACK_ID, -1, 0, SL_TRACE,
				    "pcklrput: can't pullup incoming pkt\n", 0, 0, 0);
				freemsg(mp);
				break;
			}
			pktp = (PACKHDR *)mp->b_rptr;
			if ((int)pktp->pk_dstlink >= nvc
			  || (pckminor[pktp->pk_dstlink] == -1)) {
				PCKLOG(PACK_ID, -1, 0, SL_TRACE,
					"pcklrput: invalid dst link %x\n",
					pktp->pk_dstlink, 0, 0);
				freemsg(mp);
				break;
			}
			vcp = &pck_vc[pckminor[pktp->pk_dstlink]];
			if (vcp->vc_rdq == NULL) {
				PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
					"pcklrput: msg for closed link\n", 0, 0, 0);
				freemsg(mp);
				break;
			}

			/* Process non-DATA portions of message mp */
			if (pck_recvack(mp) == NULL)
				break;

			if (!canput(vcp->vc_rdq->q_next)) {

				/* Ack all currently received data */
				if ((vcp->vc_flags & VC_SENDACK) &&
				    (pckdev.linkstate == LINKED))
					pck_sendack(vcp, 0);

				/* flow control transmitter */
				PCKLOG(FLOW_ID, VC_NUM(vcp), 0, SL_TRACE,
					"pcklrput: send stop to remote transmitter\n", 0, 0, 0);
				vcp->vc_flags |= (VC_SENDSTOP + VC_LOCALFC);
				if (pckdev.linkstate == LINKED)
					pck_sendack(vcp, 0);

				/*
				 * The first message queued locally is marked
				 * SNDACK to force an ACKP back to the transmitter
				 * after the upper queues have freed up.
				 */
				if ((l = qsize(vcp->vc_rdq)) == 0)
					pktp->pk_type |= SNDACK;

				/* Queue one transmit window locally */
				if (l >= CREDIT) {
					/*
					 * This is a backward compatability hook
					 * and allows us to talk to NPACK
					 * providers that don't recognize the
					 * STOPP message.
					 */
					PCKLOG(FLOW_ID, VC_NUM(vcp), 0, SL_TRACE,
						"pcklrput: msg discarded, queue full\n", 0, 0, 0);
					freemsg(mp);
					break;
				}
				putq(vcp->vc_rdq, mp);
				break;
			}
			if (vcp->vc_rdq->q_first || (vcp->vc_flags&VC_URSRV)) {
				/*
				 * There is still data on read queue!
				 * Queue the data locally and make sure service
				 * procedure is scheduled to run.
				 */
				l = qsize(vcp->vc_rdq);

				/* Queue one transmit window locally */
				if (l >= CREDIT) {
					/*
					 * This is a backward compatability hook
					 * and allows us to talk to NPACK
					 * providers that don't recognize the
					 * STOPP message.
					 */
					PCKLOG(FLOW_ID, VC_NUM(vcp), 0, SL_TRACE,
						"pcklrput: msg discarded, queue full\n", 0, 0, 0);
					freemsg(mp);
					break;
				}
				PCKLOG(FLOW_ID, VC_NUM(vcp), 0, SL_TRACE,
					"pcklrput: vc_rdq not empty or ursrv active: q locally\n", 0, 0, 0);
				putq(vcp->vc_rdq, mp);
				qenable(vcp->vc_rdq);
				break;
			}
			pck_recvpkt(mp);
			break;

		case DL_INFO_ACK:
		    {
			dl_info_ack_t *ackp;
			dl_bind_req_t *reqp;

			PCKLOG(PACK_ID, -1, 0, SL_TRACE,
			    "pcklrput: got DL_INFO_ACK\n", 0, 0, 0);
			if (pckdev.linkstate == WAITLINK) {
				ackp = (dl_info_ack_t *)mp->b_rptr;
				if ((ackp->dl_service_mode != DL_CLDLS) ||
				    (ackp->dl_provider_style != DL_STYLE1)) {
					iocmp->b_datap->db_type = M_IOCNAK;
					putnext(pckdev.respq, iocmp);
					iocmp = NULL;
					pckdev.linkstate = UNLINKED;
					pckdev.respq = NULL;
					pckdev.qbot = NULL;
					return;
				}

				/*
				 * Adjust interface parameters based on
				 * the data link provider.
				 */
				pck_uinfo.mi_maxpsz = ackp->dl_max_sdu - PCKHDRSZ - HDROFFSET;
				pck_uinfo.mi_hiwat = pck_uinfo.mi_maxpsz * CREDIT;
				pck_uinfo.mi_lowat = pck_uinfo.mi_maxpsz;
				pck_linfo.mi_maxpsz = ackp->dl_max_sdu;
				pck_linfo.mi_hiwat = ackp->dl_max_sdu * CREDIT;
				pck_linfo.mi_lowat = ackp->dl_max_sdu;

				/*
				 * Adjust parameters of active queues.
				 */
				for (vcp = pck_vc; vcp < &pck_vc[nvc]; vcp++) {
					if (vcp->vc_rdq) {
						register queue_t *vq;

						vq = vcp->vc_rdq;
						vq->q_hiwat = pck_uinfo.mi_hiwat;
						vq->q_lowat = pck_uinfo.mi_lowat;
						vq->q_maxpsz = pck_uinfo.mi_maxpsz;
						vq = WR(vq);
						vq->q_hiwat = pck_uinfo.mi_hiwat;
						vq->q_lowat = pck_uinfo.mi_lowat;
						vq->q_maxpsz = pck_uinfo.mi_maxpsz;
					}
				}
				q->q_hiwat = pck_linfo.mi_hiwat;
				q->q_lowat = pck_linfo.mi_lowat;
				q->q_maxpsz = pck_linfo.mi_maxpsz;
				WR(q)->q_hiwat = pck_linfo.mi_hiwat;
				WR(q)->q_lowat = pck_linfo.mi_lowat;
				WR(q)->q_maxpsz = pck_linfo.mi_maxpsz;

				/*
				 * Generate a DL_BIND_REQ to bind to a DLSAP.
				 * We know sizeof(dl_info_ack_t) is greater
				 * than sizeof(dl_bind_req_t).
				 */
				mp->b_datap->db_type = M_PROTO;
				mp->b_rptr = mp->b_datap->db_base;
				mp->b_wptr = mp->b_rptr + sizeof(dl_bind_req_t);
				reqp = (dl_bind_req_t *)mp->b_rptr;
				reqp->dl_primitive = DL_BIND_REQ;
				reqp->dl_sap = (ulong)pck_prottype;
				reqp->dl_max_conind = 0;
				reqp->dl_service_mode = DL_CLDLS;
				reqp->dl_conn_mgmt = 0;
				qreply(q, mp);
			} else {
				freemsg(mp);
			}
			break;
		    }

		case DL_BIND_ACK:
		    {
			dl_bind_ack_t *ackp;
			unchar *cp;

			PCKLOG(PACK_ID, -1, 0, SL_TRACE,
			    "pcklrput: got DL_BIND_ACK\n", 0, 0, 0);
			if (pckdev.linkstate == WAITLINK) {
				ackp = (dl_bind_ack_t *)mp->b_rptr;
				if ((ackp->dl_sap != (long)pck_prottype) ||
				    (ackp->dl_addr_length != NETADDRLEN)) {
					iocmp->b_datap->db_type = M_IOCNAK;
					putnext(pckdev.respq, iocmp);
					iocmp = NULL;
					pckdev.linkstate = UNLINKED;
					pckdev.respq = NULL;
					pckdev.qbot = NULL;
					freemsg(mp);
					break;
				}

				/*
				 * Finally ack the I_LINK ioctl.
				 */
				iocmp->b_datap->db_type = M_IOCACK;
				putnext(pckdev.respq, iocmp);
				iocmp = NULL;
				pckdev.linkstate = LINKED;
				pckdev.respq = NULL;

				/*
				 * save the physical network address
				 */
				cp = mp->b_rptr + ackp->dl_addr_offset;
				bcopy(cp, pckdev.phy_naddr, NETADDRLEN);
				freemsg(mp);

				/* 
				 *	start monitor program
				 */
				packmonitor();
			} else {
				freemsg(mp);
			}
			break;
		    }

		case DL_ERROR_ACK:
		    {
			dl_error_ack_t *ackp;
			struct iocblk *iocp;

			ackp = (dl_error_ack_t *)mp->b_rptr;
			PCKLOG(PACK_ID, -1, 0, SL_TRACE,
			    "pcklrput: got DL_ERROR_ACK, err %d, unix err %d\n",
			    ackp->dl_errno, ackp->dl_unix_errno, 0);
			if (pckdev.linkstate == WAITLINK) {
				iocmp->b_datap->db_type = M_IOCNAK;
				iocp = (struct iocblk *)iocmp->b_rptr;
				iocp->ioc_error = ackp->dl_unix_errno;
				putnext(pckdev.respq, iocmp);
				iocmp = NULL;
				pckdev.linkstate = UNLINKED;
				pckdev.respq = NULL;
				pckdev.qbot = NULL;
			}
			freemsg(mp);
			break;
		    }

		case DL_OK_ACK:
			PCKLOG(PACK_ID, -1, 0, SL_TRACE,
			    "pcklrput: got DL_OK_ACK\n", 0, 0, 0);
			freemsg(mp);
			break;

		case DL_UDERROR_IND:
			PCKLOG(PACK_ID, -1, 0, SL_TRACE,
			    "pcklrput: got DL_UDERROR_IND\n", 0, 0, 0);
			freemsg(mp);
			break;

		default:
			PCKLOG(PACK_ID, -1, 0, SL_TRACE,
			    "pcklrput: got unknown DL primitive %d\n",
			    p->dl_primitive, 0, 0);
			freemsg(mp);
			break;
		}
		break;

 	case M_FLUSH:

		/*
		 * Flush read queue free msg (can't route upstream)
		 */
		PCKLOG(PACK_ID, -1, 0, SL_TRACE, "pcklrput: got M_FLUSH\n", 0, 0, 0);
		if (*mp->b_rptr & FLUSHR)
			flushq(q, FLUSHALL);
		if (*mp->b_rptr & FLUSHW) {
			*mp->b_rptr &= ~FLUSHR;
			flushq(WR(q), FLUSHALL);
			qreply(q, mp);
		} else
			freemsg(mp);
		break;

	default:
		PCKLOG(PACK_ID, -1, 0, SL_TRACE,
			"pcklrput: invalid msg type %o\n",
			mp->b_datap->db_type, 0, 0);
		freemsg(mp);
		break;
	}
	return(0);
}

STATIC void
pck_chk_data(q, mp)
register queue_t *q;
register mblk_t *mp;
{
	register struct pck_vc *vcp;

	vcp = (struct pck_vc *)q->q_ptr;
	ASSERT(vcp != NULL);
	ASSERT((mp->b_wptr - mp->b_rptr) == sizeof(struct T_data_req));
	pck_putwq_inorder(q, mp);
}

STATIC void
pck_chk_info(q, mp)
queue_t *q;
register mblk_t *mp;
{
	register struct T_info_ack *info_ack;
	struct pck_vc *vcp;
	mblk_t *ack;

	if ((ack = allocb(sizeof(struct T_info_ack), BPRI_HI)) == NULL) {
		PCKLOG(PACK_ID, VC_NUM(q->q_ptr), 0, SL_TRACE,
		     	"pck_chk_info: couldn't allocate buffer for info_ack\n", 0, 0, 0);
		pck_send_error_ack(q, mp, TSYSERR, EAGAIN);
		return;
	}
	vcp = (struct pck_vc *)q->q_ptr;
	freemsg(mp);
	ack->b_wptr = ack->b_rptr + sizeof(struct T_info_ack);
	ack->b_datap->db_type = M_PCPROTO;
	info_ack = (struct T_info_ack *)ack->b_rptr;
	info_ack->PRIM_type = T_INFO_ACK;
	info_ack->TSDU_size = pck_uinfo.mi_maxpsz;
	info_ack->ETSDU_size = ETSDU_SIZE;
	info_ack->CDATA_size = CDATA_SIZE;
	info_ack->DDATA_size = DDATA_SIZE;
	info_ack->ADDR_size = ADDR_SIZE;
	info_ack->OPT_size = OPT_SIZE;
	info_ack->TIDU_size = pck_uinfo.mi_maxpsz;
	info_ack->SERV_type = T_COTS;
	info_ack->CURRENT_state = vcp->vc_state;
	qreply(q, ack);
}

STATIC void
pck_chk_bind(q, mp)
queue_t *q;
register mblk_t *mp;
{
	register union T_primitives *prim;
	register struct pck_vc *vcp;

	prim = (union T_primitives *)mp->b_rptr;
	vcp = (struct pck_vc *)q->q_ptr;

	ASSERT((mp->b_wptr - mp->b_rptr) >= sizeof(struct T_bind_req));
	ASSERT((mp->b_wptr - mp->b_rptr) >= (prim->bind_req.ADDR_offset +
		prim->bind_req.ADDR_length));

	if (NEXTSTATE(prim_to_event[prim->type], vcp->vc_state) == BADSTATE) {
		pck_send_error_ack(q, mp, TOUTSTATE, 0);
		return;
	}
	vcp->vc_state = NEXTSTATE(prim_to_event[prim->type], vcp->vc_state);
	vcp->vc_qlen = min(prim->bind_req.CONIND_number, MAX_CONN_IND);

	if (prim->bind_req.ADDR_length == 0) {
		register struct pckaddr *addrp;
		register link;
		mblk_t	*new;

		if ((new = allocb(sizeof(struct T_bind_ack)
				+ sizeof(struct pckaddr), BPRI_HI)) == NULL) {
			vcp->vc_qlen = 0;
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			    "pck_chk_bind: couldn't alloc buffer for bind_ack\n", 0, 0, 0);
			vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
			pck_send_error_ack(q, mp, TSYSERR, EAGAIN);
			return;
		}
		freemsg(mp);
		mp = new;
		prim = (union T_primitives *)mp->b_rptr;

		/*
		 * Find free link number for this minor device.
		 * First try link that matches minor dev.  If not
		 * free, scan table for free entry.
		 */
		if (pckminor[VC_NUM(vcp)] == -1)
			link = VC_NUM(vcp);
		else {
			if ((link = pck_findlink()) < 0) {
				vcp->vc_qlen = 0;
				vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
				pck_send_error_ack(q, mp, TNOADDR, 0);
				return;
			}
		}
		pckminor[link] = VC_NUM(vcp);
		vcp->vc_srclink = link;
		prim->bind_ack.ADDR_offset = sizeof(struct T_bind_ack);
		prim->bind_ack.ADDR_length = sizeof(struct pckaddr);
		mp->b_wptr = mp->b_rptr + prim->bind_ack.ADDR_offset;
		addrp = (struct pckaddr *)mp->b_wptr;
		bcopy(pckdev.phy_naddr, addrp->phynaddr, NETADDRLEN);
		addrp->link = link;
		mp->b_wptr += sizeof(struct pckaddr);
	} else {
		struct pckaddr addr;

		if (prim->bind_req.ADDR_length != sizeof(struct pckaddr)) {
			vcp->vc_qlen = 0;
			vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
			pck_send_error_ack(q, mp, TBADADDR, 0);
			return;
		}
		bcopy((char *)(mp->b_rptr + prim->bind_req.ADDR_offset),
			(char *)&addr, sizeof(struct pckaddr));
		bcopy(pckdev.phy_naddr, addr.phynaddr, NETADDRLEN);
		if ((addr.link >= nvc) || (addr.link < 0)) {
			vcp->vc_qlen = 0;
			vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
			pck_send_error_ack(q, mp, TBADADDR, 0);
			return;
		}
#if 0
		if (pckminor[addr.link] == -1)
			pckminor[addr.link] = VC_NUM(vcp);
		else {
			if ((addr.link = pck_findlink()) < 0) {
				vcp->vc_qlen = 0;
				vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
				pck_send_error_ack(q, mp, TNOADDR, 0);
				return;
			}
		}
#else
		/*
		 * We need to set pckminor[addr.link] in all cases instead
		 * of as above.  The above code was causing invalid dst lnk 
		 * messages in pcklrput.
		 */
		if (pckminor[addr.link] != -1) {
			if ((addr.link = pck_findlink()) < 0) {
				vcp->vc_qlen = 0;
				vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
				pck_send_error_ack(q, mp, TNOADDR, 0);
				return;
			}
		}
		pckminor[addr.link] = VC_NUM(vcp);
#endif
		vcp->vc_srclink = addr.link;
		bcopy((char *)&addr, (char *)(mp->b_rptr + prim->bind_req.ADDR_offset),
			sizeof(struct pckaddr));
	}
	prim->bind_ack.PRIM_type = T_BIND_ACK;
	prim->bind_ack.CONIND_number = vcp->vc_qlen;
	mp->b_datap->db_type = M_PCPROTO;
	vcp->vc_state = NEXTSTATE(TE_BIND_ACK, vcp->vc_state);
	qreply(q, mp);
}

STATIC void
pck_chk_unbind(q, mp)
queue_t *q;
register mblk_t *mp;
{
	register struct pck_vc *vcp;
	register i;
	union T_primitives *prim;


	ASSERT((mp->b_wptr - mp->b_rptr) >= sizeof(struct T_unbind_req));
	prim = (union T_primitives *)mp->b_rptr;
	vcp = (struct pck_vc *)q->q_ptr;
	
	if (NEXTSTATE(prim_to_event[prim->type], vcp->vc_state) == BADSTATE) {
		pck_send_error_ack(q, mp, TOUTSTATE, 0);
		return;
	}
	vcp->vc_state = NEXTSTATE(prim_to_event[prim->type], vcp->vc_state);
	if (!pck_send_flush(RD(q))) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		    	"pck_chk_unbind: pck_send_flush failed\n", 0, 0, 0);
		vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
		pck_send_error_ack(q, mp, TSYSERR, EPROTO);
		return;
	}
	for (i = 0; i < nvc; i++)
		if (pckminor[i] == VC_NUM(vcp)) {
			pckminor[i] = -1;
			break;
		}
	ASSERT(i < nvc);
	vcp->vc_qlen = 0;
	vcp->vc_state = NEXTSTATE(TE_OK_ACK1, vcp->vc_state);
	pck_send_ok_ack(q, mp, T_UNBIND_REQ);
}

STATIC void
pck_chk_optmgmt(q, mp)
queue_t *q;
register mblk_t *mp;
{
	register union T_primitives *prim;
	register struct pck_vc *vcp;

	prim = (union T_primitives *)mp->b_rptr;
	vcp = (struct pck_vc *)q->q_ptr;

	ASSERT((mp->b_wptr - mp->b_rptr) >= sizeof(struct T_optmgmt_req));
	ASSERT((mp->b_wptr - mp->b_rptr) >= (prim->optmgmt_req.OPT_offset +
		prim->optmgmt_req.OPT_length));
		
	if (NEXTSTATE(prim_to_event[prim->type], vcp->vc_state) == BADSTATE) {
		pck_send_error_ack(q, mp, TOUTSTATE, 0);
		return;
	}
	vcp->vc_state = NEXTSTATE(prim_to_event[prim->type], vcp->vc_state);

	switch(prim->optmgmt_req.MGMT_flags) {

	case T_DEFAULT: {
		register mblk_t *tmp;
		struct pckopt *optp;

		if ((tmp = allocb(sizeof(struct T_optmgmt_ack)+sizeof(struct pckopt),
			BPRI_HI)) == NULL) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		   	    "pck_chk_optmgmt: couldn't alloc buffer for optmgmt_ack\n", 0, 0, 0);
			vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
			pck_send_error_ack(q, mp, TSYSERR, EAGAIN);
			return;
		}
		freemsg(mp);
		prim = (union T_primitives *)tmp->b_rptr;
		tmp->b_datap->db_type = M_PCPROTO;
		tmp->b_wptr = tmp->b_rptr + sizeof(struct T_optmgmt_ack) +
			sizeof(struct pckopt);
		prim->optmgmt_ack.MGMT_flags = T_DEFAULT;
		prim->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
		prim->optmgmt_ack.OPT_length = sizeof(struct pckopt);
		prim->optmgmt_ack.OPT_offset = sizeof(struct T_optmgmt_ack);
		optp = (struct pckopt *)(tmp->b_rptr + sizeof(struct T_optmgmt_ack));
		optp->nretry = DFLT_NRETRY;
		optp->interval = DFLT_INTERVAL;
		vcp->vc_state = NEXTSTATE(TE_OPTMGMT_ACK, vcp->vc_state);
		qreply(q, tmp);
		return;
		}

	case T_CHECK:
		if ((prim->optmgmt_req.OPT_length != sizeof(struct pckopt)) ||
		    (prim->optmgmt_req.OPT_offset < sizeof(struct T_optmgmt_req))) {
			vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
			pck_send_error_ack(q, mp, TBADOPT, 0);
			return;
		}

		/*
		 * Any values are fine.
		 */
		prim->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
		prim->optmgmt_ack.MGMT_flags |= T_SUCCESS;
		mp->b_datap->db_type = M_PCPROTO;
		vcp->vc_state = NEXTSTATE(TE_OPTMGMT_ACK, vcp->vc_state);
		qreply(q, mp);
		return;
		
	case T_NEGOTIATE: {
		struct pckopt opt;

		/*
		 * No negotiation -- accept any values sent down
		 */
		if ((prim->optmgmt_req.OPT_length != sizeof(struct pckopt)) ||
		    (prim->optmgmt_req.OPT_offset < sizeof(struct T_optmgmt_req))) {
			vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
			pck_send_error_ack(q, mp, TBADOPT, 0);
			return;
		}
		bcopy((char *)(mp->b_rptr + prim->optmgmt_req.OPT_offset),
			(char *)&opt, sizeof(struct pckopt));
		vcp->vc_nretry = opt.nretry;
		vcp->vc_interval = opt.interval;
		prim->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
		mp->b_datap->db_type = M_PCPROTO;
		vcp->vc_state = NEXTSTATE(TE_OPTMGMT_ACK, vcp->vc_state);
		qreply(q, mp);
		return;
		}
		
	default:
		vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
		pck_send_error_ack(q, mp, TBADFLAG, 0);
		return;
	}
}

STATIC void
pck_chk_creq(q, mp)
queue_t *q;
register mblk_t *mp;
{
	register union T_primitives *prim;

	prim = (union T_primitives *)mp->b_rptr;

	ASSERT((mp->b_wptr - mp->b_rptr) >= sizeof(struct T_conn_req));
	ASSERT((mp->b_wptr - mp->b_rptr) >= (prim->conn_req.DEST_length +
		prim->conn_req.DEST_offset));
	ASSERT((mp->b_wptr - mp->b_rptr) >= (prim->conn_req.OPT_length +
		prim->conn_req.OPT_offset));

	if (prim->conn_req.DEST_length != sizeof(struct pckaddr)) {
		pck_send_error_ack(q, mp, TBADADDR, 0);
		return;
	}

	if (prim->conn_req.OPT_length != 0)
		if (prim->conn_req.OPT_length != sizeof(struct pckopt)) {
			pck_send_error_ack(q, mp, TBADOPT, 0);
			return;
		}
	if (msgdsize(mp) > CDATA_SIZE) {
		pck_send_error_ack(q, mp, TBADDATA, 0);
		return;
	}
	pck_putwq_inorder(q, mp);
}

STATIC void
pck_chk_cres(q, mp)
queue_t *q;
register mblk_t *mp;
{
	register union T_primitives *prim;
	register struct pck_vc *vcp;

	prim = (union T_primitives *)mp->b_rptr;
	vcp = (struct pck_vc *)q->q_ptr;

	ASSERT((mp->b_wptr - mp->b_rptr) >= sizeof(struct T_conn_res));
	ASSERT((mp->b_wptr - mp->b_rptr) >= (prim->conn_res.OPT_length +
		prim->conn_res.OPT_offset));

	if (prim->conn_res.OPT_length != 0)
		if (prim->conn_res.OPT_length != sizeof(struct pckopt)) {
			pck_send_error_ack(q, mp, TBADOPT, 0);
			return;
		}
	if ((vcp->vc_seqcnt > 1) && (prim->conn_res.QUEUE_ptr == OTHERQ(q))) {
		pck_send_error_ack(q, mp, TBADF, 0);
		return;
	}
	if (msgdsize(mp) > CDATA_SIZE) {
		pck_send_error_ack(q, mp, TBADDATA, 0);
		return;
	}
	pck_putwq_inorder(q, mp);
}

STATIC void
pck_chk_discon(q, mp)
queue_t *q;
register mblk_t *mp;
{
	ASSERT((mp->b_wptr - mp->b_rptr) >= sizeof(struct T_discon_req));

	if (msgdsize(mp) > DDATA_SIZE) {
		pck_send_error_ack(q, mp, TBADDATA, 0);
		return;
	}
	pck_putwq_inorder(q, mp);
}

STATIC int
pck_snd_data(q, mp)
queue_t *q;
register mblk_t *mp;
{
	register mblk_t	*protomp = NULL;
	union	T_primitives *prim;
	struct	pck_vc *vcp;
	int	rval;
	int	type;

	vcp = (struct pck_vc *)q->q_ptr;

	if (mp->b_datap->db_type == M_DATA)
		type = DATAP;
	else {
		prim = (union T_primitives *)mp->b_rptr;
		if (prim->type == T_DATA_REQ)
			type = DATAP;
		else
			type = DATAP | EXDATAP;
		protomp = mp;
		mp = mp->b_cont;
		protomp->b_cont = NULL;
	}
	if ((rval = pck_sendpkt(mp, vcp, type)) != TRUE) {
		if (protomp)
			linkb(protomp, mp);
		if (rval == NEEDBUF)
			bufcall(MINBUFSZ, BPRI_MED, pckuwsrv, q);
		return(FALSE);
	}
	if (protomp)
		freeb(protomp);
	return(TRUE);
}

STATIC int
pck_snd_creq(q, mp)
queue_t *q;
register mblk_t *mp;
{
	register int s;
	register struct pck_vc *vcp;
	register struct T_conn_req *conn_req;
	register struct pm_connect *conn_ind;
	register mblk_t *tmp = NULL;
	mblk_t	*okmp, *bp;
	struct pckaddr *addrp, addr;
	PACKHDR	*pktp;
	dl_unitdata_req_t *dlrp;

	vcp = (struct pck_vc *)q->q_ptr;

	s = splstr();
	if (vcp->vc_lnktimer != -1) {
		untimeout(vcp->vc_lnktimer);
		vcp->vc_lnktimer = -1;
		freemsg(vcp->vc_creqmp);
		vcp->vc_creqmp = NULL;
	}
	splx(s);
	if ((okmp = allocb(sizeof(struct T_ok_ack), BPRI_HI)) == NULL) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		  	"pck_snd_creq: can't alloc ok_ack\n", 0, 0, 0);
		bufcall(sizeof(struct T_ok_ack), BPRI_HI, pckuwsrv, q);
		return(FALSE);
	}
	if ((mp->b_datap->db_lim - mp->b_datap->db_base) < MINBUFSZ) {
		if ((tmp = allocb(MINBUFSZ, BPRI_MED)) == NULL) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
				"pck_snd_creq: couldn't alloc buffer for conn_ind\n", 0, 0, 0);
			freeb(okmp);
			bufcall(MINBUFSZ, BPRI_MED, pckuwsrv, q);
			return(FALSE);
		}
	}

	conn_req = (struct T_conn_req *)mp->b_rptr;
	vcp->vc_state = NEXTSTATE(prim_to_event[T_CONN_REQ], vcp->vc_state);
	bcopy((char *)(mp->b_rptr + conn_req->DEST_offset),
		(char *)&addr, sizeof(struct pckaddr));
	bcopy(addr.phynaddr, vcp->vc_dstnadr, NETADDRLEN);
	vcp->vc_dstlink = addr.link;
	vcp->vc_datreq->b_rptr = vcp->vc_datreq->b_datap->db_base;
	dlrp = (dl_unitdata_req_t *)vcp->vc_datreq->b_rptr;
	dlrp->dl_primitive = DL_UNITDATA_REQ;
	dlrp->dl_dest_addr_offset = DL_UNITDATA_REQ_SIZE;
	dlrp->dl_dest_addr_length = NETADDRLEN;
	vcp->vc_datreq->b_wptr = vcp->vc_datreq->b_rptr + DL_UNITDATA_REQ_SIZE;
	bcopy(vcp->vc_dstnadr, vcp->vc_datreq->b_wptr, NETADDRLEN);
	vcp->vc_datreq->b_wptr += NETADDRLEN;
	vcp->vc_datreq->b_datap->db_type = M_PROTO;
	if ((bp = copyb(vcp->vc_datreq)) == NULL) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pck_snd_creq: couldn't dup buffer for DL_UNITDATA_REQ\n", 0, 0, 0);
		freeb(okmp);
		if (tmp)
			freeb(tmp);
		bufcall(DATREQSIZE, BPRI_MED, pckuwsrv, q);
		return(FALSE);
	}

	if (conn_req->OPT_length != 0) {
		struct pckopt opt;

		bcopy((char *)(mp->b_rptr + conn_req->OPT_offset),
			(char *)&opt, sizeof(struct pckopt));
		vcp->vc_nretry = opt.nretry;
		vcp->vc_interval = opt.interval;
	} else {
		vcp->vc_nretry = DFLT_NRETRY;
		vcp->vc_interval = DFLT_INTERVAL;
	}

	if (tmp) {
		if (mp->b_cont)
			linkb(tmp, unlinkb(mp));
		freeb(mp);
		mp = tmp;
	}
	mp->b_datap->db_type = M_DATA;
	mp->b_rptr = mp->b_datap->db_base + HDRWORD;
	mp->b_wptr = mp->b_rptr + MINPKSIZE - HDROFFSET;
	pktp = (PACKHDR *)mp->b_rptr;
	pktp->pk_type = CTRLP;
	pktp->pk_size = msgdsize(mp) + COMPATSIZE;
	conn_ind = (struct pm_connect *)(mp->b_rptr + PCKHDRSZ);
	conn_ind->cmd = PM_CONNECT;
	mp->b_rptr -= HDROFFSET;

	/*
	 * Place address in appropriate spot for T_conn_ind so
	 * that the receiver won't have to move it around.
	 */
	conn_ind->SRC_offset = sizeof(struct T_conn_ind);
	conn_ind->SRC_length = sizeof(struct pckaddr);
	addrp = (struct pckaddr *)((char *)conn_ind + conn_ind->SRC_offset);
	bcopy(pckdev.phy_naddr, addrp->phynaddr, NETADDRLEN);
	addrp->link = vcp->vc_srclink;
	bp->b_cont = mp;
	vcp->vc_creqmp = bp;
	vcp->vc_state = NEXTSTATE(TE_OK_ACK1, vcp->vc_state);
	pck_send_ok_ack(q, okmp, T_CONN_REQ);
	pck_sendconnect(vcp);
	return(TRUE);
}

STATIC int
pck_snd_cres(q, mp)
queue_t *q;
register mblk_t *mp;
{
	register struct T_conn_res *conn_res;
	register struct pm_accept *conn_con;
	register struct pck_vc *resvcp;
	register struct pck_vc *vcp;
	register mblk_t *tmp = NULL;
	struct pckaddr *addrp;
	struct pckseq *seq;
	mblk_t	*okmp, *bp;
	PACKHDR	*pktp;
	dl_unitdata_req_t *dlrp;

	vcp = (struct pck_vc *)q->q_ptr;
	if ((okmp = allocb(sizeof(struct T_ok_ack), BPRI_HI)) == NULL) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pck_snd_cres: can't alloc ok ack\n", 0, 0, 0);
		bufcall(sizeof(struct T_ok_ack), BPRI_HI, pckuwsrv, q);
		return(FALSE);
	}
	if ((mp->b_datap->db_lim - mp->b_datap->db_base) < MINBUFSZ) {
		if ((tmp = allocb(MINBUFSZ, BPRI_MED)) == NULL) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
				"pck_snd_cres: couldn't alloc buffer for conn_con\n", 0, 0, 0);
			freeb(okmp);
			bufcall(MINBUFSZ, BPRI_MED, pckuwsrv, q);
			return(FALSE);
		}
	}

	conn_res = (struct T_conn_res *)mp->b_rptr;
	vcp->vc_state = NEXTSTATE(prim_to_event[T_CONN_RES], vcp->vc_state);
	if (conn_res->QUEUE_ptr == vcp->vc_rdq)
		resvcp = vcp;
	else {
		for (resvcp = pck_vc; resvcp < &pck_vc[nvc]; resvcp++)
			if (resvcp->vc_rdq == conn_res->QUEUE_ptr)
				break;
		if (resvcp->vc_state != TS_IDLE) {
			vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
			pck_send_error_ack(q, mp, TBADF, 0);
			freeb(okmp);
			if (tmp)
				freeb(tmp);
			freeb(bp);
			return(TRUE);
		}
		resvcp->vc_state = NEXTSTATE(TE_PASS_CONN, resvcp->vc_state);
	}

	for (seq = vcp->vc_seq; seq < &vcp->vc_seq[MAX_CONN_IND]; seq++) {
		if (seq->seqno == conn_res->SEQ_number) {
			bcopy(seq->srcnaddr, resvcp->vc_dstnadr, NETADDRLEN);
			resvcp->vc_dstlink = seq->srclink;
			seq->srclink = -1;	/* mark unused */
			break;
		}
	}
	if (seq >= &vcp->vc_seq[MAX_CONN_IND]) {
		vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
		pck_send_error_ack(q, mp, TBADSEQ, 0);
		freeb(okmp);
		if (tmp)
			freeb(tmp);
		freeb(bp);
		return(TRUE);
	}

	resvcp->vc_datreq->b_rptr = resvcp->vc_datreq->b_datap->db_base;
	dlrp = (dl_unitdata_req_t *)resvcp->vc_datreq->b_rptr;
	dlrp->dl_primitive = DL_UNITDATA_REQ;
	dlrp->dl_dest_addr_offset = DL_UNITDATA_REQ_SIZE;
	dlrp->dl_dest_addr_length = NETADDRLEN;
	resvcp->vc_datreq->b_wptr = resvcp->vc_datreq->b_rptr + DL_UNITDATA_REQ_SIZE;
	bcopy(resvcp->vc_dstnadr, resvcp->vc_datreq->b_wptr, NETADDRLEN);
	resvcp->vc_datreq->b_wptr += NETADDRLEN;
	resvcp->vc_datreq->b_datap->db_type = M_PROTO;
	if ((bp = copyb(resvcp->vc_datreq)) == NULL) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pck_snd_cres: couldn't copyb buffer for DL_UNITDATA_REQ\n", 0, 0, 0);
		freeb(okmp);
		if (tmp)
			freeb(tmp);
		bufcall(DATREQSIZE, BPRI_MED, pckuwsrv, q);
		return(FALSE);
	}
	if (tmp) {
		if (mp->b_cont)
			linkb(tmp, unlinkb(mp));
		freeb(mp);
		mp = tmp;
	}
	mp->b_datap->db_type = M_DATA;
	mp->b_rptr = mp->b_datap->db_base + HDRWORD;
	mp->b_wptr = mp->b_rptr + MINPKSIZE - HDROFFSET;
	pktp = (PACKHDR *)mp->b_rptr;
	pktp->pk_type = CTRLP;
	pktp->pk_size = msgdsize(mp) + COMPATSIZE;
	conn_con = (struct pm_accept *)(mp->b_rptr + PCKHDRSZ);
	conn_con->cmd = PM_ACCEPT;
	mp->b_rptr -= HDROFFSET;

	/*
	 * Place address in appropriate spot for T_conn_con so
	 * that the receiver won't have to move it around.
	 */
	conn_con->RES_offset = sizeof(struct T_conn_con);
	conn_con->RES_length = sizeof(struct pckaddr);
	addrp = (struct pckaddr *)((char *)conn_con + conn_con->RES_offset);
	bcopy(pckdev.phy_naddr, addrp->phynaddr, NETADDRLEN);
	addrp->link = resvcp->vc_srclink;
	if ((vcp->vc_seqcnt == 1) && (resvcp == vcp)) 
		vcp->vc_state = NEXTSTATE(TE_OK_ACK2, vcp->vc_state);
	else if (vcp->vc_seqcnt == 1)
		vcp->vc_state = NEXTSTATE(TE_OK_ACK3, vcp->vc_state);
	else
		vcp->vc_state = NEXTSTATE(TE_OK_ACK4, vcp->vc_state);
	vcp->vc_seqcnt--;
	pck_send_ok_ack(q, okmp, T_CONN_RES);
	bp->b_cont = mp;
	pck_sendout(bp, resvcp, NODUP);
	return(TRUE);
}

STATIC int
pck_snd_discon(q, mp)
queue_t *q;
register mblk_t *mp;
{
	register struct T_discon_req *discon_req;
	register struct pck_vc *vcp;
	register mblk_t *tmp = NULL;
	mblk_t	*okmp, *bp;
	long	reason;
	unchar	req_state, nextstate;

	vcp = (struct pck_vc *)q->q_ptr;
	req_state = vcp->vc_state;

	if ((okmp = allocb(sizeof(struct T_ok_ack), BPRI_HI)) == NULL) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		  	"pck_snd_discon: can't alloc ok ack\n", 0, 0, 0);
		bufcall(sizeof(struct T_ok_ack), BPRI_HI, pckuwsrv, q);
		return(FALSE);
	}
	if ((mp->b_datap->db_lim - mp->b_datap->db_base) < MINBUFSZ) {
		if ((tmp = allocb(MINBUFSZ, BPRI_MED)) == NULL) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
				"pck_snd_discon: couldn't alloc buffer for discon_ind\n", 0, 0, 0);
			freeb(okmp);
			bufcall(MINBUFSZ, BPRI_MED, pckuwsrv, q);
			return(FALSE);
		}
	}
	if ((bp = copyb(vcp->vc_datreq)) == NULL) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pck_snd_discon: couldn't copyb buffer for DL_UNITDATA_REQ\n", 0, 0, 0);
		freeb(okmp);
		if (tmp)
			freeb(tmp);
		bufcall(DATREQSIZE, BPRI_MED, pckuwsrv, q);
		return(FALSE);
	}
	
	discon_req = (struct T_discon_req *)mp->b_rptr;
	vcp->vc_state = NEXTSTATE(prim_to_event[T_DISCON_REQ], vcp->vc_state);
	if (req_state == TS_WRES_CIND) {
		register struct pckseq *seq;

		for (seq = vcp->vc_seq; seq < &vcp->vc_seq[MAX_CONN_IND]; seq++) {
			if (seq->seqno == discon_req->SEQ_number) {
				bcopy(seq->srcnaddr, vcp->vc_dstnadr, NETADDRLEN);
				vcp->vc_dstlink = seq->srclink;
				seq->srclink = -1;	/* mark unused */
				break;
			}
		}
		if (seq >= &vcp->vc_seq[MAX_CONN_IND]) {
			vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
			pck_send_error_ack(q, mp, TBADSEQ, 0);
			freeb(okmp);
			if (tmp)
				freeb(tmp);
			freeb(bp);
			return(TRUE);
		}
		if (vcp->vc_seqcnt == 1)
			nextstate = NEXTSTATE(TE_OK_ACK2, vcp->vc_state);
		else
			nextstate = NEXTSTATE(TE_OK_ACK4, vcp->vc_state);
		vcp->vc_seqcnt--;
		reason = VC_REJECT;
	} else {
		nextstate = NEXTSTATE(TE_OK_ACK1, vcp->vc_state);
		reason = VC_USERINIT;
	}
	if (tmp) {
		if (mp->b_cont)
			linkb(tmp, unlinkb(mp));
		freeb(mp);
		mp = tmp;
	}
	if (req_state == TS_DATA_XFER) {
		if (!pck_send_flush(RD(q))) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		  		"pck_snd_discon: pck_send_flush failed\n", 0, 0, 0);
			vcp->vc_state = NEXTSTATE(TE_ERROR_ACK, vcp->vc_state);
			pck_send_error_ack(q, mp, TSYSERR, EPROTO);
			freeb(okmp);
			freeb(bp);
			return(TRUE);
		}
	}
	vcp->vc_state = nextstate;
	pck_send_ok_ack(q, okmp, T_DISCON_REQ);
	bp->b_cont = mp;
	pck_senddisconn(q, bp, reason);
	return(TRUE);
}

/*
 * Process STOPP, CTRLP, and ACKP message types.  Return the
 * message pointer if the message contains DATA to go upstream; else
 * return NULL.
 */
STATIC mblk_t *
pck_recvack(mp)
register mblk_t *mp;
{
	register struct	pck_vc *vcp;
	register PACKHDR *opktp;
	register mblk_t *mp1;
	register s;
	PACKHDR	pkt;

	pkt = *(PACKHDR *)mp->b_rptr;
	vcp = &pck_vc[pckminor[pkt.pk_dstlink]];
	PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		"pck_recvack: type %x mp %x size %x\n", pkt.pk_type, mp, pkt.pk_size);

	if (pkt.pk_type & STOPP) {
		/* Remote receiver has been flow controlled */
		PCKLOG(FLOW_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pck_recvack: STOPP packet received\n", 0, 0, 0);
		vcp->vc_flags |= VC_REMFC;
		s = splstr();
		if (vcp->vc_lnktimer != -1)
			/* Wait 'patiently' for receiver */
			untimeout(vcp->vc_lnktimer);

		vcp->vc_lnktimer = timeout(pck_lnktimeout, vcp, MONITORTIME);
		splx(s);
		vcp->vc_nretry = 0;
		freemsg(mp);
		return(NULL);
	}

	if (pkt.pk_type & ACKP) {
		/*
		 *	packet contains ACK
		 */
		if (vcp->vc_state != TS_DATA_XFER) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
				"pck_recvack: ACK (%x) ignored, link down, state %x\n",
				pkt.pk_ackno, vcp->vc_state, 0);
			freemsg(mp);
			return(NULL);
		}

		if (vcp->vc_flags & VC_REMFC) {
			/*
			 * An ACKP indicates remote receiver freed up.
			 * If the ACKP acks any previously unack'd packets
			 * then the writer will be enabled below.
			 */
			PCKLOG(FLOW_ID, VC_NUM(vcp), 0, SL_TRACE,
				"pck_recvack: remote receiver freed up: pkackno(%x) rackno(%x)\n",
				pkt.pk_ackno, vcp->vc_rackno, 0);
			vcp->vc_flags &= ~VC_REMFC;
		}

		if (vcp->vc_qcnt == 0) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
				"pck_recvack: ACK (%x) ignored because queue is empty\n",
				pkt.pk_ackno, 0, 0);
			goto ctrlpk;
		}

		/*
		 *	kill packet timer
		 */
		s = splstr();
		if (vcp->vc_lnktimer != -1) {
			untimeout(vcp->vc_lnktimer);
			vcp->vc_lnktimer = -1;
		}
		splx(s);
		vcp->vc_nretry = 0;

		/*
		 *	free send buffer for all packets <= ackno
		 */
		while (mp1 = (mblk_t *) pck_deque(&vcp->vc_retrnsq)) {
			opktp = (PACKHDR *)(mp1->b_cont->b_rptr + HDROFFSET);
			if (! SEQIN(vcp->vc_rackno, opktp->pk_seqno, pkt.pk_ackno)) {
				/* 
				 * have to put the entry back in the
				 * front of the retransmit queue
				 * and need to issue a timer again
				 * because packets left in the queue
				 */
				mp1->b_next = vcp->vc_retrnsq.rq_head;
				vcp->vc_retrnsq.rq_head = mp1;
				s = splstr();
				if (vcp->vc_lnktimer != -1)
					untimeout(vcp->vc_lnktimer);

				vcp->vc_lnktimer = timeout(pck_lnktimeout, vcp, PKTIME);
				splx(s);
				break;
			}

			freemsg(mp1);
			vcp->vc_qcnt--;
		}
		vcp->vc_rackno = pkt.pk_ackno;

		/*
		 *	send out items waiting in the output queue
		 */
		if ((int)vcp->vc_qcnt < CREDIT)
			qenable(WR(vcp->vc_rdq));
	}

ctrlpk:
	if (pkt.pk_type & CTRLP) {
		int     pktsize;
		
		/*
		 *	control packet
		 */
		pktsize = msgdsize(mp) + COMPATSIZE;
		mp->b_rptr += PCKHDRSZ;
		if ((int)pkt.pk_size < pktsize)
			/*
			 * Free the pad bytes of the msg.
			 * Because of earlier pullupmsg, guaranteed that
			 * msg is in one mblk - can't do adjmsg() since
			 * mp may point to dup'ed block if loopback.
			 */
			mp->b_rptr += (pktsize - pkt.pk_size);
			
		if (pck_ctrlpkt(vcp, mp) == TRUE)
			putnext(vcp->vc_rdq, mp);
		return(NULL);
	}

	if (pkt.pk_type & (DATAP+SANITYP))
		return(mp);

	if (pkt.pk_type & SNDACK) {
		/*
		 * This packet is marked SNDACK without DATA.
		 * This is only generated when the remote tranmitter
		 * times out on a flow controlled link.  Send back
		 * a STOPP packet to indicate we're still flow controlled.
		 */
		if (vcp->vc_flags & VC_LOCALFC)
			vcp->vc_flags |= VC_SENDSTOP;
		else
			vcp->vc_flags |= VC_SENDACK;
		if (pckdev.linkstate == LINKED)
			pck_sendack(vcp, 0);
	}

	freemsg(mp);
	return(NULL);
}

/*****

NAME		pck_recvpkt()

PURPOSE
	To handle input packets. 

DESCRIPTION
	RECVPKT(mp)

	if (length received is not the same as the packet length)
		ignore the packet and return;
	if (packet contains data) {
		if (packet sequence number is greater than expected)
			error, ignore the packet and return;
		if (packet sequence number is less than expected) {
			send ACK packet;
			ignore the packet and return;
		}
		increment expected packet sequence number;
		set timer to send ACK packet later;
		pass the packet to the layer above;
	}

*****/

STATIC void
pck_recvpkt(mp)
register mblk_t *mp;
{
	register struct	pck_vc *vcp;
	register int s;
	PACKHDR	pkt;
	int	pktsize;	/* packet data size */
	mblk_t	*amp;

	pkt = *(PACKHDR *)mp->b_rptr;
	vcp = &pck_vc[pckminor[pkt.pk_dstlink]];
	PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		"pck_recvpkt: type %x mp %x size %x\n", pkt.pk_type, mp, pkt.pk_size);
	pktsize = msgdsize(mp) + COMPATSIZE;
	mp->b_rptr += sizeof(PACKHDR);
	if ((int)pkt.pk_size < pktsize)
		/*
		 * Free the pad bytes of the msg.
		 * Because of earlier pullupmsg, guaranteed that
		 * msg is in one mblk - can't do adjmsg() since
		 * mp may point to dup'ed block if loopback.
		 */
		mp->b_rptr += (pktsize - pkt.pk_size);

	/*
	 *	ignore the packet if virtual circuit is down
	 */
	if (vcp->vc_state != TS_DATA_XFER) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
		    "pck_recvpkt: data packet ignored, link down, state %x\n",
		    vcp->vc_state, 0, 0);
		goto freepkt;
	}

	/*
	 *	ignore the packet if sequence number > expected
	 */
	if (pkt.pk_seqno > vcp->vc_epsn) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
		    "pck_recvpkt: seq # (%x) greater than expected (%x)\n",
		    pkt.pk_seqno, vcp->vc_epsn, 0);
		goto freepkt;
	}

	if (pkt.pk_seqno < vcp->vc_epsn) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
		    "pck_recvpkt: seq # (%x) less than expected (%x)\n",
		    pkt.pk_seqno, vcp->vc_epsn, 0);
		/*
		 *	ignore the packet if sequence number < expected
		 *	but send an ACK back
		 */
		vcp->vc_flags |= VC_SENDACK;
		if (pckdev.linkstate == LINKED)
			pck_sendack(vcp, 0);

		goto freepkt;
	}

	/*
	 * Allocate M_PROTO for T_DATA_IND or T_EXDATA_IND
	 */
	if (pkt.pk_type & DATAP) {
		register mblk_t *protomp;
		union T_primitives *prim;

		if ((protomp = allocb(sizeof(struct T_data_ind), BPRI_MED)) == NULL) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
				"pck_recvpkt: can't allocb data proto block\n", 0, 0, 0);
			goto freepkt;
		}
		protomp->b_datap->db_type = M_PROTO;
		prim = (union T_primitives *)protomp->b_rptr;
		prim->type = (pkt.pk_type & EXDATAP) ? T_EXDATA_IND : T_DATA_IND;
		prim->data_ind.MORE_flag = 0;
		protomp->b_wptr += sizeof(struct T_data_ind);
		linkb(protomp, mp);
		mp = protomp;
	}

	/*
	 *	increment expected sequence number
	 */
	vcp->vc_epsn++;
	vcp->vc_sackno = pkt.pk_seqno;
	vcp->vc_flags |= VC_SENDACK;

	if ((pkt.pk_type & SNDACK) || (pkt.pk_type & SANITYP)) {
		/*
		 *	send back ack immediately
		 */
		if (pckdev.linkstate == LINKED)
			pck_sendack(vcp, 0);
	}
	else {
		/*
		 *	set a timer for sending back ack 
		 *	in case there is no data traffic 
		 *	going to the other direction
		 */
		s = splstr();
		if (vcp->vc_acktimer == -1)
			vcp->vc_acktimer = timeout(pck_acktimeout, vcp, ACKTIME);
		splx(s);
	}

	if (pkt.pk_type & SANITYP)
		goto freepkt;		/* discard sanity packet */

	putnext(vcp->vc_rdq, mp);
	return;

	/*
	 *	free packet space
	 */
freepkt:
	freemsg(mp);
	return;
}

/*****

NAME		pck_sendpkt()

PURPOSE
	To send packet out to the link.

DESCRIPTION
	SENDPKT(mp, dstlink, type)

		mp is the pointer to the message to be sent out,
		dstlink is the destination link number.
		type is the packet type.

	if (qcnt is less than CREDIT) {
		put an entry in retransmit queue;
		send data packet;
	}
	else
		put an entry in output queue;

RETURN	TRUE - if message is handled successfully
	FALSE - fail to get buffer, may need to put the message 
		back in the front of queue

*****/

STATIC int
pck_sendpkt(mp, vcp, type)
register mblk_t *mp;		/* message pointer */
register struct pck_vc *vcp;	/* virtual circuit pointer */
register int	type;		/* packet type */
{
	register PACKHDR *pktp;
	register mblk_t *mp1;
	register int s;
	int	pktsize;
	mblk_t *bp;

	ASSERT(mp);
	PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE, "pck_sendpkt: sending pkt\n", 0, 0, 0);
	pktsize = msgdsize(mp);
	if (vcp->vc_state != TS_DATA_XFER || pktsize == 0) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
		    "pck_sendpkt: packet rejected, bad state %x or zero-length\n",
			vcp->vc_state, 0, 0);
		freemsg(mp);
		return(TRUE);
	}
	if ((int)vcp->vc_qcnt >= CREDIT)
		return(FALSE);

	/*
	 *	allocate a buffer for the packet header
	 */
	if ((mp1 = allocb(MINBUFSZ, BPRI_MED)) == NULL) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
			"pck_sendpkt: allocb fail\n", 0, 0, 0);
		return(NEEDBUF);
	}
	if ((bp = copyb(vcp->vc_datreq)) == NULL) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pck_sendpkt: couldn't copyb buffer for DL_UNITDATA_REQ\n", 0, 0, 0);
		freeb(mp1);
		return(NEEDBUF);
	}

	/*
	 *	fill in the packet header
	 */
	mp1->b_cont = mp;
	mp1->b_rptr += HDRWORD;
	mp1->b_wptr = mp1->b_rptr + PCKHDRSZ;
	pktp = (PACKHDR *)mp1->b_rptr;
	pktp->pk_type = type;
	pktp->pk_size = pktsize + PCKHDRSZ;
	mp1->b_rptr -= HDROFFSET;

	/*
	 * Pad out small data messages so they'll be accepted by
	 * media driver.
	 */
	if (pktp->pk_size < MINPKSIZE)
		mp1->b_wptr += (MINPKSIZE - pktp->pk_size);
	pktp->pk_size += COMPATSIZE;
	pktp->pk_seqno = vcp->vc_cpsn;
	vcp->vc_cpsn++;
	bp->b_cont = mp1;
	pck_enque(&vcp->vc_retrnsq, bp);
	if (++vcp->vc_qcnt == CREDIT) {
		/*
		 *	up to the window size limit,
		 *	force the other side to send back ack
		 */
		pktp->pk_type |= SNDACK;
	}

	s = splstr();
	if (vcp->vc_lnktimer == -1)
		vcp->vc_lnktimer = timeout(pck_lnktimeout, vcp, PKTIME);
	splx(s);
	pck_sendout(bp, vcp, DUP);
	return(TRUE);
}

STATIC void
pck_sendack(vcp, poke)
register struct pck_vc *vcp;
int poke;
{
	register mblk_t *ackbp, *bp;
	register dl_unitdata_req_t *reqp;
	PACKHDR *ackpktp;

	PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		"pck_sendack: vcp %x\n", vcp, 0, 0);
	if ((ackbp = allocb(MINBUFSZ, BPRI_MED)) == NULL) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pck_sendack: allocb failed\n", 0, 0, 0);
		return;
	}
	if ((bp = allocb(DL_UNITDATA_REQ_SIZE, BPRI_MED)) == NULL) {
		freeb(ackbp);
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pck_sendack: allocb failed\n", 0, 0, 0);
		return;
	}
	ackbp->b_rptr += HDRWORD;
	ackpktp = (PACKHDR *)ackbp->b_rptr;
	if (poke)
		ackpktp->pk_type = SNDACK;
	else
		ackpktp->pk_type = ACKP;
	ackpktp->pk_size = MINPKSIZE + COMPATSIZE - HDROFFSET;
	ackbp->b_rptr -= HDROFFSET;
	ackbp->b_wptr = ackbp->b_rptr + MINPKSIZE;
	bp->b_datap->db_type = M_PROTO;
	reqp = (dl_unitdata_req_t *)bp->b_rptr;
	reqp->dl_primitive = DL_UNITDATA_REQ;
	reqp->dl_dest_addr_offset = DL_UNITDATA_REQ_SIZE;
	reqp->dl_dest_addr_length = NETADDRLEN;
	bp->b_wptr += DL_UNITDATA_REQ_SIZE;
	bcopy(vcp->vc_dstnadr, bp->b_wptr, NETADDRLEN);
	bp->b_wptr += NETADDRLEN;
	bp->b_cont = ackbp;
	pck_sendout(bp, vcp, NODUP);
}

/*
 *	pck_sendout() -- send out packet
 */
STATIC void
pck_sendout(mp, vcp, dupflg)
register mblk_t *mp;
register struct pck_vc *vcp;
int dupflg;
{
	register PACKHDR *pktp;
	register mblk_t *nmp;
	register int	s;

	PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		"pck_sendout: mp %x vcp %x\n", mp, vcp, 0);
	if (!canput(pckdev.qbot->q_next)) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pck_sendout: driver wq full\n", 0, 0, 0);
		return;
	}

	/*
	 *	duplicate message before send to emd
	 */
	pktp = (PACKHDR *)(mp->b_cont->b_rptr + HDROFFSET);
	if (dupflg == NODUP)
		nmp = mp;
	else {
		if ((nmp = dupmsg(mp)) == NULL) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
				"pck_sendout: dupmsg fail\n", 0, 0, 0);
			return;		/* time out will force retry later */
		}
	}

	if (vcp->vc_flags & VC_SENDSTOP) {
		/*
		 * Send a STOPP packet to remote transmitter if needed.
		 * We clear the ACKP bit in order to provide backwards
		 * compatability to NPACK providers which don't recognize
		 * STOPP packets.  They will simply discard this message.
		 */
		vcp->vc_flags &= ~VC_SENDSTOP;
		pktp->pk_type &= ~ACKP;
		pktp->pk_type |= STOPP;
	} else if (vcp->vc_flags & VC_SENDACK) {
		/*
		 * Piggy back ACK if needed.
		 *
		 * We must not send back ACKP packets while local
		 * receiver is flow controlled.
		 */
		if (vcp->vc_flags & VC_LOCALFC) {
			pktp->pk_type &= ~ACKP;
			PCKLOG(FLOW_ID, VC_NUM(vcp), 0, SL_TRACE,
				"pck_sendout: ACKP not sent - LOCALFC set\n", 0, 0, 0);
		} else {
			pktp->pk_type |= ACKP;
			pktp->pk_ackno = vcp->vc_sackno;
		}
		vcp->vc_flags &= ~VC_SENDACK;
		s = splstr();
		if (vcp->vc_acktimer != -1) {
			untimeout(vcp->vc_acktimer);
			vcp->vc_acktimer = -1;
		}
		splx(s);
	}
	pktp->pk_srclink = vcp->vc_srclink;
	pktp->pk_dstlink = vcp->vc_dstlink;
	putnext(pckdev.qbot, nmp);
}

/*****
NAME		pck_retrns()

	To handle packet retransmission when time-out.

*****/

STATIC void
pck_retrns(vcp)
register struct pck_vc *vcp;		/* link */
{
	register PACKHDR *pktp;
	register mblk_t *mp1, *mp2;
	register int s;
	mblk_t	*amp;

	PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
		"pck_retrns: packet time out for link\n", 0, 0, 0);

	/*
	 *	ignore the time-out if link is down
	 */
	if (vcp->vc_state != TS_DATA_XFER) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
		   "pck_retrns: time-out ignored, link down, state %x\n",
		   vcp->vc_state, 0, 0);
		return;
	}

	vcp->vc_nretry++;

	/*
	 *	link is down if nretry > NRETRNS
	 */
	if ((int)vcp->vc_nretry > NRETRNS) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
			"pck_retrns: link is down\n", 0, 0, 0);
#ifdef DEBUG
		if (dudebug & NO_RECOVER)
			return;
#endif

		/*
		 *	discard packet in the queue
		 */
		while (mp1 = (mblk_t *) pck_deque(&vcp->vc_retrnsq))
			freemsg(mp1);
		vcp->vc_qcnt = 0;


		/*
		 * send a DISCONNECT message to the upstream queue
		 */
		if (pck_send_flush(vcp->vc_rdq))
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
		    		"pck_retrns: pck_send_flush failed\n", 0, 0, 0);
		vcp->vc_reason = VC_LINKDOWN;
		pck_updisconn(vcp);
		return;
	}

	if (vcp->vc_flags & VC_REMFC) {
		/*
		 * Remote receiver may be flow controlled - 
		 * poke it to make sure.
		 */
		PCKLOG(FLOW_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pck_retrns: timeout on flow controlled link\n", 0, 0, 0);
		s = splstr();
		if (vcp->vc_lnktimer != -1)
			untimeout(vcp->vc_lnktimer);

		vcp->vc_lnktimer = timeout(pck_lnktimeout, vcp, MONITORTIME);
		splx(s);
		pck_sendack(vcp, 1);
		return;
	}

	/*
	 *	retransmit all packets in the retransmit queue
	 */
	s = splstr();
	if (vcp->vc_lnktimer != -1)
		untimeout(vcp->vc_lnktimer);

	vcp->vc_lnktimer = timeout(pck_lnktimeout, vcp, PKTIME);
	for (mp1 = vcp->vc_retrnsq.rq_head, mp2 = mp1->b_next;
	     mp1 != NULL; mp1 = mp2, mp2 = mp2->b_next) {
		pktp = (PACKHDR *)(mp1->b_cont->b_rptr + HDROFFSET);
		vcp->vc_flags |= VC_SENDACK;
		pck_sendout(mp1, vcp, DUP);
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pck_retrns: packet (%x) retransmit\n", pktp->pk_seqno, 0, 0);
	}
	splx(s);
}

/*****
	pck_lnktimeout

	called from clock routine when a link times out.
	Just mark the link that timed out and 
	enable the appropriate stream queue.
*****/

STATIC void
pck_lnktimeout (vcp)
register struct pck_vc *vcp;
{
	PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE, "pck_lnktimeout: link timed out\n", 0, 0, 0);
	vcp->vc_lnktimer = -1;

#ifdef DEBUG
	if (dudebug & NO_RETRANS)
		return;
#endif

	if (vcp->vc_state != TS_DATA_XFER)
		return;
	vcp->vc_flags |= VC_LNKTOUT;
	qenable(WR(vcp->vc_rdq));
}

/*****
	pck_acktimeout

	called from clock routine when ack needs to be sent back.
	Just mark the link that timed out and 
	enable the appropriate stream queue.
*****/

STATIC void
pck_acktimeout (vcp)
register struct pck_vc *vcp;
{
	PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE, "pck_acktimeout: ack timed out\n", 0, 0, 0);
	vcp->vc_acktimer = -1;
	if (vcp->vc_state != TS_DATA_XFER)
		return;
	vcp->vc_flags |= VC_ACKTOUT;
	qenable(WR(vcp->vc_rdq));
}

/*****
	packmonitor -- to send sanity packets periodically to check 
		   if links are still up.

	This routine is scheduled every minute and try to send
	a sanity packet to every link that is up and has no
	packet in the retransmit queue.  i.e., the sanity packet
	is sent only if there is no traffic on the link.
*****/

STATIC void
packmonitor()
{
	register struct pck_vc *vcp;

	PCKLOG(PACK_ID, -1, 0, SL_TRACE, "packmonitor: active\n", 0, 0, 0);
	if (pckdev.linkstate != LINKED)
		return;
#ifdef DEBUG
	if (dudebug & NO_MONITOR)
		goto skip;
#endif

	for (vcp = pck_vc; vcp < &pck_vc[nvc]; vcp++) {
		if (vcp->vc_state != TS_DATA_XFER)
			continue;
		if (vcp->vc_qcnt > 0)
			continue;
		if (vcp->vc_flags & VC_REMFC)
			continue;

		/*
		 *	mark the link that needs to send sanity packet
		 *	and enable the appropriate stream queue
		 */
		vcp->vc_flags |= VC_SANITY;
		qenable(WR(vcp->vc_rdq));
	}

	/*
	 *	schedule itself again
	 */
skip:
	timeout(packmonitor, 0, MONITORTIME);
}

/*
 * pck_sendconnect() -- send out connect request for virtual circuit set up
 */
STATIC void
pck_sendconnect(vcp)
register struct pck_vc *vcp;		/* link ptr */
{
	register s;
	register mblk_t *mp;

	PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		"pck_sendconnect: sending connect request\n", 0, 0, 0);
	s = splstr();
	if (vcp->vc_state == TS_DATA_XFER || pckdev.linkstate != LINKED) {
		if (vcp->vc_creqmp) freemsg(vcp->vc_creqmp);
		vcp->vc_creqmp = NULL;
		splx(s);
		return;
	}
	if (vcp->vc_nretry-- == 0) {
		/*
		 *	exceed connection retries
		 *	return fail to upper stream
		 */
		freemsg(vcp->vc_creqmp);
		vcp->vc_creqmp = NULL;
		splx(s);
		vcp->vc_reason = VC_CONNFAIL;
		pck_updisconn(vcp);
		return;
	}

	/*
	 *  send (or retry) connect request - set timer for retry
	 */
	vcp->vc_lnktimer = timeout(pck_sendconnect, vcp, vcp->vc_interval);
	if ((mp = dupmsg(vcp->vc_creqmp)) == NULL) {
		splx(s);
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
			"pck_sendconnect: couldn't dup msg for conn_req\n", 0, 0, 0);
		return;
	}
	splx(s);
	pck_sendout(mp, vcp, NODUP);
}

/*
 * pck_senddisconn(q, mp, reason)
 *
 *	send DISCONNECT indication to remote user
*/
STATIC void
pck_senddisconn(q, mp, reason)
queue_t	*q;
register mblk_t *mp;
long	reason;
{
	register struct pm_discon *discon_ind;
	register struct pck_vc *vcp;
	struct pckaddr *addrp;
	PACKHDR	*pktp;
	mblk_t *bp;

	vcp = (struct pck_vc *)q->q_ptr;
	if (pckdev.linkstate != LINKED) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			"pck_senddisconn: mux not linked\n", 0, 0, 0);
		freemsg(mp);
		return;
	}
	bp = mp->b_cont;
	bp->b_rptr = bp->b_datap->db_base + HDRWORD;
	bp->b_wptr = bp->b_rptr + MINPKSIZE - HDROFFSET;
	bp->b_datap->db_type = M_DATA;
	pktp = (PACKHDR *)bp->b_rptr;
	pktp->pk_type = CTRLP;
	pktp->pk_size = msgdsize(bp) + COMPATSIZE;
	discon_ind = (struct pm_discon *)(bp->b_rptr + PCKHDRSZ);
	discon_ind->cmd = PM_DISCONNECT;
	discon_ind->SRC_offset = sizeof(struct pm_discon);
	discon_ind->SRC_length = sizeof(struct pckaddr);
	addrp = (struct pckaddr *)((char *)discon_ind + discon_ind->SRC_offset);
	bcopy(pckdev.phy_naddr, addrp->phynaddr, NETADDRLEN);
	addrp->link = vcp->vc_srclink;
	bp->b_rptr -= HDROFFSET;
	/*
	 * The reason code is in the appropriate spot for T_discon_ind
	 * that the receiver won't have to move it around.
	 */
	discon_ind->reason = reason;
	pck_sendout(mp, vcp, NODUP);
}

/****

	receive control message for virtual circuit set up.

	CTRLPKT(i, pktp, mp)

		i is the link number;
		pktp is the packet pointer;
		mp is message pointer;

****/

STATIC int
pck_ctrlpkt(vcp, mp)
register struct pck_vc *vcp;	/* link pointer */
mblk_t *mp;			/* message pointer */
{
	register union pm_ctrl *ctrlp;

	if (pck_adjctrl(&mp, MINPKSIZE - PCKHDRSZ - HDROFFSET) == 0) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
			"pck_ctrlpkt: can't adjust incoming pkt\n", 0, 0, 0);
		freemsg(mp);
		return(FALSE);
	}
	ctrlp = (union pm_ctrl *)mp->b_rptr;
	if (ctrlp->cmd >= NCTRL) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
			"pck_ctrlpkt: invalid cmd %x\n", ctrlp->cmd, 0, 0);
		freemsg(mp);
		return (FALSE);
	}
	PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE, "pck_ctrlpkt: cmd %x\n", ctrlp->cmd, 0, 0);
	return((*rcv_ctrl[ctrlp->cmd])(vcp, mp));
}

/*
 * receive CONNECT indication from remote side
 */
STATIC int
pck_rcv_connect(vcp, mp)
register struct pck_vc *vcp;
register mblk_t *mp;
{
	register union pm_ctrl *ctrlp;
	register struct T_conn_ind *conn_ind;
	register struct pckseq	*seq;
	struct pckaddr	*addrp;
	int	s;

	ctrlp = (union pm_ctrl *)mp->b_rptr;
	s = splstr();
	if ((NEXTSTATE(TE_CONN_IND, vcp->vc_state) == BADSTATE) ||
	    ((int)vcp->vc_seqcnt >= vcp->vc_qlen)) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
		    "pck_rcv_connect: recv'd PM_CONNECT in state %x\n", vcp->vc_state, 0, 0);
		freemsg(mp);
		splx(s);
		return(FALSE);
	}
	addrp = (struct pckaddr *)((char *)ctrlp + ctrlp->connect.SRC_offset);
	for (seq = vcp->vc_seq; seq < &vcp->vc_seq[vcp->vc_qlen]; seq++) {
		if (seq->srclink == -1) {
			seq->seqno = ++seq_counter;
			bcopy(addrp->phynaddr, seq->srcnaddr, NETADDRLEN);
			seq->srclink = addrp->link;
			break;
		}
	}
	ASSERT(seq < &vcp->vc_seq[vcp->vc_qlen]);
	vcp->vc_seqcnt++;
	vcp->vc_state = NEXTSTATE(TE_CONN_IND, vcp->vc_state);
	splx(s);
	conn_ind = (struct T_conn_ind *)ctrlp;
	conn_ind->PRIM_type = T_CONN_IND;
	conn_ind->OPT_length = 0;
	conn_ind->OPT_offset = 0;
	conn_ind->SEQ_number = seq->seqno;
	mp->b_datap->db_type = M_PROTO;
	mp->b_wptr = mp->b_rptr +
		sizeof(struct T_conn_ind) + sizeof(struct pckaddr);
	return(TRUE);
}

/*
 * receive connect confirmation from remote side 
 * for the virtual circuit set up
 */
STATIC int
pck_rcv_accept(vcp, mp)
register struct pck_vc *vcp;
register mblk_t *mp;
{
	register union pm_ctrl *ctrlp;
	register struct T_conn_con *conn_con;
	register s;
	struct pckaddr	*addrp;

	s = splstr();
	if (NEXTSTATE(TE_CONN_CON, vcp->vc_state) == BADSTATE) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
		    "pck_rcv_accept: recv'd PM_ACCEPT in state %x\n", vcp->vc_state, 0, 0);
		freemsg(mp);
		splx(s);
		return(FALSE);
	}
	vcp->vc_state = NEXTSTATE(TE_CONN_CON, vcp->vc_state);
	if (vcp->vc_lnktimer != -1) {
		untimeout(vcp->vc_lnktimer);
		vcp->vc_lnktimer = -1;
	}
	splx(s);
	ctrlp = (union pm_ctrl *)mp->b_rptr;
	addrp = (struct pckaddr *)((char *)ctrlp + ctrlp->accept.RES_offset);
	vcp->vc_dstlink = addrp->link;
	freemsg(vcp->vc_creqmp);
	vcp->vc_creqmp = NULL;
	conn_con = (struct T_conn_con *)ctrlp;
	conn_con->PRIM_type = T_CONN_CON;
	conn_con->OPT_length = 0;
	conn_con->OPT_offset = 0;
	mp->b_datap->db_type = M_PROTO;
	mp->b_wptr = mp->b_rptr +
		sizeof(struct T_conn_con) + sizeof(struct pckaddr);
	return(TRUE);
}

/*
 * Process incoming disconnect indication
 */
STATIC int
pck_rcv_discon(vcp, mp)
register struct pck_vc *vcp;
register mblk_t *mp;
{
	register union pm_ctrl *ctrlp;
	register struct T_discon_ind *discon_ind;
	register struct pckseq	*seq;
	struct pckaddr	*addrp;
	int	s;
	unchar	nextstate;

	ctrlp = (union pm_ctrl *)mp->b_rptr;
	discon_ind = (struct T_discon_ind *)ctrlp;
	discon_ind->PRIM_type = T_DISCON_IND;
	s = splstr();
	if (vcp->vc_state == TS_WRES_CIND) {
		addrp = (struct pckaddr *)((char *)ctrlp +
			ctrlp->discon.SRC_offset);
		for (seq = vcp->vc_seq; seq < &vcp->vc_seq[MAX_CONN_IND]; seq++) {
			if ((seq->srclink == addrp->link) &&
			    (!pck_addrcmp(seq->srcnaddr, addrp->phynaddr, NETADDRLEN))) {
				discon_ind->SEQ_number = seq->seqno;
				seq->srclink = -1;
				break;
			}
		}
		if (seq >= &vcp->vc_seq[vcp->vc_qlen]) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
				"pck_rcv_discon: PM_DISCONNECT from unknown src\n", 0, 0, 0);
			freemsg(mp);
			splx(s);
			return(FALSE);
		}
		if (vcp->vc_seqcnt > 1)
			nextstate = NEXTSTATE(TE_DISCON_IND3, vcp->vc_state);
		else
			nextstate = NEXTSTATE(TE_DISCON_IND2, vcp->vc_state);
		vcp->vc_seqcnt--;
	} else {
		if (NEXTSTATE(TE_DISCON_IND1, vcp->vc_state) == BADSTATE) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
			   "pck_rcv_discon: recv'd PM_DISCONNECT in state %x\n",
			   vcp->vc_state, 0, 0);
			freemsg(mp);
			splx(s);
			return(FALSE);
		}
		discon_ind->SEQ_number = -1;
		nextstate = NEXTSTATE(TE_DISCON_IND1, vcp->vc_state);
	}
	mp->b_datap->db_type = M_PROTO;
	mp->b_wptr = mp->b_rptr + sizeof(struct T_discon_ind);
	if (vcp->vc_state == TS_DATA_XFER)
		if (!pck_send_flush(vcp->vc_rdq)) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		    		"pck_rcv_discon: pck_send_flush failed\n", 0, 0, 0);
			pck_send_fatal(vcp->vc_rdq, mp);
			splx(s);
			return(FALSE);
		}
	vcp->vc_state = nextstate;
	vcp->vc_flags = 0;
	splx(s);
	return(TRUE);
}

/*
 * pck_adjctrl(mp, ctrlsize)
 *
 *	Align message into a control and a data part if data exists
 */
STATIC int
pck_adjctrl(mp, ctrlsize)
register mblk_t	**mp;
int	ctrlsize;
{
	register mblk_t *cmp;
	int mblksize;

	mblksize = (*mp)->b_wptr - (*mp)->b_rptr;
	ASSERT(mblksize >= ctrlsize);
	if (mblksize == ctrlsize)
		return(1);

	if ((cmp = allocb(ctrlsize, BPRI_MED)) == NULL)
		return(0);
	cmp->b_datap->db_type = M_PROTO;
	bcopy((char *)(*mp)->b_rptr, (char *)cmp->b_wptr, ctrlsize);
	(*mp)->b_rptr += ctrlsize;
	cmp->b_wptr += ctrlsize;
	linkb(cmp, (*mp));
	(*mp) = cmp;
	return(1);
}

STATIC void
pck_term(q, qbot)
queue_t *q;
queue_t *qbot;
{
	register struct pck_vc *vcp;
	dl_unbind_req_t *ubp;
	mblk_t *mp;

	for (vcp = pck_vc; vcp < &pck_vc[nvc]; vcp++)
		if (vcp->vc_rdq && vcp != (struct pck_vc *)q->q_ptr) {
			vcp->vc_flags |= VC_FATAL;
			putctl1(vcp->vc_rdq->q_next, M_ERROR, EIO);
		}

	if (!canput(qbot->q_next)) {
		cmn_err(CE_CONT, "pck_term: driver queue full\n");
		return;
	}

	/*
	 * build a message block to unbind a protocol type
	 */
	if ((mp = allocb(DL_UNBIND_REQ_SIZE, BPRI_MED)) == NULL) {
		cmn_err(CE_CONT, "pck_term: can't alloc DL_BIND_REQ\n");
		return;
	}

	mp->b_datap->db_type = M_PROTO;
	mp->b_wptr += DL_UNBIND_REQ_SIZE;
	ubp = (dl_unbind_req_t *)mp->b_rptr;
	ubp->dl_primitive = DL_UNBIND_REQ;

	/*
	 * now send the DL_UNBIND_REQ to the driver
	 */
	PCKLOG(PACK_ID, -1, 0, SL_TRACE, "pck_term: send DL_UNBIND_REQ to driver\n", 0, 0, 0);
	putnext(qbot, mp);
}

STATIC void
pck_finished(q, reason)
register queue_t *q;
long reason;
{
	register struct pck_vc *vcp = (struct pck_vc *)q->q_ptr;
	register mblk_t *mp, *bp;
	struct pckseq *seq;
	queue_t *rdq, *wrq;

	if (q->q_flag & QREADR) {
		rdq = q;
		wrq = WR(q);
	} else {
		rdq = RD(q);
		wrq = q;
	}
	flushq(rdq, FLUSHALL);
	flushq(wrq, FLUSHALL);
	if (vcp->vc_seqcnt) {
		for (seq = vcp->vc_seq; seq < &vcp->vc_seq[MAX_CONN_IND]; seq++) {
			if (seq->srclink != -1) {
				bcopy(seq->srcnaddr, vcp->vc_dstnadr, NETADDRLEN);
				vcp->vc_dstlink = seq->srclink;
				seq->srclink = -1;
				if ((mp = allocb(MINBUFSZ, BPRI_MED)) == NULL) {
					printf("pck_finished: couldn't alloc buffer to send discon\n");
					goto noalloc;
				}
				if ((bp = copyb(vcp->vc_datreq)) == NULL) {
					PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
					    "pck_finished: couldn't copyb buffer for DL_UNITDATA_REQ\n", 0, 0, 0);
					freeb(mp);
					goto noalloc;
				}
				bp->b_cont = mp;
				pck_senddisconn(q, bp, reason);
			}
		}
	} else if ((vcp->vc_state == TS_DATA_XFER) || (vcp->vc_state == TS_WCON_CREQ)) {
		if ((mp = allocb(MINBUFSZ, BPRI_MED)) == NULL) {
			printf("pck_finished: couldn't alloc buffer to send discon\n");
			goto noalloc;
		}
		if ((bp = copyb(vcp->vc_datreq)) == NULL) {
			PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
			    "pck_finished: couldn't copyb buffer for DL_UNITDATA_REQ\n", 0, 0, 0);
			freeb(mp);
			goto noalloc;
		}
		bp->b_cont = mp;
		pck_senddisconn(q, bp, reason);
	}
noalloc:
	vcp->vc_state = TS_UNBND;
}

/*
 *  Place messages on write queue in the appropriate order
 *  based on the TLI precedence rules.
 */
STATIC void
pck_putwq_inorder(q, mp)
register queue_t *q;
register mblk_t	 *mp;
{
	long primtype;

	primtype = *(long *)mp->b_rptr;
	switch(primtype) {

	default:
		putq(q, mp);
		return;

	case T_EXDATA_REQ: {
		/*
		 *   Insert expedited message before regular data messages.
		 *   Can only have data and exdata msgs on queue here.
		 */
		register mblk_t *mp1;

		for (mp1 = q->q_first; mp1 != NULL; mp1 = mp1->b_next) {
	 		primtype = *(long *)mp1->b_rptr;
			if (primtype != T_EXDATA_REQ)
				break;
		}
		insq(q, mp1, mp);
		return;
		}

	case T_DISCON_REQ:
		/*
		 *  Must check for T_CONN_REQ on queue, and remove this
		 *  msg if you find it.
		 */
		if (q->q_last == NULL)
			putq(q, mp);
		else {
			primtype = *(long *)q->q_last->b_rptr;
			if (primtype == T_CONN_REQ) {
				flushq(q, FLUSHDATA);
				freemsg(mp);
			} else {
				flushq(q, FLUSHDATA);
				putq(q, mp);
			}
		}
		return;
	}
}

STATIC void
pck_send_fatal(q, mp)
register queue_t *q;
register mblk_t *mp;
{
	register struct pck_vc *vcp = (struct pck_vc *)q->q_ptr;

	pck_finished(q, VC_PACKFAIL);
	vcp->vc_flags |= VC_FATAL;
	mp->b_datap->db_type = M_ERROR;
	*mp->b_datap->db_base = EPROTO;
	mp->b_rptr = mp->b_datap->db_base;
	mp->b_wptr = mp->b_datap->db_base + sizeof(char);
	freemsg(unlinkb(mp));
	if (q->q_flag&QREADR)
		putnext(q, mp);
	else
		qreply(q, mp);
}

STATIC void
pck_send_error_ack(q, mp, tli_error, unix_error)
queue_t *q;
register mblk_t *mp;
long tli_error;
long unix_error;
{
	mblk_t *tmp;
	long type;
	register union T_primitives *prim;

	prim = (union T_primitives *)mp->b_rptr;
	type = prim->type;
	/*
	 * is message large enough to send
	 * up a T_ERROR_ACK primitive
	 */
	 if ((mp->b_datap->db_lim - mp->b_datap->db_base) < sizeof(struct T_error_ack)) {
		if ((tmp=allocb(sizeof(struct T_error_ack), BPRI_HI)) == NULL) {
			PCKLOG(PACK_ID, VC_NUM(q->q_ptr),0, SL_TRACE,
		       		"pck_send_error_ack: couldn't allocate buffer for error_ack\n", 0, 0, 0);
			pck_send_fatal(q, mp);
			return;
		}
		freemsg(mp);
		mp = tmp;
	 }
	 mp->b_rptr = mp->b_datap->db_base;
	 mp->b_wptr = mp->b_rptr + sizeof(struct T_error_ack);
	 prim = (union T_primitives *)mp->b_rptr;
	 prim->error_ack.ERROR_prim = type;
	 prim->error_ack.TLI_error = tli_error;
	 prim->error_ack.UNIX_error = unix_error;
	 prim->error_ack.PRIM_type = T_ERROR_ACK;
	 mp->b_datap->db_type = M_PCPROTO;
	 freemsg(unlinkb(mp));
	 if (q->q_flag&QREADR)
		putnext(q, mp);
	 else
		qreply(q, mp);
	 return;
}

STATIC void
pck_send_ok_ack(q, mp, type)
queue_t *q;
register mblk_t *mp;
long type;
{
	mblk_t *tmp;
	register struct T_ok_ack *ok_ack;

	/*
	 * is message large enough to send
	 * up a T_OK_ACK primitive
	 */
	 if ((mp->b_datap->db_lim - mp->b_datap->db_base) < sizeof(struct T_ok_ack)) {
		if ((tmp = allocb(sizeof(struct T_ok_ack), BPRI_HI)) == NULL) {
			PCKLOG(PACK_ID, VC_NUM(q->q_ptr), 0, SL_TRACE,
		    	   "pck_send_ok_ack: couldn't alloc buffer for ok_ack\n", 0, 0, 0);
			pck_send_fatal(q, mp);
			return;
		}
		freemsg(mp);
		mp = tmp;
	 }
	 mp->b_rptr = mp->b_datap->db_base;
	 mp->b_wptr = mp->b_rptr + sizeof(struct T_ok_ack);
	 ok_ack = (struct T_ok_ack *)mp->b_rptr;
	 ok_ack->CORRECT_prim = type;
	 ok_ack->PRIM_type = T_OK_ACK;
	 mp->b_datap->db_type = M_PCPROTO;
	 freemsg(unlinkb(mp));
	 if (q->q_flag&QREADR)
		putnext(q, mp);
	 else
		qreply(q, mp);
	 return;
}

STATIC int
pck_send_flush(q)
queue_t *q;
{
	mblk_t *mp;

	if ((mp = allocb(1, BPRI_HI)) == NULL)
		return(0);
	mp->b_wptr++;
	mp->b_datap->db_type = M_FLUSH;
	*mp->b_rptr = FLUSHRW;
	putnext(q, mp);
	return(1);
}

/*
 * pck_updisconn(vcp)
 *
 *	send DISCONNECT indication upstream to local user
 */
STATIC void
pck_updisconn(vcp)
register struct pck_vc *vcp;
{
	register mblk_t *mp;
	register struct T_discon_ind *discon_ind;

	PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE,
		"pck_updisconn: send DISCONNECT indication upstream\n", 0, 0, 0);
	vcp->vc_state = TS_IDLE;
	if (vcp->vc_rdq == NULL)
		return;

	if ((mp = allocb(sizeof(struct T_discon_ind), BPRI_MED)) == NULL) {
		PCKLOG(PACK_ID, VC_NUM(vcp), 0, SL_TRACE|SL_ERROR,
			"pck_updisconn: allocb fail\n", 0, 0, 0);
		bufcall(sizeof(struct T_discon_ind), BPRI_MED, pck_updisconn, vcp);
		return;
	}

	mp->b_datap->db_type = M_PROTO;
	discon_ind = (struct T_discon_ind *)mp->b_wptr;
	discon_ind->PRIM_type = T_DISCON_IND;
	discon_ind->DISCON_reason = vcp->vc_reason;
	mp->b_wptr += sizeof(struct T_discon_ind);
	putnext(vcp->vc_rdq, mp);
	return;
}

STATIC void
pck_enque(rq, mp)
register struct retransq *rq;
register mblk_t	*mp;
{
	if (rq->rq_head == NULL) {

		/*
		 *	queue is empty
		 */
		rq->rq_head = mp;
		rq->rq_tail = mp;
	} else {
		rq->rq_tail->b_next = mp;
		rq->rq_tail = mp;
	}
	mp->b_next = NULL;
}

STATIC mblk_t *
pck_deque(rq)
register struct retransq *rq;
{
	register mblk_t	*mp;

	if ((mp = rq->rq_head) != NULL) {
		rq->rq_head = rq->rq_head->b_next;
		mp->b_next = NULL;
	}
	return(mp);
}

STATIC long
pck_findlink()
{
	long	link;

	for (link = 0; link < nvc; link++)
		if (pckminor[link] == -1)
			return (link);
	return (-1);
}

/*
 * Return 0 if addresses match and 1 otherwise.
 */
STATIC int
pck_addrcmp(a1, a2, len)
register unchar *a1, *a2;
register uint len;
{
	while (len-- > 0) {
		if (*a1++ != *a2++)
			return(1);
	}
	return(0);
}
