/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)emd:io/emd.c	1.9"

typedef long CAPP;
typedef long RAPP;
#define RQSIZE 8
#define CQSIZE 8
#define NUM_QUEUES 3

#include "sys/cmn_err.h"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/strlog.h"
#include "sys/log.h"
#include "sys/queue.h"
#include "sys/cio_defs.h"
#include "sys/devcode.h"
#include "sys/firmware.h"
#include "sys/sbd.h"
#include "sys/debug.h"
#include "sys/inline.h"
#include "sys/systm.h"
#include "sys/cred.h"
#include "sys/dlpi.h"
#include "sys/ddi.h"
#include "sys/emduser.h"
#include "sys/emd.h"

/*
 * Streams interface information.
 */
STATIC int emdopen(), emdclose(), emdwput(), emdwsrv();

STATIC struct module_info emdm_info = {
	EMD_MID, EMD_NAME, 1, EMDMAXPSZ-EHEADSIZE, EMD_HIWAT, EMD_LOWAT
};

STATIC struct qinit emdrinit = {
	NULL, NULL, emdopen, emdclose, NULL, &emdm_info, NULL
};

STATIC struct qinit emdwinit = {
	emdwput, emdwsrv, NULL, NULL, NULL, &emdm_info, NULL
};

struct streamtab emdinfo = {
	&emdrinit, &emdwinit, NULL, NULL
};

/*
 * These are allocated from the master file.
 */
extern struct emd emd_emd[];		/* private per-stream data */
extern emd_maxdev;			/* max number of minors per board */
extern emd_nbds;			/* number of boards */
extern paddr_t emd_addr[];		/* physical address of boards */
extern struct emd_board emd_bd[];	/* EMD board information */

/*
 * Global EMD information.
 */
STATIC struct eig_elem {
	struct eigsend gblk[MSGLENMAX];		/* structure for multiple */
} eigarray[RQSIZE];				/* streams message blocks */
STATIC struct eig_elem *eignext;		/* the next message array */
STATIC int emd_majinit = 0;	/* set when major info has been initialized */
int emddevflag = 0;		/* we support a new-style (SVR4) interface */

#ifdef DEBUG
int emdlog = 0;
#endif

/*
 * Routines defined in this file.
 */
void emdinit();
STATIC void emd_binit();
STATIC int emdwproc();
int emdint();
STATIC void emdgetrb();
STATIC void emdcproc();
STATIC void emdrproc();
STATIC void emdterm();
STATIC void emdtimeout();
STATIC short emdfw_init();
STATIC short emdfw_attention();
STATIC short emdfw_job();
STATIC void emdunitind();
STATIC void emdnonfatal();
STATIC void emdfatal();
STATIC void emd_domajors();
STATIC int getboard();

#ifdef UNUSED
STATIC void emd_time();
#endif

/*
 * Initialize the EMD data structures.  Called from
 * main() at boot time.
 */
void
emdinit()
{
	register int board;
	register int index;
	register int offset;

	eignext = eigarray;
	for (board = 0; board < emd_nbds; board++) {
		for (index = 0; index < emd_maxdev; index++) {
			offset = (board * emd_maxdev) + index;
			emd_emd[offset].emd_rdq = NULL;
			emd_emd[offset].emd_head = NULL;
			emd_emd[offset].emd_tail = NULL;
			emd_emd[offset].emd_state = 0;
			emd_emd[offset].emd_seq = 0;
			emd_emd[offset].emd_bid = board;
			emd_emd[offset].emd_sap = 0;
		}
		emd_binit(board);
	}
}

/*
 * Initialize a board.
 */
STATIC void
emd_binit(bid)
	register int bid;
{
	register int index;

	EMD_RESET(bid);		/* reset the EMD hardware */
	emd_bd[bid].eb_emdp = &emd_emd[bid * emd_maxdev];

	/*
	 * Initialize the receive buffer array.
	 */
	for (index = 0; index < RQSIZE; index++)
		emd_bd[bid].eb_srbuf[index] = NULL;
	for (index = 0; index < RQSIZE; index++)
		emd_bd[bid].eb_lrbuf[index] = NULL;
	for (index = 0; index < RQSIZE+RQSIZE; index++)
		emd_bd[bid].eb_ind[index] = NULL;
	emd_bd[bid].eb_state = EB_DOWN;
	if (emd_bd[bid].eb_timeid)	/* reset the firmware timer */
		untimeout(emd_bd[bid].eb_timeid);
	emd_bd[bid].eb_timeid = 0;
	return;
}

/* 
 * EMD open procedure.  Initialize the data structures.
 */
STATIC int
emdopen(q, devp, flag, sflag, credp)
	queue_t *q;
	dev_t *devp;
	int flag;
	int sflag;
	cred_t *credp;
{
	register int bid;
	register struct emd *ep;
	dev_t dev;

	if (!emd_majinit)	/* set up board-to-major mapping */
		emd_domajors(*devp);
	if (sflag == MODOPEN)
		return(EINVAL);
	if ((bid = getboard(*devp)) < 0)
		return(EINVAL);
	if (q->q_ptr)		/* already open */
		return(0);
	if (sflag == CLONEOPEN) {
		ep = emd_bd[bid].eb_emdp;
		for (dev = 0; dev < emd_maxdev; dev++, ep++) {
			if (!ep->emd_rdq)
				break;
		}
		if (dev >= emd_maxdev)
			return(ENOSPC);
		*devp = makedevice(emd_bd[bid].eb_major, dev);
	} else {
		dev = getminor(*devp);
		if ((dev < 0) || (dev >= emd_maxdev))
			return(EINVAL);
		ep = emd_bd[bid].eb_emdp + dev;
	}
	ep->emd_rdq = q;
	ep->emd_head = NULL;
	ep->emd_tail = NULL;
	ep->emd_sap = 0;
	ep->emd_seq = 0;
	q->q_ptr = (char *)ep;
	WR(q)->q_ptr = (caddr_t)ep;
	noenable(WR(q));
	ep->emd_state = DL_UNBOUND;
	if (emd_bd[bid].eb_state == EB_DOWN) {
		emd_bd[bid].eb_state = EB_UNINIT;
		emd_bd[bid].eb_initep = ep;
	}
	EMDLOG(EMD_MID, ep - emd_emd, 0, SL_TRACE, "EMD - open", 0);
	return(0);
}

/*
 * EMD close procedure.  
 * All messages are freed.  
 */
STATIC int
emdclose(q, flag, credp)
	queue_t *q;
	int flag;
	cred_t *credp;
{
	register struct emd *ep;
	register int s;
	mblk_t *mp, *tmp;

	s = splemd();
	ep = (struct emd *)q->q_ptr;
	for (mp = ep->emd_head; mp;) {
		tmp = mp->b_next;
		mp->b_prev = (mblk_t *)NULL;
		freemsg(mp);
		mp = tmp;
	}
	if (WR(q)->q_first)
		flushq(WR(q), FLUSHALL);
	ep->emd_head = NULL;
	ep->emd_tail = NULL;
	ep->emd_rdq = NULL;
	ep->emd_sap = 0;
	ep->emd_state = DL_UNBOUND;
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	if (emd_bd[ep->emd_bid].eb_initep == ep) {
		if (emd_bd[ep->emd_bid].eb_state & EB_INIT)
			emd_bd[ep->emd_bid].eb_state = EB_DOWN;
		emd_bd[ep->emd_bid].eb_initep = NULL;
	}
	splx(s);
	EMDLOG(EMD_MID, ep - emd_emd, 0, SL_TRACE, "EMD - close", 0);
	return(0);
}

/*
 * EMD write side put procedure.  This performs the action indicated
 * by the message provided.  If the message requires no action by
 * the interface hardware, it is handled immediately.  Otherwise,
 * the message is either enqueued (because the firmware queue is full)
 * or a firmware job is created and the message put on the emd_head list.
 * emdint() will take care of handling the response from the 
 * hardware and sending messages back upstream.
 */
STATIC int
emdwput(q, mp)
	queue_t *q;
	mblk_t *mp;
{
	register int s;
	register struct emd *ep;
	
	ep = (struct emd *)q->q_ptr;
	ASSERT(ep);
	switch (mp->b_datap->db_type) {
	case M_FLUSH:
		EMDLOG(EMD_MID, ep - emd_emd, 0, SL_TRACE, "EMD - wput flush", 0);
		if (*mp->b_rptr & FLUSHW) {
			flushq(q, FLUSHALL);
			*mp->b_rptr &= ~FLUSHW;
		}
		if (*mp->b_rptr & FLUSHR) 
			qreply(q, mp);
		else 
			freemsg(mp);
		break;

	default:
		s = splemd();
		if (q->q_first != NULL) {
			EMDLOG(EMD_MID, ep - emd_emd, 0, SL_TRACE,
			    "EMD - wput enqueuing message", 0);
			putq(q, mp);	/* message must wait its turn */
			emd_bd[ep->emd_bid].eb_waitcnt++;
			splx(s);
			break;
		}
		splx(s);
		(void) emdwproc(ep, mp);
	}
	return(0);
}

/*
 * Dummy write service procedure for
 * flow control only.
 */
STATIC int
emdwsrv(q)
	queue_t *q;
{
	register struct emd *ep;
	
	ep = (struct emd *)q->q_ptr;
	if (!ep)
		return;
	EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - emdwsrv called", 0);
}

/*
 * Write side message processing procedure.  This is called from the
 * put procedure or from emdcproc() to process a message from a 
 * given stream.  If the message requires action by the interface
 * hardware,  a pointer to the message is stored on the stream, the 
 * action is initiated, and then emdwproc() returns.  The completion
 * of the action is left for emdcproc(), which either frees the
 * message or generates an appropriate response when the interface
 * activity has completed.
 * 1 is returned to indicate that the message was processed, 0
 * to indicate that it could not be processed and was enqueued.
 */
STATIC int
emdwproc(ep, mp)
	register struct emd *ep;
	mblk_t *mp;
{
	register mblk_t *tmp;
	register int s;
	register union DL_primitives *p;
	queue_t *rq;		/* read queue */
	struct iocblk *iocp;
	u_char vector;
	int i;
	int len;
	paddr_t temp;
	ushort sap, type;
	struct ehead *hp;
	struct emd *nep;
	struct emd_board *bdp;
	u_long id;
	int idx;
	int nacknum = 0;
	
	rq = ep->emd_rdq;
	bdp = &emd_bd[ep->emd_bid];
	switch(mp->b_datap->db_type) {
	case M_IOCTL:
		if ((mp->b_wptr - mp->b_rptr) < sizeof(struct iocblk)) {
			mp->b_datap->db_type = M_IOCNAK;
			putnext(rq, mp);
			return(1);
		}			
		iocp = (struct iocblk *)mp->b_rptr;		
		if (iocp->ioc_uid != 0) {
			mp->b_datap->db_type = M_IOCNAK;
			putnext(rq, mp);
			return(1);
		}
		if (iocp->ioc_count == TRANSPARENT) {	/* you lose */
			mp->b_datap->db_type = M_IOCNAK;
			putnext(rq, mp);
			return(1);
		}

		/*
		 * Most of these ioctls cause an action to be taken
		 * by the board and so cannot be acknowledged until
		 * a completion request is retured for this stream.
		 * emdcproc() handles the generation of the ack - the
		 * message is merely placed in the waiting list until the
		 * completion request comes back.
		 */
		switch (iocp->ioc_cmd) {
		case EI_GETA:	/* get board address */
		    {
			struct eiseta *setp;

			if ((mp->b_cont->b_wptr - mp->b_cont->b_rptr) != sizeof(struct eiseta))
				goto iocfail;
			setp = (struct eiseta *)mp->b_cont->b_rptr;
			eiacpy(emd_bd[ep->emd_bid].eb_addr, setp->eis_addr);
			iocp->ioc_count = sizeof(struct eiseta);
			iocp->ioc_error = 0;
			mp->b_datap->db_type = M_IOCACK;
			putnext(rq, mp);
			return(1);
		    }

		case EI_SETA:	/* set board address */
		    {
			struct eiseta *setp;

			if ((mp->b_cont->b_wptr - mp->b_cont->b_rptr) != sizeof(struct eiseta))
				goto iocfail;
			setp = (struct eiseta *)mp->b_cont->b_rptr;
			if ((emd_bd[ep->emd_bid].eb_state != EB_UNINIT) &&
			    (emd_bd[ep->emd_bid].eb_state != EB_DOWN) &&
			    (emd_bd[ep->emd_bid].eb_state != EB_RESET))
				goto iocfail;
			eiacpy(setp->eis_addr, emd_bd[ep->emd_bid].eb_addr);
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = 0;
			putnext(rq, mp);
			return(1);
		    }

		case EI_RESET:  /* sequence order 1 */
			EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - reset ioctl", 0);
			s = splemd();
			if (bdp->eb_state == EB_UNINIT) {
				nep = bdp->eb_emdp;
				for (i = 0; i < emd_maxdev; i++, nep++) {
					if (nep == ep)
						continue;
					if (nep->emd_rdq)
						putctl1(nep->emd_rdq->q_next,
							M_ERROR, ENXIO);
					nep->emd_state = DL_UNBOUND;
				}
				for (i = 0; i < RQSIZE; i++) {
					if (bdp->eb_srbuf[i]) {
						freeb(bdp->eb_srbuf[i]);
						bdp->eb_srbuf[i] = NULL;
					}
					if (bdp->eb_lrbuf[i]) {
						freeb(bdp->eb_lrbuf[i]);
						bdp->eb_lrbuf[i] = NULL;
					}
				}
				for (i = 0; i < (RQSIZE + RQSIZE); i++) {
					if (bdp->eb_ind[i]) {
						freeb(bdp->eb_ind[i]);
						bdp->eb_ind[i] = NULL;
					}
				}
			}
			vector = getvec(emd_addr[ep->emd_bid]);
			bdp->eb_initep = ep;
			EMD_RESET(ep->emd_bid);
			EMD_DELAY();
			if (emdfw_init(vector, ep->emd_bid) != PASS) {
				splx(s);
				cmn_err(CE_CONT,"EMD: init failed\n");
				goto iocfail;
			}
			bdp->eb_waitcnt = 0;
			bdp->eb_state = EB_RESET;
			break;

		case EI_LOAD: /* sequence order 2 */
		    {
			struct eipump *ip;

			EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, 
		 	  "EMD - load ioctl", 0);
			if ((bdp->eb_state != EB_RESET) &&
				(bdp->eb_state != EB_LOAD))
					goto iocfail;
			ip = (struct eipump *)mp->b_cont->b_rptr;
			if ((temp = kvtophys((caddr_t)ip->data)) == 0) 
				goto iocfail;
			s = splemd();
			BUMPSEQ(ep);
			id = emdmkid(ep-emd_emd, ep->emd_seq);
			if (emdfw_job(id,_EIDLM,temp,ip->size,((u_long)(ip->address))>>8,ep->emd_bid,GE_QUE) != PASS) {
				splx(s);
				goto iocfail;
			}
			bdp->eb_state = EB_LOAD;
			break;
		    }

		case EI_FCF:	/* sequence order 3 */
			EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - fcf ioctl", 0);
			if (bdp->eb_state != EB_LOAD)
				goto iocfail;
			s = splemd();
			/* BUMPSEQ(ep); probably a bug in pupware [sar] */
			id = emdmkid(ep-emd_emd, ep->emd_seq);
			if (emdfw_job(id,_EIFCF,*((long *)mp->b_cont->b_rptr),0,CS_STATUS,ep->emd_bid,GE_QUE) != PASS) {
				splx(s);
				goto iocfail;
			}
			bdp->eb_state = EB_FCF;
			mp->b_prev = (mblk_t *)ep->emd_seq;
			emd_add(ep, mp);
			emdtimeout(ep->emd_bid);
			splx(s);
			EMD_DELAY();
			return(1);

		case EI_SYSGEN:	/* sequence order 4 */
			EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - sysgen ioctl", 0);
			if (bdp->eb_state != EB_FCF)
				goto iocfail;
			vector = getvec(emd_addr[ep->emd_bid]);
			s = splemd();
			if (emdfw_init(vector,ep->emd_bid) != PASS) {
				splx(s);
				goto iocfail;
			}
			bdp->eb_state = EB_SYSGEN;
			break;

		case EI_SETID:
			EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, 
			  "EMD - setid ioctl", 0);
			if (bdp->eb_state != EB_SYSGEN)
				goto iocfail;
		 	if ((temp = kvtophys((caddr_t)bdp->eb_addr)) == 0)
				goto iocfail;
			s = splemd();
			BUMPSEQ(ep);
			id = emdmkid(ep-emd_emd, ep->emd_seq);
		 	if (emdfw_job(id,_EIPHYAD,temp,0,
			  CS_STATUS,ep->emd_bid,GE_QUE) != PASS) {
				splx(s);
				goto iocfail;
		 	}
			bdp->eb_state = EB_SETID;
			break;
			
		case EI_TURNON:
			EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - turnon ioctl", 0);
			if (bdp->eb_state != EB_SETID)
				goto iocfail;
			s = splemd();
			BUMPSEQ(ep);
			id = emdmkid(ep-emd_emd, ep->emd_seq);
		 	if (emdfw_on(id, ep->emd_bid) != PASS) {
				splx(s);
				goto iocfail;
		 	}
			bdp->eb_state = EB_TURNON;
			break;
			
		case EI_ALLOC:
			EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - alloc ioctl", 0);
			if (bdp->eb_state != EB_TURNON)
				goto iocfail;
			s = splemd();
			for (i = 0; i < (RQSIZE + RQSIZE); i++)
				emdgetrb(mkgetid(ep->emd_bid, i));
			bdp->eb_state = EB_UP;
			splx(s);
			mp->b_datap->db_type = M_IOCACK;
			putnext(rq, mp);
			return(1);

		case EI_TERM:
			EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - term ioctl", 0);
			s = splemd();
			emdterm(ep->emd_bid);
			break;
		default:
iocfail:
			mp->b_datap->db_type = M_IOCNAK;
			putnext(rq, mp);
			EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - ioctl failed", 0);
			return(1);
		} /* end of ioctl switch */

		mp->b_prev = (mblk_t *)ep->emd_seq;
		emd_add(ep, mp);
		emdtimeout(ep->emd_bid);
		splx(s);
		return(1);

	case M_PROTO:
	case M_PCPROTO:
		if (bdp->eb_state != EB_UP) {
			emdfatal(rq, mp);
			return(1);
		}
		p = (union DL_primitives *)mp->b_rptr;
		switch(p->dl_primitive) {
		case DL_BIND_REQ:
		    {
			dl_bind_req_t *reqp;
			dl_bind_ack_t *ackp;

			EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - bind", 0);
			if (ep->emd_state != DL_UNBOUND) {
				emdnonfatal(rq, mp, DL_BIND_REQ, DL_OUTSTATE, 0);
				return(1);
			}
			reqp = (dl_bind_req_t *)mp->b_rptr;
			sap = reqp->dl_sap;
			for (i = 0; i < (emd_maxdev * emd_nbds); i++) 
				if ((emd_emd[i].emd_state == DL_IDLE) &&
				    (sap == emd_emd[i].emd_sap)) {
					emdnonfatal(rq, mp, DL_BIND_REQ, DL_BADSAP, 0);
					return(1);
				}

			/* bind o.k., return dl_bind_ack_t */
			if (!REUSEABLE(mp, sizeof(*ackp)+PHYAD_SIZE)) {
				if (!(tmp = allocb(sizeof(*ackp)+PHYAD_SIZE, BPRI_MED))) {
					emdnonfatal(rq, mp, DL_BIND_REQ, DL_SYSERR, ENOMEM);
					return(1);
				}
				freemsg(mp);
				mp = tmp;
			} else {
				mp->b_rptr = mp->b_datap->db_base;
				mp->b_wptr = mp->b_rptr;
				if (mp->b_cont) {
					freemsg(mp->b_cont);
					mp->b_cont = NULL;
				}
			}

			mp->b_datap->db_type = M_PCPROTO;
			ackp = (dl_bind_ack_t *)mp->b_wptr;
			ackp->dl_primitive = DL_BIND_ACK;
			ackp->dl_sap = sap;
			ackp->dl_addr_length = PHYAD_SIZE;
			ackp->dl_addr_offset = sizeof(*ackp);
			ackp->dl_max_conind = 0;
			ackp->dl_growth = 0;
			mp->b_wptr += sizeof(*ackp);
			eiacpy(bdp->eb_addr, mp->b_wptr);
			mp->b_wptr += PHYAD_SIZE;

			ep->emd_state = DL_IDLE;
			ep->emd_sap = sap;
			putnext(rq, mp);
			return(1);
		    }

		case DL_UNBIND_REQ:
		    {
			dl_ok_ack_t *okp;

			EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - unbind", 0);
			if (ep->emd_state != DL_IDLE)  {
				emdnonfatal(rq, mp, DL_UNBIND_REQ, DL_OUTSTATE, 0);
				return(1);
			}
	
			/* unbind o.k., return dl_ok_ack_t */
			if (!REUSEABLE(mp, sizeof(dl_ok_ack_t))) {
				if (!(tmp = allocb(sizeof(dl_ok_ack_t), BPRI_MED))) {
					emdnonfatal(rq, mp, DL_UNBIND_REQ, DL_SYSERR, ENOMEM);
					return(1);
				}
				freemsg(mp);
				mp = tmp;
			} else {
				mp->b_rptr = mp->b_datap->db_base;
				mp->b_wptr = mp->b_rptr;
				if (mp->b_cont) {
					freemsg(mp->b_cont);
					mp->b_cont = NULL;
				}
			}

			okp = (dl_ok_ack_t *)mp->b_wptr;
			mp->b_datap->db_type = M_PCPROTO;
			okp->dl_primitive = DL_OK_ACK;;
			okp->dl_correct_primitive = DL_UNBIND_REQ;
			mp->b_wptr += sizeof(dl_ok_ack_t);
			ep->emd_state = DL_UNBOUND;
			ep->emd_sap = 0;
			putnext(rq, mp);
			return(1);
		    }

		case DL_INFO_REQ:
		    {
			dl_info_ack_t *ackp;

			EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - inforeq", 0);
			if (!REUSEABLE(mp, sizeof(dl_info_ack_t)+PHYAD_SIZE)) {
				if (!(tmp = allocb(sizeof(dl_info_ack_t)+PHYAD_SIZE, BPRI_MED))) {
					emdnonfatal(rq, mp, DL_UNBIND_REQ, DL_SYSERR, ENOMEM);
					return(1);
				}
				freemsg(mp);
				mp = tmp;
			} else {
				mp->b_rptr = mp->b_datap->db_base;
				mp->b_wptr = mp->b_rptr;
				if (mp->b_cont) {
					freemsg(mp->b_cont);
					mp->b_cont = NULL;
				}
			}

			mp->b_datap->db_type = M_PCPROTO;
			ackp = (dl_info_ack_t *)mp->b_wptr;
			ackp->dl_primitive = DL_INFO_ACK;
			ackp->dl_max_sdu = EMDMAXPSZ-EMDMINPSZ;
			ackp->dl_min_sdu = 1;
			ackp->dl_addr_length = PHYAD_SIZE;
			ackp->dl_mac_type = DL_ETHER;
			ackp->dl_reserved = 0;
			ackp->dl_current_state = ep->emd_state;
			ackp->dl_max_idu = EMDMAXPSZ-EMDMINPSZ;
			ackp->dl_service_mode = DL_CLDLS;
			ackp->dl_qos_length = 0;
			ackp->dl_qos_offset = 0;
			ackp->dl_qos_range_length = 0;
			ackp->dl_qos_range_offset = 0;
			ackp->dl_provider_style = DL_STYLE1;
			ackp->dl_addr_offset = sizeof(dl_info_ack_t);
			ackp->dl_growth = 0;
			mp->b_wptr += sizeof(dl_info_ack_t);
			eiacpy(bdp->eb_addr, mp->b_wptr);
			mp->b_wptr += PHYAD_SIZE;
			putnext(rq, mp);
			return(1);
		    }

		case DL_UNITDATA_REQ:
		    {
			dl_unitdata_req_t *dp;
			unchar *ap;
			int fillsz;
			struct ehead hdr;

			EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - datareq", 0);
			nacknum = 0;
			dp = (dl_unitdata_req_t *)mp->b_rptr;
			if (ep->emd_state != DL_IDLE) {
				nacknum = EMD_NOTBIND;
				goto nackdata;
			}
			s = splemd();
			if (RQFULL(ep->emd_bid, GE_QUE)) {
				putq(WR(rq), mp);
				bdp->eb_waitcnt++;
				splx(s);
				EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - firmware queue full", 0);
				return(0);
			}
			splx(s);
			ap = mp->b_rptr + dp->dl_dest_addr_offset;
			eiacpy(bdp->eb_addr, hdr.src_addr);
			eiacpy(ap, hdr.dest_addr);
			hdr.pro_type.val = ep->emd_sap;
			if (!REUSEABLE(mp, EHEADSIZE)) {
				if (!(tmp = allocb(EHEADSIZE, BPRI_MED))) {
					nacknum = EMD_NOMEM;
					goto nackdata;
				}
				tmp->b_cont = mp->b_cont;
				mp->b_cont = NULL;
				freeb(mp);
				mp = tmp;
			} else {
				mp->b_rptr = mp->b_datap->db_base;
			}
			bcopy((caddr_t)&hdr, (caddr_t)mp->b_rptr, EHEADSIZE);
			mp->b_wptr = mp->b_rptr + EHEADSIZE;

			/*
			 * Pad packet if necessary.
			 */
			len = 0;
			for (tmp = mp; tmp; tmp = tmp->b_cont)
				len += tmp->b_wptr - tmp->b_rptr;
			if (len > EMDMAXPSZ) {
				nacknum = EMD_PACK_SIZE;
				goto nackdata;
			} else if (len < EMDMINPSZ) {
				fillsz = EMDMINPSZ - len;
				for (tmp = mp->b_cont; tmp->b_cont; tmp = tmp->b_cont)
					;
				if ((tmp->b_datap->db_lim - tmp->b_wptr) >= fillsz) {
					tmp->b_wptr += fillsz;
				} else {
					tmp->b_cont = allocb(fillsz, BPRI_MED);
					if (!tmp->b_cont) {
						nacknum = EMD_NOMEM;
						goto nackdata;
					}
					tmp->b_cont->b_wptr += fillsz;
				}
				len += fillsz;
			}

			/*
			 * Kick it to the network.
			 */
			s = splemd();
			i = 0;
			for (tmp = mp; tmp; tmp = tmp->b_cont) {
				int size, off;

				/* get rid of empty block */
				size = tmp->b_wptr - tmp->b_rptr;
				if (size == 0)
					continue;
				off = (int)tmp->b_rptr % NBPC;
				if ((off + size) > NBPC) {
					/* data crosses page boundary */

					if (i > (MSGLENMAX - 2)) {
						nacknum = EMD_XPAGE;
						goto pullitup;
					}
					eignext->gblk[i].eig_addr =
					    kvtophys((caddr_t)tmp->b_rptr);
					eignext->gblk[i].eig_size =
					    NBPC - off;
					eignext->gblk[i].eig_last = 0;
					i++;
					eignext->gblk[i].eig_addr =
					    kvtophys((caddr_t)(tmp->b_rptr + NBPC - off));
					eignext->gblk[i].eig_size =
					    size - NBPC + off;
					eignext->gblk[i].eig_last =
					    (tmp->b_cont ? 0 : 1);
				} else {
					eignext->gblk[i].eig_addr =
					    kvtophys((caddr_t)tmp->b_rptr);
					eignext->gblk[i].eig_size =
					    size;
					eignext->gblk[i].eig_last =
					    (tmp->b_cont ? 0 : 1);
				}
				if (++i == MSGLENMAX && tmp->b_cont) {
					i--;
					nacknum = EMD_NOMEM;
					goto pullitup;
				}
			}
			goto sendit;
pullitup:
			if (!pullupmsg(tmp, -1))
				goto snackdata;
			eignext->gblk[i].eig_addr =
			    kvtophys((caddr_t)tmp->b_rptr);
			eignext->gblk[i].eig_size = tmp->b_wptr - tmp->b_rptr;
			eignext->gblk[i].eig_last = 1;
sendit:
			if ((temp = kvtophys((caddr_t)eignext)) == 0) {
				nacknum = EMD_EVTOP;
				goto snackdata;
			}
			BUMPSEQ(ep);
			id = emdmkid(ep-emd_emd, ep->emd_seq);
			if ((emdfw_job(id,_EIGSEND,temp,len,CS_STATUS,ep->emd_bid,GE_QUE)) != PASS) {
				nacknum = EMD_EFW;
				goto snackdata;
			}
			if ((++eignext - eigarray) == RQSIZE)
				eignext = eigarray;
			EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - gsend", 0);
			mp->b_prev = (mblk_t *)ep->emd_seq;
			emd_add(ep, mp);
			splx(s);
			return(1);
		    }

		default:
			EMDLOG(EMD_MID, ep-emd_emd, 0, (SL_TRACE|SL_ERROR), "EMD - unrecognized control message type %x", mp->b_datap->db_type);
			freemsg(mp);
			return(1);
		}

	default:
		EMDLOG(EMD_MID, ep-emd_emd, 0, (SL_TRACE|SL_ERROR), "EMD - unrecognized message type %x", mp->b_datap->db_type);
		emdfatal(rq, mp);
		return(1);

	} /* end of message type switch */

snackdata:
	splx(s);

nackdata:
	{
		dl_uderror_ind_t *errp;

		if (!REUSEABLE(mp, DL_UDERROR_IND_SIZE)) {
			freemsg(mp);
			if (!(mp = allocb(DL_UDERROR_IND_SIZE, BPRI_MED))) {
				cmn_err(CE_WARN, "EMD - could not allocate DL_UDERROR_IND\n");
				return(1);
			}
		} else {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
			mp->b_rptr = mp->b_datap->db_base;
		}
		errp = (dl_uderror_ind_t *)mp->b_rptr;
		/* dl_src_addr_length and dl_src_addr_offset are set to 0 */
		bzero((char *)errp, DL_UDERROR_IND_SIZE);
		errp->dl_primitive = DL_UDERROR_IND;
		errp->dl_errno = nacknum;
		mp->b_wptr = mp->b_rptr + DL_UDERROR_IND_SIZE;
		mp->b_datap->db_type = M_PCPROTO;
		putnext(rq, mp);
		EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - nacking data, num = %x", nacknum);
		return(1);
	}
}

/*
 * EMD interrupt handling procedure.  The interrupt id identifies the
 * stream for which the completed action was being performed and the 
 * message that initiated it.  The stream and message are then examined 
 * and the appropriate completion action carried out.  If another message
 * is waiting on that stream, it is then serviced.  Finally, if some
 * stream has been waiting for the request queue to empty out,
 * that stream is serviced (if possible).  
 * Note that there can be only one outstanding request per stream.
 */
int
emdint(bid)
	register long bid;
{
	register ulong id;
	register int index;
	mblk_t *bp;
	CENTRY c_entry;

getcqe:
	/*
	 * Get a completion queue entry.  If none is available return;
	 * otherwise handle the completion request and come back here.
	 */
	if (emdfw_attention(&c_entry, bid) != PASS)
		return(0);

	if ((id = (u_long)c_entry.appl) == 0xffffffff) {
		/*
		 * This is an unsolicited queue entry - it
		 * usually reports a transient problem such as
		 * internal queue overflow.  Ignored for now.
		 */
		EMDLOG(EMD_MID, -1, 0, SL_TRACE, "EMD - weird completion queue entry", 0);
		goto getcqe;
	}
	if (isrecv(id)) {

		/*
		 * A packet has been received on the interface.  Send the
		 * packet up the stream of the selected protocol, and then
		 * obtain a new receive buffer to replace the one just
		 * used.
		 */
		index = id2idx(id);
		if ((index < 0) || (index >= RQSIZE+RQSIZE)) {
			cmn_err(CE_CONT,"EMD - bad index value received, %x\n", id);
			goto getcqe;
		}
		if (index < RQSIZE)
			bp = emd_bd[bid].eb_srbuf[index];
		else
			bp = emd_bd[bid].eb_lrbuf[index-RQSIZE];
		bp->b_wptr += c_entry.common.codes.bytes.bytcnt;
		emdrproc(bid, index, bp);
		emdgetrb(mkgetid(bid, index));
		goto getcqe;
	}

	/*
	 * The completion queue entry is for a request made by one
	 * of the streams.  Call emdcproc() to process clean up
	 * after the request.
	 */
	if ((id2minor(id) < 0) || (id2minor(id) >= (emd_maxdev*emd_nbds))) {
		cmn_err(CE_CONT,"EMD - bad minor in id %x\n", id);
		goto getcqe;
	}
	emdcproc(bid, id2minor(id), id2seqid(id));
	goto getcqe;
}

/*
 * Get a new receive buffer for board bid, slot idx.
 * (getid = ((bid << 8) | idx)).
 */
STATIC void
emdgetrb(getid)
	register long getid;
{
	mblk_t *mp;
	paddr_t tmp;
	int size;
	int bid;
	int idx;
	long id;

	bid = getid2bid(getid);
	idx = getid2idx(getid);
	if (idx < RQSIZE)
		size = SRECSIZE;
	else
		size = LRECSIZE;
	if (!(mp = allocb(size, BPRI_HI))) {
		cmn_err(CE_CONT,
		   "EMD - could not allocate receive buffer of size %d\n",
		    size);
		if (!bufcall(size, BPRI_HI, emdgetrb, getid))
			timeout(emdgetrb, (caddr_t)getid, 1*HZ);
		return;
	}
	if (!emd_bd[bid].eb_ind[idx]) {	/* need to allocate new buffer */
		if (!(emd_bd[bid].eb_ind[idx] = allocb(DATINDSIZE, BPRI_HI))) {
			cmn_err(CE_CONT,
			   "EMD - could not allocate DL_UNITDATA_IND buffer\n");
			freeb(mp);
			if (!bufcall(size, BPRI_HI, emdgetrb, getid))
				timeout(emdgetrb, (caddr_t)getid, 1*HZ);
			return;
		}
	}

	/*
	 * Ethernet header is 14 bytes long.  We want data to
	 * be word-aligned.
	 */
	mp->b_rptr += 2;
	mp->b_wptr += 2;
	tmp = kvtophys((caddr_t)mp->b_rptr);
	id = mkrecid(idx, 0);
	if (size == SRECSIZE) {
		emd_bd[bid].eb_srbuf[idx] = mp;
		(void) emdfw_job(id, _EIRECV,tmp,SRECSIZE-2,CS_STATUS,bid,RD_S_QUE);
	} else {
		emd_bd[bid].eb_lrbuf[idx - RQSIZE] = mp;
		(void) emdfw_job(id, _EIRECV,tmp,LRECSIZE-2,CS_STATUS,bid,RD_L_QUE);
	}
}

/*
 * EMD interface job completion procedure.  
 */
STATIC void
emdcproc(bid, eidx, seqid)
	int bid;
	int eidx;
	ushort seqid;
{
	register struct emd *ep;
	register mblk_t *mp;
	register i;

	if (seqid == 0) {	/* pumpware is buggy */
		if (!(ep = emd_bd[bid].eb_initep)) {
			cmn_err(CE_CONT,"EMD: orphaned initialization interrupt\n");
			return;
		}
		if (ep->emd_head)
			seqid = (ushort)ep->emd_head->b_prev;
		
	} else {
		ep = &emd_emd[eidx];
	}

	for (mp = ep->emd_head; mp &&
	    (mp->b_prev != (mblk_t *)seqid) ; mp = mp->b_next)
		;
	if (!mp) {
		cmn_err(CE_CONT,"EMD: completion interrupt without waiting message\n");
		return;
	}

	emd_rmv(ep, mp);
	mp->b_prev = (mblk_t *)NULL;

	switch (mp->b_datap->db_type) {

	case M_IOCTL:
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
		((struct iocblk *)mp->b_rptr)->ioc_count = 0;
		mp->b_datap->db_type = M_IOCACK;
		putnext(ep->emd_rdq, mp);
		break;
		
	case M_PROTO:
	case M_DATA:
		freemsg(mp);
		break;

	default:
		cmn_err(CE_CONT, "EMD: unknown completion message type\n");
		break;
	}
	EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - completion handler", 0);
	while (emd_bd[bid].eb_waitcnt) {
		if (++emd_bd[bid].eb_emdnext >= emd_maxdev)
			emd_bd[bid].eb_emdnext = 0;
		ep = &emd_emd[emd_bd[bid].eb_emdnext];
		if (ep->emd_rdq  &&  WR(ep->emd_rdq)->q_first) {
			mp = getq(WR(ep->emd_rdq));
			emd_bd[bid].eb_waitcnt--;
			if (!emdwproc(ep, mp))
				return;
		}
	}
}

STATIC void
emdrproc(bid, index, mp) 
	long bid;
	long index;
	mblk_t *mp;
{
	register struct emd *ep;
	register mblk_t *bp;
	register ushort sap;
	struct ehead hdr;
	u_char *sp, *dp;

	dp = mp->b_rptr;
	mp->b_rptr += PHYAD_SIZE;
	sp = mp->b_rptr;
	mp->b_rptr += PHYAD_SIZE;
	hdr.pro_type.bytes[0] = *mp->b_rptr++; /* watch for byte order */
	hdr.pro_type.bytes[1] = *mp->b_rptr++;
	sap = hdr.pro_type.val;
	for (ep = emd_bd[bid].eb_emdp; ((ep->emd_sap != sap) ||
	    (ep->emd_state != DL_IDLE)); ep++)
		if (ep >= &emd_bd[bid].eb_emdp[emd_maxdev])
			goto free;
	if (!canput(ep->emd_rdq->q_next)) {
		EMDLOG(EMD_MID, ep-emd_emd, 0, (SL_TRACE|SL_ERROR),
		    "EMD: upstream queue full, dropping packet on floor", 0);
free:
		freemsg(mp);
		return;
	}
	bp = emd_bd[bid].eb_ind[index];
	emdunitind(bp, sp, dp);
	emd_bd[bid].eb_ind[index] = NULL;

	/*
	 * Note: we've skipped the ethernet header and because of our
	 * 2-byte offset, user data IS word-aligned.
	 */
	bp->b_cont = mp;
	EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - received message %x\n", mp);
	putnext(ep->emd_rdq, bp);
}

/*
 * This function handles the KERNEL LEVEL POWER CLEAR.  It will
 * reset the EMD firmware and hardware.  All communication to and
 * from the network will terminate.  The only way to recover will
 * be via a EMD "pump" command, which will re-initialize the
 * hardware and firmware.
 */
STATIC void
emdterm(bid)
	long bid;
{
	register int s;
	register struct emd *ep;
	int i;
	long index;

	ep = emd_bd[bid].eb_emdp;
	for(index = 0; index < emd_maxdev; index++, ep++) {
		if (ep->emd_rdq) {
			putctl(ep->emd_rdq->q_next, M_HANGUP);
			ep->emd_state = DL_UNBOUND;
		}
	}
	/* reset the EMD board */
	s = splemd();
	EMD_RESET(bid);
	splx(s);
	emd_bd[bid].eb_state = EB_DOWN;

	/* clear the receive buffers */
	for (i = 0; i < RQSIZE; i++) {
		if (emd_bd[bid].eb_srbuf[i]) {
			freemsg(emd_bd[bid].eb_srbuf[i]);
			emd_bd[bid].eb_srbuf[i] = NULL;
		}
	}
	for (i = 0; i < RQSIZE; i++) {
		if (emd_bd[bid].eb_lrbuf[i]) {
			freemsg(emd_bd[bid].eb_lrbuf[i]);
			emd_bd[bid].eb_lrbuf[i] = NULL;
		}
	}
	for (i = 0; i < (RQSIZE + RQSIZE); i++) {
		if (emd_bd[bid].eb_ind[i]) {
			freeb(emd_bd[bid].eb_ind[i]);
			emd_bd[bid].eb_ind[i] = NULL;
		}
	}
}

#ifdef UNUSED
/*
 * This function gets called when a timer that was set timed out.
 * The timeout occured because the EMD hardware gave no response
 * back (interrupt) for max time allowed.
 */
STATIC void
emd_time(board)
	int board;
{
	struct emd *ep;
	mblk_t *mp;
	int i;

	ep = emd_bd[board].eb_emdp;
	for (i = 0; i < emd_maxdev; i++, ep++)
		if (ep->emd_rdq) {
			if ((mp = allocb(4, BPRI_HI)) == NULL) {
				continue;
			} else {
				emdfatal(ep->emd_rdq, mp);
			}
		}
	emd_bd[board].eb_timeid = 0;	/* reset the board timer */
	EMD_RESET(board);	/* reset the emd board */
	emd_bd[board].eb_state = EB_DOWN;
	return;
}
#endif

STATIC void
emdtimeout(bid)
	int bid;
{
#ifdef UNUSED
	if (emd_bd[bid].eb_timeid)
		untimeout(emd_bd[bid].eb_timeid); /*reset the timer */
	/* start the timer again */
	emd_bd[bid].eb_timeid = timeout(emd_time, bid, EMDTIME);
	return;
#endif
}

/*
 * This function is used to send requests to the firmware
 * on the GE_QUE or RD_S_QUE or RD_L_QUE.
 */
STATIC short
emdfw_job(id, opcode, addr, size, subdev, bid, q)
	u_long id;
	char opcode;		/* firmware job opcode */
	u_long addr;	/* address for the firmware */
	u_long size;	/* size field for the firmware */
	long subdev;		/* sub-device number */
	long bid;		/* EMD board number */
	long q;			/* EMD firmware que */
{
	register int rindex;
	register int s;

	if(RQFULL(bid,q))
		return(FULQUE);
	rindex = RLOAD(bid,q) / sizeof(RENTRY);
	s = splemd();
	emd_bd[bid].eb_rq.queue[q].entry[rindex].common.codes.bytes.subdev = subdev;
	emd_bd[bid].eb_rq.queue[q].entry[rindex].common.codes.bytes.opcode = opcode;
	emd_bd[bid].eb_rq.queue[q].entry[rindex].appl = id;
	emd_bd[bid].eb_rq.queue[q].entry[rindex].common.addr = addr;
	emd_bd[bid].eb_rq.queue[q].entry[rindex].common.codes.bytes.bytcnt = size;
	RLOAD(bid,q) = sizeof(RENTRY) * NEW_RLPTR(bid,q);
	EMD_ASS(bid);
	splx(s);
	return(PASS);
}

/*
 * This function is used to initialize the firmware/driver
 * interface and queues.
 */
STATIC short
emdfw_init(vect, bid)
	u_char vect;
	register long bid;	/* EMD board number */
{
	register paddr_t temp;
	char int_id;

	int_id = *((char *)(emd_addr[bid] + 1));

	EMD_DELAY();
	if ((temp = kvtophys((caddr_t)&emd_bd[bid].eb_rq)) == -1) {
		return(FAIL);
	} 
	emd_bd[bid].eb_sgdblk.request = temp;
	if ((temp = kvtophys((caddr_t)&emd_bd[bid].eb_cq)) == -1) {
		return(FAIL);
	} 
	emd_bd[bid].eb_sgdblk.complt = temp;
	emd_bd[bid].eb_sgdblk.req_size = RQSIZE;
	emd_bd[bid].eb_sgdblk.comp_size = CQSIZE;
	emd_bd[bid].eb_sgdblk.int_vect = vect;
	emd_bd[bid].eb_sgdblk.no_rque = NUM_QUEUES;
	if ((temp = kvtophys((caddr_t)&emd_bd[bid].eb_sgdblk)) == -1) {
		return(FAIL);
	} 
	SG_PTR = (long)temp;
	RLOAD(bid,GE_QUE) = 0;
	RULOAD(bid,GE_QUE) = 0;
	RLOAD(bid,RD_S_QUE) = 0;
	RULOAD(bid,RD_S_QUE) = 0;
	RLOAD(bid,RD_L_QUE) = 0;
	RULOAD(bid,RD_L_QUE) = 0;
	CLOAD(bid) = 0;
	CULOAD(bid) = 0;
	EMD_ASS(bid);
	EMD_DELAY();
	return(PASS);
}

STATIC short
emdfw_attention(p_entry,bid)
	CENTRY	*p_entry;
	register long bid;		/* EMD board number */
{
	register s;
	int cindex;

	if(CQEMPTY(bid)) {
		return(EMPQUE);
	}
	cindex = CULOAD(bid) / sizeof(CENTRY);
	s = splemd();
	*p_entry = emd_bd[bid].eb_cq.queue.entry[cindex];
	CULOAD(bid) = sizeof(CENTRY) * NEW_CUPTR(bid);
	if(p_entry->common.codes.bits.opcode != 0 && p_entry->common.codes.bits.opcode != 3) {
		/*emdfw_error(p_entry->common.codes.bits.opcode,bid);*/
		splx(s);
		return(FAIL);
	}
	splx(s);
	return(PASS);
}

/*
 * fill in DL_unitdata_ind, mp is large enough upon entry
 */
STATIC void
emdunitind(mp, srcaddr, destaddr)
	register mblk_t *mp;
	char *srcaddr, *destaddr;
{
	register dl_unitdata_ind_t *dp;
	register unchar *p;

	dp = (dl_unitdata_ind_t *)mp->b_wptr;
	dp->dl_primitive = DL_UNITDATA_IND;
	dp->dl_dest_addr_length = PHYAD_SIZE;
	dp->dl_dest_addr_offset = sizeof(*dp);
	dp->dl_src_addr_length = PHYAD_SIZE;
	dp->dl_src_addr_offset = sizeof(*dp) + PHYAD_SIZE;
	/* fill in remote address */
	p = mp->b_wptr + dp->dl_dest_addr_offset;
	eiacpy(destaddr, p);
	/* fill in local address */
	p = mp->b_wptr + dp->dl_src_addr_offset;
	eiacpy(srcaddr, p);
	dp->dl_reserved = 0;
	mp->b_wptr += DATINDSIZE;
	mp->b_datap->db_type = M_PROTO;
}

STATIC void
emdnonfatal(rq, mp, prim, dlerr, unixerr)
queue_t *rq;		/* read q */
mblk_t *mp;
long prim, dlerr, unixerr;
{
	register dl_error_ack_t *errp;

	EMDLOG(EMD_MID, ((struct emd *)rq->q_ptr)-emd_emd, 0, SL_TRACE, 
		"EMD - nonfatal errno %d\n", unixerr);

	if (!REUSEABLE(mp, sizeof(*errp))) {
		mblk_t *bp;
		if ((bp = allocb(sizeof(*errp), BPRI_HI)) == NULL) {
			freemsg(mp);
			return;
		}
		freemsg(mp);
		mp = bp;
	} else {
		mp->b_rptr = mp->b_datap->db_base;
		mp->b_wptr = mp->b_rptr;
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = NULL;
		}
	}
	mp->b_datap->db_type = M_PCPROTO;
	errp = (dl_error_ack_t *)mp->b_wptr;
	errp->dl_primitive = DL_ERROR_ACK;
	errp->dl_error_primitive = prim;
	errp->dl_errno = dlerr;
	errp->dl_unix_errno = unixerr;
	mp->b_wptr += sizeof(*errp);
	putnext(rq, mp);
}

/*
 * fatal error: should have set emd_state to error state, instead UNBND
 */
STATIC void
emdfatal(rq, mp)
	queue_t *rq;
	mblk_t *mp;
{
	struct emd *ep;

	ep = (struct emd *)rq->q_ptr;
	ep->emd_state = DL_UNBOUND;
	EMDLOG(EMD_MID, ep-emd_emd, 0, SL_TRACE, "EMD - fatal error\n", 0);
	mp->b_datap->db_type = M_ERROR;
	mp->b_wptr = mp->b_rptr = mp->b_datap->db_base;
	*mp->b_wptr++ = EPROTO;
	putnext(rq, mp);
}

/*
 * Map external major numbers to boards.
 */
STATIC void
emd_domajors(dev)
	register dev_t dev;
{
	register int bid;
	register maj = -1;

	dev = getmajor(dev);
	for (bid = 0; bid < emd_nbds; bid++) {
		maj = itoemajor(dev, maj);
		if (maj == -1)
			cmn_err(CE_PANIC, "EMD: more boards than majors configured\n");
		emd_bd[bid].eb_major = maj;
	}
	if (itoemajor(dev, maj) != -1)
		cmn_err(CE_PANIC, "EMD: more majors than boards configured\n");
	emd_majinit = 1;
}

/*
 * Given a dev, return the corresponding board id.
 */
STATIC int
getboard(dev)
	dev_t dev;
{
	register int i;

	dev = getemajor(dev);
	for (i = 0; i < emd_nbds; i++) {
		if (dev == emd_bd[i].eb_major)
			return(i);
	}
	return(-1);
}

