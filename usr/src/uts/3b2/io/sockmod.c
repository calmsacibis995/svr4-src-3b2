/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/sockmod.c	1.4.2.6"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

/*
 * Socket Interface Library cooperating module
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/strsubr.h>
#include <sys/socketvar.h>
#include <sys/tihdr.h>
#include <sys/timod.h>
#include <sys/un.h>
#include <sys/sockmod.h>
#include <sys/socket.h>
#include <sys/tiuser.h>
#include <sys/debug.h>
#include <sys/strlog.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/cred.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/sysmacros.h>
#include <sys/ticlts.h>

#define _AF_UNIX

#define	SIMOD_ID	50
#define SIMWAIT		(1*HZ)
#define	MSGBLEN(A)	((A)->b_wptr - (A)->b_rptr)
#define	MBLKLEN(A)	(int)((A)->b_datap->db_lim - (A)->b_datap->db_base)
#define _S_RECOVER(A, B, C) \
			{ \
			putbq((A), (B)); \
			if (!bufcall((C), BPRI_MED, qenable, (caddr_t)(A))) \
				(void)timeout(qenable, (caddr_t)(A), SIMWAIT); \
			}

int sockdevflag = 0;	/* New style module */

/* Doubly linked list of so_so that
 * represent a UNIX domain socket.
 */
struct so_ux {
	struct so_so *next;
	struct so_so *prev;
};

typedef struct sockaddr * sockaddr_t;

/* Pointer to the beginning of a list of
 * so_so entries that represent UNIX
 * domain sockets.
 */
STATIC struct so_so	*so_ux_list = NULL;

struct so_so {
	long 	 		flags;
	queue_t			*rdq;
	mblk_t  		*iocsave;
	struct	t_info	 	tp_info;
	struct	netbuf		raddr;
	struct	netbuf		laddr;
	struct	ux_extaddr	lux_dev;
	struct	ux_extaddr	rux_dev;
	int	 		so_error;
	mblk_t  		*oob;
	struct	so_so		*so_conn;
	mblk_t 			*consave;
	struct	si_udata	udata;
	int	 		so_option;
	mblk_t  		*bigmsg;
	struct	so_ux		so_ux;
	ulong			msgcnt;
};

extern	struct so_so	so_so[];
extern 	int		so_cnt;
extern	int		nulldev();

STATIC	int		tlitosyserr();
STATIC	long		_t_setsize();
STATIC	int		so_options();
STATIC	mblk_t		*_s_getmblk();
STATIC	void		snd_ERRACK();
STATIC	void		snd_OKACK();
STATIC	int		so_init();
STATIC	void		strip_zerolen();
STATIC	void		save_addr();
STATIC	void		snd_SIGPIPE();
STATIC	void		snd_ZERO();
STATIC	void		snd_FLUSHR();
STATIC	void		snd_ERRORW();
STATIC	void		snd_IOCNAK();
STATIC	void		ux_dellink();
STATIC	void		ux_addlink();
STATIC	struct so_so	*ux_findlink();
STATIC	void		ux_restoreaddr();
STATIC	void		ux_saveraddr();
STATIC	void		fill_udata_req_addr();
STATIC	void		fill_udata_ind_addr();

int	socklog = 0;
#define SOCKLOG(S, A, B, C, D, E, F)  \
			{ \
			if (((S) && (S)->udata.so_options & SO_DEBUG) || \
							socklog & 1) { \
				if (socklog & 2) \
					(void)printf((E), (F)); \
				else	(void)strlog((A), (B), (C), (D), \
							(E), (F)); \
			} \
			}

/* Standard STREAMS templates.
 */
int 	sockmodopen(),
	sockmodclose(),
	sockmodrput(),
	sockmodwput(),
	sockmodwsrv(),
	sockmodrsrv();

STATIC struct module_info sockmod_info = {
	SIMOD_ID,
	"sockmod",
	0,		/* Write side set in sockmodopen() */
	INFPSZ,		/* Write side set in sockmodopen() */
	512,		/* Always left small */
	128		/* Always left small */
};
STATIC struct qinit sockmodrinit = {
	sockmodrput,
	sockmodrsrv,
	sockmodopen,
	sockmodclose,
	nulldev,
	&sockmod_info,
	NULL
};
STATIC struct qinit sockmodwinit = {
	sockmodwput,
	sockmodwsrv,
	sockmodopen,
	sockmodclose,
	nulldev,
	&sockmod_info,
	NULL
};
struct	streamtab sockinfo = {
	&sockmodrinit,
	&sockmodwinit,
	NULL,
	NULL
};


/*
 * sockmodopen - open routine gets called when the
 *	       module gets pushed onto the stream.
 */
/*ARGSUSED*/
sockmodopen(q, dev, flag, sflag, crp)
	register queue_t	*q;
	register dev_t		*dev;
	register int		flag;
	register int		sflag;
	register cred_t		*crp;
{
	register struct so_so		*so;
	register struct stroptions	*stropt;
	register mblk_t			*bp;

	ASSERT(q != NULL);

SOCKLOG((struct so_so *)NULL, SIMOD_ID, 0, 0, SL_TRACE, "sockmodopen: Entered queue %x\n", q);
	if (q->q_ptr)
		return 1;

	for (so=so_so; so < &so_so[so_cnt]; so++)
		if (!(so->flags & USED))
			break;

	if (so >= &so_so[so_cnt])
		return ENOSPC;

SOCKLOG((struct so_so *)NULL, SIMOD_ID, 0, 0, SL_TRACE, "sockmodopen: Allocated so %x\n", so);
	so->flags = USED;
	so->rdq = q;
	so->iocsave = NULL;
	so->consave = NULL;
	so->oob = NULL;
	so->so_error = 0;
	so->so_option = 0;
	so->so_conn = NULL;
	so->msgcnt = 0;
	bzero((caddr_t)&so->udata, sizeof(struct si_udata));

	bzero((caddr_t)&so->lux_dev, sizeof(struct ux_extaddr));
	bzero((caddr_t)&so->rux_dev, sizeof(struct ux_extaddr));

	so->so_ux.next = NULL;
	so->so_ux.prev = NULL;
	q->q_ptr = (caddr_t)so;
	WR(q)->q_ptr = (caddr_t)so;

	/* Send down T_INFO_REQ and wait for a reply
	 */
	if ((bp = allocb(sizeof(struct T_info_req) +
			 sizeof(struct T_info_ack), BPRI_LO)) == NULL) {
		so->flags = 0;
		return ENOSR;
	}
 
	so->flags |= S_WINFO;
	bp->b_datap->db_type = M_PCPROTO;
	*(long *)bp->b_wptr = T_INFO_REQ;
	bp->b_wptr += sizeof(struct T_info_req);
	putnext(WR(q), bp);

	while (so->flags & S_WINFO) {
		 if (sleep(so, PZERO+1|PCATCH)) {
			/* Interrupted
			 */
			so->flags = 0;
			return EINTR;
		}
	}
	if (so->so_error) {
		/* Some bad error occurred.
		 */
		so->flags = 0;
		return so->so_error;
	}
	/* Reserve space for local and remote
	 * addresses.
	 */
	if ((so->laddr.buf = (char *)kmem_alloc(so->udata.addrsize, 
						KM_SLEEP)) == NULL) {
		so->flags = 0;
		return EAGAIN;
	}
	so->laddr.maxlen = so->udata.addrsize;
	so->laddr.len = 0;
	(void)bzero(so->laddr.buf, so->udata.addrsize);

	if ((so->raddr.buf = (char *)kmem_alloc(so->udata.addrsize, 
						KM_SLEEP)) == NULL) {
		so->flags = 0;
		kmem_free(so->laddr.buf, so->laddr.maxlen);
		return EAGAIN;
	}
	so->raddr.maxlen = so->udata.addrsize;
	so->raddr.len = 0;
	(void)bzero(so->raddr.buf, so->udata.addrsize);

	/* Set our write maximum and minimum packet sizes
	 * to that of the transport provider. If a
	 * transport provider is to be PR_ATOMIC then
	 * its q_minpsz should be set to 1 for write(2)
	 * to fail messages to large to be sent in a 
	 * single crack.
	 */
	OTHERQ(q)->q_minpsz = (OTHERQ(q)->q_next)->q_minpsz;
	OTHERQ(q)->q_maxpsz = (OTHERQ(q)->q_next)->q_maxpsz;
SOCKLOG(so, SIMOD_ID, 0, 0, SL_TRACE, "sockmodopen: WRq minpsz %x\n", OTHERQ(q)->q_minpsz);
SOCKLOG(so, SIMOD_ID, 0, 0, SL_TRACE, "sockmodopen: WRq maxpsz %x\n", OTHERQ(q)->q_maxpsz);

	/* Set stream head option.
	 * Set M_READ and discard M_PROTO/M_PASSFP
	 * messages in strread().
	 */
	if ((bp = allocb(sizeof(*stropt), BPRI_MED)) == NULL) {
		so->flags = 0;
		return ENOSR;
	}
	bp->b_datap->db_type = M_SETOPTS;
	stropt = (struct stroptions *)bp->b_rptr;
	stropt->so_flags = SO_MREADON | SO_READOPT;
	stropt->so_readopt = RPROTDIS;
	bp->b_wptr += sizeof(*stropt);
	putnext(q, bp);

	return 0;
}


/*
 * sockmodclose - This routine gets called when the module
 *		gets popped off of the stream.
 */
/*ARGSUSED*/
sockmodclose(q, flag, credp)
	register queue_t	*q;
	register int 		flag;
	cred_t   		*credp;
{
	register struct so_so	*so;
	register mblk_t		*mp;
	register mblk_t		*nmp;

	ASSERT(q != NULL);
	so = (struct so_so *)q->q_ptr;
	ASSERT(so != NULL);

SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodclose: Entered\n", 0);

	/* Put any remaining messages downstream.
	 */
	while ((mp = getq(OTHERQ(q))) != NULL)
		putnext(OTHERQ(q), mp);

	freemsg(so->iocsave);

	mp = so->consave;
	while (mp) {
		nmp = mp->b_next;
		freemsg(mp);
		mp = nmp;
	}
	if (so->oob)
		freemsg(so->oob);

	kmem_free(so->laddr.buf, so->laddr.maxlen);
	kmem_free(so->raddr.buf, so->raddr.maxlen);

	/* If this was a UNIX domain endpoint, then
	 * update the linked list.
	 */
#ifdef _AF_UNIX
	ux_dellink(so);
#endif
	so->flags = 0;
}


/*
 * sockmodrput - Module read queue put procedure.
 *		 This is called from the module or
 *		 driver downstream.
 *		 Handles data messages if it can and
 *		 queues the rest.
 */
sockmodrput(q, mp)
	register queue_t	*q;
	register mblk_t		*mp;
{
	register union T_primitives	*pptr;
	register struct so_so		*so;
	register int			s;

	ASSERT(q != NULL);
	so = (struct so_so *)q->q_ptr;
	ASSERT(so != NULL);

	switch(mp->b_datap->db_type) {
	default:
		putq(q, mp);
		return 0;

	case M_FLUSH:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrput: Got M_FLUSH\n", 0);
		if (*mp->b_rptr & FLUSHW)
			flushq(WR(q), FLUSHDATA);
		if (*mp->b_rptr & FLUSHR)
			flushq(q, FLUSHDATA);

		putnext(q, mp);
		return 0;

	case M_IOCACK: {
		register struct iocblk		*iocbp;
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrput: Got M_IOCACK\n", 0);

		iocbp = (struct iocblk *)mp->b_rptr;
		switch(iocbp->ioc_cmd) {
		default:
			putq(q, mp);
			return 0;

		case TI_GETPEERNAME:
			if (so->udata.servtype == T_CLTS)
				so->udata.so_state |= SS_ISCONNECTED;
			/* Deliberate fall thru.
			 */
		case TI_GETMYNAME:
			ASSERT(so->iocsave != NULL);
			freemsg(so->iocsave);
			so->iocsave = NULL;
			so->flags &= ~WAITIOCACK;
			putnext(q, mp);
			return 0;
		}	
	}

	case M_IOCNAK: {
		register struct iocblk		*iocbp;
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrput: Got M_IOCNAK\n", 0);

		iocbp = (struct iocblk *)mp->b_rptr;
		switch(iocbp->ioc_cmd) {
		default:
			putq(q, mp);
			return 0;

		case TI_GETMYNAME:
			ASSERT(so->iocsave != NULL);
			freemsg(mp);
			mp = so->iocsave;
			so->iocsave = NULL;
			so->flags |= NAMEPROC;
			if (ti_doname(WR(q), mp, so->laddr.buf, so->laddr.len,
				so->raddr.buf, so->raddr.len) != DONAME_CONT) {
				so->flags &= ~NAMEPROC;
			}
			so->flags &= ~WAITIOCACK;
			return 0;

		case TI_GETPEERNAME:
			freemsg(mp);
			ASSERT(so->iocsave != NULL);
			mp = so->iocsave;
			so->iocsave = NULL;
			so->flags |= NAMEPROC;
			if (ti_doname(WR(q), mp, so->laddr.buf, so->laddr.len,
				so->raddr.buf, so->raddr.len) != DONAME_CONT) {
				so->flags &= ~NAMEPROC;
			}
			so->flags &= ~WAITIOCACK;
			if (so->udata.servtype == T_CLTS)
				so->udata.so_state |= SS_ISCONNECTED;
			return 0;
		}	
	}

	case M_PROTO:
	case M_PCPROTO:
		/* Assert checks if there is enough data to determine type
		 */
		ASSERT(MSGBLEN(mp) >= sizeof(long));

		pptr = (union T_primitives *)mp->b_rptr;

		switch (pptr->type) {
		default:
			putq(q, mp);
			return 0;

		case T_UDERROR_IND:
			/* Just set so_error.
			 */
			so->so_error = pptr->uderror_ind.ERROR_type;
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrput: Got T_UDERROR_IND error %d\n", so->so_error);
			freemsg(mp);
			return 0;

		case T_UNITDATA_IND: {
			register struct T_unitdata_ind	*udata_ind;
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrput: Got T_UNITDATA_IND\n", 0);
			/* If we are connected, then we must ensure the
			 * source address is the one we connected to.
			 */
			udata_ind = (struct T_unitdata_ind *)mp->b_rptr;
			if (so->udata.so_state & SS_ISCONNECTED) {
				if (((sockaddr_t)so->raddr.buf)->sa_family
								== AF_UNIX) {
#ifdef _AF_UNIX
					register char	*addr;

					addr = (char *)(mp->b_rptr +
						udata_ind->SRC_offset);
					if (bcmp(addr,
						(caddr_t)&so->rux_dev.addr,
						so->rux_dev.size) != 0) {
						/* Log error and free the msg.
					 	 */
						so->so_error = EINVAL;
						freemsg(mp);
						return 0;
					}
#endif
				}
				else	{
					if (bcmp(so->raddr.buf, 
				  	      mp->b_rptr+udata_ind->SRC_offset,
					      so->raddr.len) != 0) {
						/* Log error and free the msg.
					 	 */
						so->so_error = EINVAL;
						freemsg(mp);
						return 0;
					}
				}
			}

			/* If flow control is blocking us then
			 * let the service procedure handle it.
			 */
			s = splstr();
			if (!canput(q->q_next)) {
				putq(q, mp);
				(void)splx(s);
				return 0;
			}
			(void)splx(s);

			if (((sockaddr_t)so->raddr.buf)->sa_family == AF_UNIX) {
#ifdef _AF_UNIX
				register struct so_so	*oso;
				register int		size;
				register char 		*addr;

				addr = (caddr_t)(mp->b_rptr +
						udata_ind->SRC_offset);
				if ((oso = ux_findlink(addr,
					  udata_ind->SRC_length)) == NULL) {
					freemsg(mp);
					return 0;
				}

				size = sizeof(*udata_ind) + oso->laddr.len;
				if (MBLKLEN(mp) < size) {
					register mblk_t		*bp;

					if ((bp = _s_getmblk(NULL, size))
								 == NULL) {
						putq(q, mp);
						return 0;
					}
					*(struct T_unitdata_ind *)bp->b_wptr =
							*udata_ind;

					bp->b_cont = mp->b_cont;
					mp->b_cont = NULL;
					freemsg(mp);
					mp = bp;
				}
				fill_udata_ind_addr(mp, oso->laddr.buf,
							oso->laddr.len);
#endif
			}
			/* Check for zero length message and ensure that
			 * we always leave one linked to the header if
			 * there is no "real" data.
			 * This facilitates sending zero length messages
			 * on dgram sockets.
			 */
			if (mp->b_cont && msgdsize(mp->b_cont) == 0) {
				register mblk_t		*bp;

				/* Zero length message.
				 */
SOCKLOG(so, SIMOD_ID, -1, 0, SL_TRACE, "sockmodrput: Got zero length msg\n", 0);
				bp = mp->b_cont;
				freemsg(bp->b_cont);
				bp->b_cont = NULL;
			}
			else	strip_zerolen(mp);

			putnext(q, mp);
			return 0;
		}

		case T_EXDATA_IND:

SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrput: Got T_EXDATA_IND\n", 0);
			/* We want to queue this, but to maintain
			 * its position in the data stream we have
			 * to make sure any succeeding data is sent
			 * upstream AFTER the expedited data.
			 */
			s = splstr();
			so->flags |= S_RBLOCKED;
			(void)splx(s);
			putq(q, mp);

			return 0;

		case T_DATA_IND:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrput: Got T_DATA_IND\n", 0);
			so->msgcnt--;

			/* If the socket is marked such that we don't
			 * want to get anymore data then free it.
			 */
			if (so->udata.so_state & SS_CANTRCVMORE) {
				freemsg(mp);
				return 0;
			}

			strip_zerolen(mp);

			s = splstr();
			if (so->flags & S_RBLOCKED || !canput(q->q_next)) {
				if ((so->flags & S_RBLOCKED) == 0)
					so->flags |= S_RBLOCKED;
				putq(q, mp);
				(void)splx(s);
			}
			else	{
				(void)splx(s);
				putnext(q, mp);
			}
			return 0;
		}
	}
}

/*
 * sockmodrsrv - Module read queue service procedure.
 *		 Handles everything the write put 
 *		 procedure dosen't want to.
 */
sockmodrsrv(q)
	register queue_t	*q;
{
	register union T_primitives	*pptr;
	register struct so_so		*so;
	register struct iocblk		*iocbp;
	register mblk_t			*bp;
	register mblk_t			*mp;
	register int			s;
	register int			size;

	ASSERT(q != NULL);
	so = (struct so_so *)q->q_ptr;
	ASSERT(so != NULL);

rgetnext:
	if ((mp = getq(q)) == NULL) {
		if (so->flags & S_RBLOCKED)
			so->flags &= ~S_RBLOCKED;
		return 0;
	}

	switch(mp->b_datap->db_type) {
	default:
		putnext(q, mp);
		goto rgetnext;

	case M_PROTO:
	case M_PCPROTO:
		/* Assert checks if there is enough data to determine type
		 */
		ASSERT(MSGBLEN(mp) >= sizeof(long));

		pptr = (union T_primitives *)mp->b_rptr;

		switch (pptr->type) {
		default:
			putnext(q, mp);
			goto rgetnext;

		case T_DISCON_IND:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrsrv: Got T_DISCON_IND Reason: %d\n", pptr->discon_ind.DISCON_reason);
			so->udata.so_state |= (SS_CANTRCVMORE|SS_CANTSENDMORE);
			so->so_error = pptr->discon_ind.DISCON_reason;

			/* If this is in response to a disconnect,
			 * and the caller is waiting, then send the
			 * disconnect up.
			 */
			if (so->udata.so_state & SS_ISCONNECTING &&
						!(so->flags & S_WRDISABLE)) {
				/* Make sure we can get a buffer before we
				 * carry on.
				 */
				if ((bp = _s_getmblk(NULL, 2)) == NULL) {
					_S_RECOVER(q, mp, 2);
					return 0;
				}
				/* Send the disconnect up, so that
				 * the reason can be extracted.
				 */
				putnext(q, mp);
				mp = bp;
			}
			else	{
				mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
			}

			/* Send up an M_ERROR with the disconnect reason
			 */
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrsrv: Sending up M_ERROR\n",0);
			/* New M_ERROR flavour.
			 */
			mp->b_datap->db_type = M_ERROR;
			*mp->b_wptr++ = so->so_error;
			*mp->b_wptr++ = EPIPE;
			putnext(q, mp);

			/* Send a SIGPIPE for good luck.
			 */
			snd_SIGPIPE(q);

			goto rgetnext;

		case T_ORDREL_IND:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrsrv: Got T_ORDREL_IND\n", 0);
			so->udata.so_state |= SS_CANTRCVMORE;

			/* Send up zero length message(EOF) to
			 * wakeup anyone in a read(), or select().
			 */
			freemsg(mp);
			snd_ZERO(q);

			goto rgetnext;

		case T_CONN_IND: {
			register mblk_t			*nbp;
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrsrv: Got T_CONN_IND\n", 0);
			/* Make sure we can dup the new
			 * message before proceeding.
			 */
			if ((nbp = dupmsg(mp)) == NULL) {
				_S_RECOVER(q, mp, sizeof(mblk_t));
				return 0;
			}

			if (((sockaddr_t)so->laddr.buf)->sa_family == AF_UNIX) {
#ifdef _AF_UNIX
				register struct T_conn_ind	*conn_ind;
				register struct T_conn_ind	*nconn_ind;
				register char			*addr;
				register struct so_so		*oso;

				/* To make sure the user sees a string rather
				 * than a dev/ino pair, we have to find the
				 * source socket structure and copy in the
				 * local (string) address.
				 */
				conn_ind = (struct T_conn_ind *)mp->b_rptr;
				addr = (caddr_t)(mp->b_rptr +
						conn_ind->SRC_offset);
				if ((oso = ux_findlink(addr,
					conn_ind->SRC_length)) == NULL) {
					freemsg(mp);
					goto rgetnext;
				}

				size = sizeof(*nconn_ind) + oso->laddr.len;
				if (MBLKLEN(mp) < size) {
					if ((bp = _s_getmblk(NULL, size))
								 == NULL) {
						_S_RECOVER(q, mp, size);
						freemsg(nbp);
						return 0;
					}
	 
					bp->b_datap->db_type = M_PROTO;

					nconn_ind = 
						(struct T_conn_ind *)bp->b_rptr;
					*nconn_ind = *conn_ind;
					freemsg(mp);
					mp = bp;
				}
				conn_ind = (struct T_conn_ind *)mp->b_rptr;
				(void)bcopy(oso->laddr.buf,
					mp->b_rptr + conn_ind->SRC_offset,
					oso->laddr.len);

				conn_ind->SRC_length = oso->laddr.len;
				mp->b_wptr = mp->b_rptr + size;
#endif
			}

			/* Save out dup'ed copy.
			 */
			nbp->b_next = so->consave;
			so->consave = nbp;

			putnext(q, mp);
			goto rgetnext;
		}	/* case T_CONN_IND */

		case T_CONN_CON: {
			register struct T_conn_con	*conn_con;
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrsrv: Got T_CONN_CON\n", 0);

			/* Pass this up only if the user is waiting-
			 * tell the write service procedure to
			 * go for it.
			 */
			so->udata.so_state &= ~SS_ISCONNECTING;
			so->udata.so_state |= SS_ISCONNECTED;

			if (so->flags & S_WRDISABLE) {
				freemsg(mp);

				/* Enable the write service queue to
				 * be scheduled, and schedule it.
				 */
				enableok(WR(q));
				qenable(WR(q));
				goto rgetnext;
			}

			conn_con = (struct T_conn_con *)mp->b_rptr;
			if (((sockaddr_t)so->raddr.buf)->sa_family == AF_UNIX) {
#ifdef _AF_UNIX
				register char		*addr;

				/* We saved the destination address, when
				 * the T_CONN_REQ was processed, so put
				 * it back.
				 */
				addr = (caddr_t)(mp->b_rptr +
						conn_con->RES_offset);
				size = sizeof(*conn_con) + so->raddr.len;
				if (MBLKLEN(mp) < size) {
					register struct T_conn_con *nconn_con;
					register mblk_t		   *bp;

					if ((bp = _s_getmblk(NULL, size))
								 == NULL) {
						_S_RECOVER(q, mp, size);
						return 0;
					}
	
					bp->b_datap->db_type = M_PROTO;

					nconn_con = 
						(struct T_conn_con *)bp->b_rptr;
					*nconn_con = *conn_con;
					freemsg(mp);
					mp = bp;
				}
				conn_con = (struct T_conn_con *)mp->b_rptr;
				addr = (caddr_t)(mp->b_rptr +
						conn_con->RES_offset);
				(void)bcopy(so->raddr.buf, 
						addr, so->raddr.len);

				conn_con->RES_length = so->raddr.len;

				mp->b_wptr = mp->b_rptr + size;
#endif
			}
			else	save_addr(&so->raddr,
						mp->b_rptr+conn_con->RES_offset,
						conn_con->RES_length);

			putnext(q, mp);
			goto rgetnext;
		}

		case T_UNITDATA_IND: {
			register struct T_unitdata_ind	*udata_ind;
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrsrv: Got T_UNITDATA_IND\n", 0);
			s = splstr();
			if (!canput(q->q_next)) {
				putbq(q, mp);
				(void)splx(s);
				return 0;
			}
			(void)splx(s);

			udata_ind = (struct T_unitdata_ind *)mp->b_rptr;
			if (((sockaddr_t)so->laddr.buf)->sa_family == AF_UNIX) {
#ifdef _AF_UNIX
				/* UNIX domain, copy useful address.
				 */
				register char 			*addr;
				register struct so_so		*oso;

				addr = (caddr_t)(mp->b_rptr +
						udata_ind->SRC_offset);

				if ((oso = ux_findlink(addr,
					  udata_ind->SRC_length)) == NULL) {
					freemsg(mp);
					goto rgetnext;
				}

				size = sizeof(*udata_ind) + oso->laddr.len;
				if (MBLKLEN(mp) < size) {
					register mblk_t		*bp;

					if ((bp = _s_getmblk(NULL, size))
								 == NULL) {
						_S_RECOVER(q, mp, size);
						return 0;
					}
					*(struct T_unitdata_ind *)bp->b_wptr =
							*udata_ind;

					bp->b_cont = mp->b_cont;
					mp->b_cont = NULL;
					freemsg(mp);
					mp = bp;
				}
				fill_udata_ind_addr(mp, oso->laddr.buf,
							oso->laddr.len);
#endif
			}
			/* Check for zero length message and ensure that
			 * we always leave one linked to the header if
			 * there is no "real" data.
			 * This facilitates sending zero length messages
			 * on dgram sockets.
			 */
			if (mp->b_cont && msgdsize(mp->b_cont) == 0) {
				register mblk_t		*bp;

				/* Zero length message.
				 */
SOCKLOG(so, SIMOD_ID, -1, 0, SL_TRACE, "sockmodrput: Got zero length msg\n", 0);
				bp = mp->b_cont;
				freemsg(bp->b_cont);
				bp->b_cont = NULL;
			}
			else	strip_zerolen(mp);

			putnext(q, mp);
			goto rgetnext;
		}

		case T_DATA_IND:
			s = splstr();
			if (canput(q->q_next)) {
				(void)splx(s);
				putnext(q, mp);
			}
			else	{
				if ((so->flags & S_RBLOCKED) == 0)
					so->flags |= S_RBLOCKED;
				putbq(q, mp);
				(void)splx(s);
				return 0;
			}

			goto rgetnext;

		case T_EXDATA_IND: {
			register mblk_t	*nmp;
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrsrv: Got T_EXDATA_IND\n", 0);
			/* If the socket is marked such that we don't
			 * want to get anymore data then free it.
			 */
			if (so->udata.so_state & SS_CANTRCVMORE) {
				freemsg(mp);
				goto rgetnext;
			}

			s = splstr();
			if (!canput(q->q_next)) {
				putbq(q, mp);
				(void)splx(s);
				return 0;
			}
			(void)splx(s);

			/* If out-of-bound data is to be taken out-of-line
			 * (default) then take it out and arrange for an
			 * empty message to be sent upstream to mark its
			 * position.
			 * Otherwise, send it up as is.
			 */
			size = sizeof(struct T_exdata_ind);
			if ((so->udata.so_options & SO_OOBINLINE) == 0) {
				/* Send up a zero length T_EXDATA_IND
				 * message to mark the position of
				 * the urgent data.
				 *
				 * The message will also generate a
				 * SIGIO if requested.
				 *
				 * The message will be discarded by
				 * strread() and by the sockets lib.
				 *
				 * Make sure that we can allocate the
				 * message block for the SIGURG as well.
				 */
				if ((nmp = _s_getmblk(NULL, size)) == NULL ||
				     (bp = _s_getmblk(NULL, size)) == NULL) {
					if (nmp)
						freemsg(nmp);
					_S_RECOVER(q, mp, size*2);
					return 0;
				}
				nmp->b_datap->db_type = M_PROTO;
				*(long *)nmp->b_wptr = T_EXDATA_IND;
				nmp->b_wptr += size;
				nmp->b_flag |= MSGMARK;
				putnext(q, nmp);

				nmp = mp->b_cont;
				if (so->oob) 
					freemsg(so->oob);
				so->oob = nmp;

				nmp->b_cont = NULL;
				freeb(mp);
			}
			else	{
				/* Allocate the SIGURG message block.
				 */
				if ((bp = _s_getmblk(NULL, size)) == NULL) {
					_S_RECOVER(q, mp, size);
					return 0;
				}
				/* Send up the OOB data.
				 */
				strip_zerolen(mp);
				mp->b_band = 0;
				mp->b_flag |= MSGMARK;
				putnext(q, mp);
			}

			/* Cause a SIGURG to be sent irrespective
			 * of whether or not the OOB data is being
			 * inline or not.
			 *
			 * The message will be discarded by
			 * strread() and by the sockets lib.
			 */
			bp->b_datap->db_type = M_PROTO;
			*(long *)bp->b_wptr = T_EXDATA_IND;
			bp->b_wptr += size;

			/* The magic incredient.
			 */
			bp->b_band = 1;
			putnext(q, bp);

			goto rgetnext;
		}

		case T_ERROR_ACK:
			ASSERT(MSGBLEN(mp) == sizeof(struct T_error_ack));
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrsrv: Got T_ERROR_ACK\n", 0);
			if (pptr->error_ack.ERROR_prim == T_CONN_RES) {
				if (so->so_conn) {
					so->so_conn->udata.so_state 
							&= ~SS_ISCONNECTED;
					so->so_conn = 0;
				}
			}

			if (pptr->error_ack.ERROR_prim == T_UNBIND_REQ &&
							so->flags & S_WUNBIND) {
				if (pptr->error_ack.TLI_error == TSYSERR)
					so->so_error = 
						pptr->error_ack.UNIX_error;
				else	so->so_error = 
					tlitosyserr(pptr->error_ack.TLI_error);

				/* The error is a result of
				 * our internal unbind request.
				 */
				so->flags &= ~S_WUNBIND;
				freemsg(mp);
				goto rgetnext;
			}

			if (pptr->error_ack.ERROR_prim == T_INFO_REQ &&
							so->flags & S_WINFO) {
				if (pptr->error_ack.TLI_error == TSYSERR)
					so->so_error = 
						pptr->error_ack.UNIX_error;
				else	so->so_error = 
					tlitosyserr(pptr->error_ack.TLI_error);

				so->flags &= ~S_WINFO;
				wakeup(so);
				freemsg(mp);
				goto rgetnext;
			}

			if (pptr->error_ack.ERROR_prim == T_DISCON_REQ) {
				freemsg(mp);
				goto rgetnext;
			}
				
			if ( !(so->flags & WAITIOCACK)) {
				putnext(q, mp);
				goto rgetnext;
			}

			ASSERT(so->iocsave != NULL);
			if (pptr->error_ack.ERROR_prim !=
					*(long *)so->iocsave->b_cont->b_rptr) {
				putnext(q, mp);
				goto rgetnext;
			}

			if (pptr->error_ack.ERROR_prim == T_BIND_REQ) {
				if (so->udata.so_options & SO_ACCEPTCONN)
					so->udata.so_options &= ~SO_ACCEPTCONN;
			}
			else
			if (pptr->error_ack.ERROR_prim == T_OPTMGMT_REQ) {
				if (so->so_option)
					so->so_option = 0;
			}

			switch (pptr->error_ack.ERROR_prim) {
			case T_OPTMGMT_REQ:
			case T_BIND_REQ:
			case T_UNBIND_REQ:
			case T_INFO_REQ:
				/* Get saved ioctl msg and set values
				 */
				iocbp = (struct iocblk *)so->iocsave->b_rptr;
				iocbp->ioc_error = 0;
				iocbp->ioc_rval = pptr->error_ack.TLI_error;
				if (iocbp->ioc_rval == TSYSERR) {
					iocbp->ioc_rval |=
						pptr->error_ack.UNIX_error << 8;
					so->so_error = 
						pptr->error_ack.UNIX_error;
				}
				else	so->so_error =
					   	tlitosyserr(iocbp->ioc_rval);

				so->iocsave->b_datap->db_type = M_IOCACK;
				putnext(q, so->iocsave);

				so->iocsave = NULL;
				so->flags &= ~WAITIOCACK;
				freemsg(mp);
				goto rgetnext;
			}	/* switch(pptr->error_ack.ERROR_prim) */ 

		case T_OK_ACK:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrsrv: Got T_OK_ACK\n", 0);
			if (pptr->ok_ack.CORRECT_prim == T_CONN_RES) {
				so->so_conn->udata.so_state |= SS_ISCONNECTED;
				so->so_conn = NULL;
			}
			if (so->flags & S_WUNBIND &&
				    pptr->ok_ack.CORRECT_prim == T_UNBIND_REQ) {
				so->flags &= ~S_WUNBIND;
				freemsg(mp);
				goto rgetnext;
			}
			/* Check for UNIX domain.
			 */
#ifdef _AF_UNIX
			if (pptr->ok_ack.CORRECT_prim == T_UNBIND_REQ &&
				((sockaddr_t)so->laddr.buf)->sa_family
							 == AF_UNIX) {
				/* Unlink it from the list.
				 */
				ux_dellink(so);
			}
#endif

			if (pptr->ok_ack.CORRECT_prim == T_DISCON_REQ) {
				/* Don't send it up.
				 */
				freemsg(mp);
				goto rgetnext;
			}

			if (so->flags & WAITIOCACK) {
				ASSERT(so->iocsave != NULL);
				if (pptr->ok_ack.CORRECT_prim !=
					*(long *)so->iocsave->b_cont->b_rptr) {
					putnext(q, mp);
					goto rgetnext;
				}
				goto out;
			}
			putnext(q, mp);
			goto rgetnext;

		case T_BIND_ACK: {
			register struct T_bind_ack	*bind_ack;
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrsrv: Got T_BIND_ACK\n", 0);
			if ( !(so->flags & WAITIOCACK)) {
				putnext(q, mp);
				goto rgetnext;
			}

			ASSERT(so->iocsave != NULL);
			if (*(long *)so->iocsave->b_cont->b_rptr != 
							T_BIND_REQ) {
				putnext(q, mp);
				goto rgetnext;
			}

			bind_ack = (struct T_bind_ack *)mp->b_rptr;
#ifdef _AF_UNIX
			if (so->laddr.len &&
			 ((sockaddr_t)so->laddr.buf)->sa_family == AF_UNIX) {
				register char			*addr;

				addr = (caddr_t)(mp->b_rptr +
						bind_ack->ADDR_offset);

				/* If we don't have a copy of the actual
				 * address bound to then save one.
				 */
				if (so->lux_dev.size == 0) {
					(void)bcopy(addr,
					(caddr_t)&so->lux_dev.addr,
						bind_ack->ADDR_length);
					so->lux_dev.size =
							bind_ack->ADDR_length;
				}

				/* UNIX domain, we have to put back the
				 * string part of the address as well as
				 * the actual address bound to.
				 */
				size = sizeof(struct bind_ux) + sizeof(*bind_ack);
				if (MBLKLEN(mp) < size) {
					register struct T_bind_ack *nbind_ack;
					register mblk_t		   *bp;

					if ((bp = _s_getmblk(NULL, size))
								 == NULL) {
						_S_RECOVER(q, mp, size);
						return 0;
					}
					bp->b_datap->db_type = M_PROTO;

					nbind_ack = 
						(struct T_bind_ack *)bp->b_wptr;
					*nbind_ack = *bind_ack;

					ux_restoreaddr(so, bp, addr,
						bind_ack->ADDR_length);
					bp->b_wptr = bp->b_rptr + size;

					freemsg(mp);
					mp = bp;
				}
				else	{
					ux_restoreaddr(so, mp, addr,
						bind_ack->ADDR_length);
					mp->b_wptr = mp->b_rptr + size;
				}
			}
#endif
			else	{
				/* Remember the bound address.
				 */
				save_addr(&so->laddr,
					mp->b_rptr+bind_ack->ADDR_offset,
					bind_ack->ADDR_length);
			}

			so->udata.so_state |= SS_ISBOUND;

			goto out;
		}

		case T_OPTMGMT_ACK:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrsrv: Got T_OPTMGMT_ACK\n", 0);
			if (so->flags & WAITIOCACK) {
				ASSERT(so->iocsave != NULL);
				if (*(long *)so->iocsave->b_cont->b_rptr != T_OPTMGMT_REQ) {
					putnext(q, mp);
					goto rgetnext;
				}
				if (so->so_option) {
					so->udata.so_options |= so->so_option;
					so->so_option = 0;
				}
				goto out;
			}
			putnext(q, mp);
			goto rgetnext;

		case T_INFO_ACK:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodrsrv: Got T_INFO_ACK\n", 0);
			if (so->flags & S_WINFO) {
				so->so_error = so_init(so,
				    (struct T_info_ack *)pptr);


				so->flags &= ~S_WINFO;
				wakeup(so);
				freemsg(mp);
				goto rgetnext;
			}

			if (so->flags & WAITIOCACK) {
				ASSERT(so->iocsave != NULL);
				if (*(long *)so->iocsave->b_cont->b_rptr != T_INFO_REQ) {
					putnext(q, mp);
					goto rgetnext;
				}
				q->q_maxpsz = pptr->info_ack.TIDU_size;
				OTHERQ(q)->q_maxpsz = pptr->info_ack.TIDU_size;
				goto out;
			}
			putnext(q, mp);
			goto rgetnext;
out:
			iocbp = (struct iocblk *)so->iocsave->b_rptr;
			ASSERT(so->iocsave->b_datap != NULL);
			so->iocsave->b_datap->db_type = M_IOCACK;
			mp->b_datap->db_type = M_DATA;
			freemsg(so->iocsave->b_cont);
			so->iocsave->b_cont = mp;
			iocbp->ioc_error = 0;
			iocbp->ioc_rval = 0;
			iocbp->ioc_count = MSGBLEN(mp);
	
			putnext(q, so->iocsave);

			so->iocsave = NULL;
			so->flags &= ~WAITIOCACK;
			goto rgetnext;
		}	/* switch(pptr->type) */
	}	/* switch(mp->b_datap->db_type) */
}

/*
 * sockmodwput - Module write queue put procedure.
 *		 Called from the module or driver
 *		 upstream.
 *		 Handles messages that must be passed
 *		 through with minimum delay and queues
 *		 the rest for the service procedure to
 *		 handle.
 */
sockmodwput(q, mp)
	register queue_t	*q;
	register mblk_t		*mp;
{
	register struct so_so		*so;
	register union T_primitives	*pptr;
	register mblk_t			*bp;
	register int			size;
	register int			s;

	ASSERT(q != NULL);
	so = (struct so_so *)q->q_ptr;
	ASSERT(so != NULL);


wputagain:
	switch(mp->b_datap->db_type) {
	default:
		putq(q, mp);
		return 0;

	case M_IOCTL: {
		register struct	iocblk 	*iocbp;

		iocbp = (struct iocblk *)mp->b_rptr;
		switch(iocbp->ioc_cmd) {
		default:
			putq(q, mp);
			return 0;

		case SI_GETINTRANSIT: {
			register int	error;
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwput: Got SI_GETINTRANSIT\n", 0);
			/* Return the peer's in-transit count.
			 */
			error = 0;
			if (so->udata.servtype != T_COTS_ORD &&
					so->udata.servtype != T_COTS)
				error = EINVAL;
			else
			if ((so->udata.so_state & SS_ISCONNECTED) == 0)
				error = ENOTCONN;
			else
			if (MSGBLEN(mp->b_cont) < sizeof(ulong))
				error = EINVAL;
			else
			if (((sockaddr_t)so->laddr.buf)->sa_family != AF_UNIX)
				error = EINVAL;

			if (error) {
				snd_IOCNAK(q, mp, error);
				return 0;
			}

SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwput: msgcnt %d\n", so->so_conn->msgcnt);
			*(int *)mp->b_cont->b_rptr = so->so_conn->msgcnt;

			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			return 0;
		}
		}
	}

	case M_FLUSH:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwput: Got M_FLUSH\n", 0);
		if (*mp->b_rptr & FLUSHW)
			flushq(q, FLUSHDATA);
		if (*mp->b_rptr & FLUSHR)
			flushq(RD(q), FLUSHDATA);

		putnext(q, mp);
		return 0;

	case M_IOCDATA:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwput: Got M_IOCDATA\n", 0);
	    	if (so->flags & NAMEPROC) {
	       		if (ti_doname(q, mp, so->laddr.buf, so->laddr.len,
		  	    so->raddr.buf, so->raddr.len) != DONAME_CONT)
		    		so->flags &= ~NAMEPROC;
			return 0;
	    	}     
	    	putnext(q, mp);
		return 0;

	case M_DATA:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwput: Got M_DATA state %x\n", so->udata.so_state);
		if ( !(so->udata.so_state & SS_ISCONNECTED)) {
			/* Set so_error, and free the message.
			 */
			so->so_error = ENOTCONN;
			freemsg(mp);
			return 0;
		}
		if (so->udata.so_state & SS_CANTSENDMORE) {
			/* Set so_error, send SIGPIPE,
			 * followed by new M_ERROR.
			 */
			freemsg(mp);
			so->so_error = EPIPE;
			snd_SIGPIPE(RD(q));
			snd_ERRORW(RD(q));
			return 0;
		}

		/* Pre-pend the M_PROTO header.
		 */
		if (so->udata.servtype == T_CLTS) {
			if (((sockaddr_t)so->raddr.buf)->sa_family == AF_UNIX) {
#ifdef _AF_UNIX
				size = sizeof(struct T_unitdata_req) +
							 so->rux_dev.size;
				if ((bp = _s_getmblk(NULL, size)) == NULL) {
					putq(q, mp);
					return 0;
				}
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwput: M_DATA: size %x\n", so->rux_dev.size);
				fill_udata_req_addr(bp,
						(caddr_t)&so->rux_dev.addr,
						so->rux_dev.size);
#endif
			}
			else	{
				/* Not UNIX domain.
				 */
				size = sizeof(struct T_unitdata_req) +
							 so->raddr.len;
				if ((bp = _s_getmblk(NULL, size)) == NULL) {
					putq(q, mp);
					return 0;
				}
				fill_udata_req_addr(bp, so->raddr.buf,
							so->raddr.len);
			}
			linkb(bp, mp);
			mp = bp;
		}

		s = splstr();
		if (so->flags & S_WBLOCKED || !canput(q->q_next)) {
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwput: canput returned false\n",0);
			if ((so->flags & S_WBLOCKED) == 0)
				so->flags |= S_WBLOCKED;
			putq(q, mp);
			(void)splx(s);
		}
		else	{
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwput: canput returned true\n",0);
			(void)splx(s);
			putnext(q, mp);
		}
		return 0;

	case M_PROTO:
		/* Assert checks if there is enough data to determine type
		 */
		ASSERT(MSGBLEN(mp) >= sizeof(long));

		pptr = (union T_primitives *)mp->b_rptr;

		switch(pptr->type) {
		default:
			putq(q, mp);
			return 0;

		case T_DATA_REQ:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwput: Got T_DATA_REQ\n", 0);
			if (so->udata.so_state & SS_CANTSENDMORE) {
				/* Set so_error and send SIGPIPE
				 */
				freemsg(mp);
				so->so_error = EPIPE;
				snd_SIGPIPE(RD(q));
				snd_ERRORW(RD(q));
				return 0;
			}
			if ( !(so->udata.so_state & SS_ISCONNECTED)) {
				/* Set so_error and free the message.
				 */
				 so->so_error = ENOTCONN;
				 freemsg(mp);
				 return 0;
			}

			s = splstr();
			if (so->flags & S_WBLOCKED || !canput(q->q_next)) {
				if ((so->flags & S_WBLOCKED) == 0)
					so->flags |= S_WBLOCKED;
				putq(q, mp);
				(void)splx(s);
			}
			else	{
				(void)splx(s);
				if (so->so_conn)
					so->so_conn->msgcnt++;
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwput: T_DATA_REQ len %d\n", MSGBLEN(mp->b_cont));
				putnext(q, mp);
			}
			return 0;

		case T_UNITDATA_REQ: {
			register struct T_unitdata_req	*udata_req;

SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwput: Got T_UNITDATA_REQ state %x\n", so->udata.so_state);
			if (so->udata.so_state & SS_CANTSENDMORE) {
				/* Set so_error, send SIGPIPE,
				 * followed by new M_ERROR.
				 */
				freemsg(mp);
				so->so_error = EPIPE;
				snd_SIGPIPE(RD(q));
				snd_ERRORW(RD(q));
				return 0;
			}
			/* If no destination address then make it look
			 * like a plain M_DATA and try again.
			 */
			udata_req = (struct T_unitdata_req *)mp->b_rptr;
			if (MSGBLEN(mp) < sizeof(*udata_req)) {
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwput: Bad unitdata header %d\n", MSGBLEN(mp));
				freemsg(mp);
				return 0;
			}
			if (udata_req->DEST_length == 0) {
				if (mp->b_cont == NULL) {
					/* Zero length message.
					 */
					mp->b_datap->db_type = M_DATA;
					mp->b_wptr = mp->b_rptr;
				}
				else	{
					bp = mp->b_cont;
					mp->b_cont = NULL;
					freemsg(mp);
					mp = bp;
				}
				goto wputagain;
			}

			s = splstr();
			if (so->flags & S_WBLOCKED || !canput(q->q_next)) {
				if ((so->flags & S_WBLOCKED) == 0)
					so->flags |= S_WBLOCKED;
				putq(q, mp);
				(void)splx(s);
			}
			else	{
				(void)splx(s);
				putnext(q, mp);
			}

			return 0;
		}	/* case T_UNITDATA_REQ: */
		}	/* case pptr_type */
	}		/* case db_type */
}

/*
 * sockmodwsrv - Module write queue service procedure.
 *		 Handles messages that the put procedure
 *		 couldn't or didn't want to handle.
 */
sockmodwsrv(q)
	register queue_t	*q;
{
	register struct so_so		*so;
 	struct iocblk			*iocbp;
	register mblk_t			*mp;
	register mblk_t			*bp;
	register union T_primitives	*pptr;
	register int			s;
	register int			size;

	ASSERT(q != NULL);
	so = (struct so_so *)q->q_ptr;
	ASSERT(so != NULL);

wgetnext:
	if ((mp = getq(q)) == NULL) {
		/* If we have been blocking downstream writes 
		 * in the put procedure, then re-enable them.
		 */
		if (so->flags & S_WBLOCKED)
			so->flags &= ~S_WBLOCKED;
		return 0;
	}

	/* If we have been disabled, and the message
	 * we have is the message which caused select
	 * to work then just free it.
	 */
	if (so->flags & S_WRDISABLE) {
		if (mp == so->bigmsg) {
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Taking big message off write queue.", 0);
			freemsg(mp);
			so->flags &= ~S_WRDISABLE;
			so->bigmsg = NULL;
			goto wgetnext;
		}
	}

again:
	switch(mp->b_datap->db_type) {
	default:
		putnext(q, mp);
		goto wgetnext;

	case M_IOCDATA:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Got M_IOCDATA\n", 0);
	    	if (so->flags & NAMEPROC) {
	       		if (ti_doname(q, mp, so->laddr.buf, so->laddr.len,
		  	    so->raddr.buf, so->raddr.len) != DONAME_CONT)
		    		so->flags &= ~NAMEPROC;
			goto wgetnext;
	    	}     
	    	putnext(q, mp);
		goto wgetnext;

	case M_DATA:

SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Got M_DATA %x bytes\n", MSGBLEN(mp));
		if (so->udata.so_state & SS_CANTSENDMORE) {
			/* Set so_error, send SIGPIPE,
			 * followed by new M_ERROR message.
			 */
			freemsg(mp);
			so->so_error = EPIPE;
			snd_SIGPIPE(RD(q));
			snd_ERRORW(RD(q));
			goto wgetnext;
		}

		/* If CLTS, pre-pend the M_PROTO header.
		 */
		if (so->udata.servtype == T_CLTS) {
			if (((sockaddr_t)so->raddr.buf)->sa_family == AF_UNIX) {
#ifdef _AF_UNIX
				size = sizeof(struct T_unitdata_req) +
							so->rux_dev.size;
				if ((bp = _s_getmblk(NULL, size)) == NULL) {
					_S_RECOVER(q, mp, size);
					return 0;
				}
				fill_udata_req_addr(bp,
						(caddr_t)&so->rux_dev.addr,
						so->rux_dev.size);
#endif
			}
			else	{
				size = sizeof(struct T_unitdata_req) +
							so->raddr.len;
				if ((bp = _s_getmblk(NULL, size)) == NULL) {
					_S_RECOVER(q, mp, size);
			       		return 0;
				}
				fill_udata_req_addr(bp, so->raddr.buf,
							 so->raddr.len);
			}
			linkb(bp, mp);
			mp = bp;
		}

		s = splstr();
		if (canput(q->q_next)) {
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: canput returned true\n", 0);
			(void)splx(s);
			putnext(q, mp);
		}
		else	{
			if ((so->flags & S_WBLOCKED) == 0)
				so->flags |= S_WBLOCKED;
			putbq(q, mp);
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: canput returned false\n", 0);
			(void)splx(s);
			return 0;
		}

		goto wgetnext;

	case M_PROTO:
		/* Assert checks if there is enough data to determine type
		 */
		ASSERT(MSGBLEN(mp) >= sizeof(long));

		pptr = (union T_primitives *)mp->b_rptr;

		switch(pptr->type) {
		default:
			putnext(q, mp);
			goto wgetnext;

		case T_UNITDATA_REQ:
		case T_DATA_REQ:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: got T_[UNIT]DATA_REQ\n", 0);
			if (so->udata.so_state & SS_CANTSENDMORE) {
				/* Set so_error, send SIGPIPE,
				 * followed by new M_ERROR message.
				 */
				freemsg(mp);
				so->so_error = EPIPE;
				snd_SIGPIPE(RD(q));
				snd_ERRORW(RD(q));
				goto wgetnext;
			}

			s = splstr();
			if (canput(q->q_next)) {
				(void)splx(s);
				if (so->so_conn)
					so->so_conn->msgcnt++;
				putnext(q, mp);
			}
			else	{
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: canput returned false\n", 0);
				if ((so->flags & S_WBLOCKED) == 0)
					so->flags |= S_WBLOCKED;
				putbq(q, mp);
				(void)splx(s);
				return 0;
			}

			goto wgetnext;

		case T_CONN_REQ: {
			struct T_conn_req		*con_req;
			register int			error;
			register struct sockaddr	*addr;
			register struct bind_ux		*bind_ux;
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Got T_CONN_REQ\n", 0);

			/* Make sure we can get an mblk large
			 * enough for any eventuality.
			 */
			size = max(sizeof(struct T_error_ack), 
					sizeof(struct T_ok_ack));
			if ((bp = _s_getmblk(NULL, size)) == NULL) {
				_S_RECOVER(q, mp, size);
				return 0;
			}

			con_req = (struct T_conn_req *)mp->b_rptr;
			if (MSGBLEN(mp) < sizeof(*con_req) ||
			    MSGBLEN(mp) < (con_req->DEST_offset +
						con_req->DEST_length)) {
				snd_ERRACK(q, bp, T_CONN_REQ, EINVAL);
				freemsg(mp);
				goto wgetnext;
			}

			addr = (sockaddr_t)(mp->b_rptr + con_req->DEST_offset);
			bind_ux = (struct bind_ux *)addr;

			/* If CLTS, we have to do the connect.
			 */
			if (so->udata.servtype == T_CLTS) {
				/* If the destination address is NULL, then
				 * dissolve the association.
				 */
				if (con_req->DEST_length == 0 ||
					addr->sa_family !=
			((sockaddr_t)so->laddr.buf)->sa_family) {
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: CLTS: Invalid address\n", 0);
					freemsg(mp);
					so->raddr.len = 0;
					so->udata.so_state &= ~SS_ISCONNECTED;
					snd_ERRACK(q, bp, T_CONN_REQ,
								EAFNOSUPPORT);
					goto wgetnext;
				}
				/* Remember the destination address.
				 */
				if (con_req->DEST_length > so->udata.addrsize) {
					snd_ERRACK(q, bp, T_CONN_REQ, EPROTO);
					freemsg(mp);
					goto wgetnext;
				}
				if (addr->sa_family == AF_UNIX) {
#ifdef _AF_UNIX
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: T_CONN_REQ(CLTS) on UNIX domain\n", 0);
					if (con_req->DEST_length !=
							sizeof(*bind_ux) ||
						bind_ux->extsize >
						   sizeof(bind_ux->extaddr)) {
						snd_ERRACK(q, bp, T_CONN_REQ,
								EINVAL);
						freemsg(mp);
						goto wgetnext;
					}

					ux_saveraddr(so, bind_ux);

SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: T_CONN_REQ(CLTS) on UNIX domain - size %x\n", so->rux_dev.size);
#endif
				}
				else	{
					/* Not UNIX domain.
					 */
					save_addr(&so->raddr, (caddr_t)addr,
						 con_req->DEST_length);
				}
				so->udata.so_state |= SS_ISCONNECTED;

				/* Now send back the T_OK_ACK
				 */
				snd_OKACK(q, bp, T_CONN_REQ);
				freemsg(mp);
				goto wgetnext;
			}

			/* COTS: 
			 * Make sure not already connecting/ed.
			 */
			error = 0;
			if (so->udata.so_state &
					(SS_ISCONNECTING|SS_ISCONNECTED)) {
				if (so->udata.so_state & SS_ISCONNECTED)
					error = EISCONN;
				else	error = EALREADY;
			}
			else
			if (con_req->DEST_length == 0 || addr->sa_family !=
			     ((sockaddr_t)so->laddr.buf)->sa_family)
				error = EAFNOSUPPORT;
			else
			if ((so->udata.so_state & SS_ISBOUND) == 0)
				error = EPROTO;

			if (error) {
				snd_ERRACK(q, bp, T_CONN_REQ, error);
				freemsg(mp);
				goto wgetnext;
			}

			/* COTS: OPT_length will be -1 if
			 * user has O_NDELAY set.
			 */
			if (con_req->OPT_length == -1) {
				register mblk_t		*nmp;

				/* Put a large enough message on my write
				 * queue to cause the stream head
				 * to block anyone doing a write, and also
				 * cause select to work as we want, i.e.
				 * to not return true until a T_CONN_CON
				 * is returned.
				 * Note that no module/driver upstream 
				 * can have a service procedure if this
				 * is to work.
				 */
				size = q->q_hiwat;
				if ((nmp = _s_getmblk(NULL, size)) == NULL) {
					_S_RECOVER(q, mp, size);
					freemsg(bp);
					return 0;
				}
	
				con_req->OPT_length = 0;
			
				nmp->b_datap->db_type = M_PROTO;
				nmp->b_wptr = nmp->b_datap->db_lim;
				so->bigmsg = nmp;
			}
			else	so->bigmsg = NULL;

			/* Check for UNIX domain.
			 */
			if (addr->sa_family == AF_UNIX) {
#ifdef _AF_UNIX
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: T_CONN_REQ(COTS) on UNIX domain\n", 0);
				if (con_req->DEST_length != sizeof(*bind_ux) ||
						bind_ux->extsize >
						   sizeof(bind_ux->extaddr)) {
					snd_ERRACK(q, bp, T_CONN_REQ, EPROTO);
					freemsg(mp);
					goto wgetnext;
				}

				/* Remember destination and
				 * adjust address.
				 */
				ux_saveraddr(so, bind_ux);

				(void)bcopy((caddr_t)&so->rux_dev.addr,
					addr, so->rux_dev.size);

				con_req->DEST_length = so->rux_dev.size;
				mp->b_wptr = mp->b_rptr + con_req->DEST_offset
						+ con_req->DEST_length;
#endif
			}

			freemsg(bp);	/* No longer needed */

			so->udata.so_state |= SS_ISCONNECTING;
			putnext(q, mp);

			if (so->bigmsg) {
				/* Prevent the write service procedure
				 * from being enabled so that the large
				 * message that we are about to put on it
				 * will not be lost. The queue will be enabled
				 * when the T_CONN_CON is received.
				 */
				so->flags |= S_WRDISABLE;
				noenable(q);

				/* Enqueue the large message
				 */
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Putting big msg %d\n", MSGBLEN(so->bigmsg));
				putq(q, so->bigmsg);
				return 0;
			}
			else	goto wgetnext;
		}	/* case T_CONN_REQ: */

		case T_CONN_RES: {
			register struct T_conn_res	*conn_res;
			register struct T_conn_ind	*conn_ind;
			register queue_t		*soq;
			register mblk_t			*pmp;
			register struct so_so		*oso;

SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Got T_CONN_RES\n", 0);
			if (MSGBLEN(mp) < sizeof(*conn_res)) {
				size = sizeof(struct T_error_ack);
				if ((bp = _s_getmblk(mp, size)) == NULL) {
					_S_RECOVER(q, mp, size);
					return 0;
				}
				else	snd_ERRACK(q, bp, T_CONN_RES, EINVAL);
				goto wgetnext;
			}

			/* We have to set the local and remote addresses
			 * for the endpoint on which the connection was
			 * accepted on. The endpoint is marked connected
			 * when the T_OK_ACK is received.
			 */
			conn_res = (struct T_conn_res *)mp->b_rptr;

			/* Find the new endpoints queue_t.
			 */
			soq = conn_res->QUEUE_ptr;
			while ((soq = soq->q_next) != NULL) {
				register struct so_so *oso;

				for (oso=so_so; oso < &so_so[so_cnt]; oso++) {
					if (!(oso->flags & USED))
						continue;
					if (soq->q_ptr == (caddr_t)oso) {
						/* Found it
						 */
						goto found;
					}
				}
			}
			if (soq == NULL) {
				/* Something wrong here 
				 * let the transport provider
				 * find it.
				 */
				putnext(q, mp);
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: No queue_t\n", 0);
				goto wgetnext;
			}
found:
			oso = (struct so_so *)soq->q_ptr;
			so->so_conn = oso;

			/* Set the local address of the
			 * new endpoint to the local address 
			 * of the endpoint on which the connect
			 * request was received.
			 */
			save_addr(&oso->laddr, so->laddr.buf, so->laddr.len);

			/* Set the peer address of the
			 * new endpoint to the source address
			 * of the connect request.
			 * We have to find the saved
			 * T_CONN_IND for this sequence number
			 * to retrieve the correct SRC address.
			 */
			pmp = NULL;
			for (bp = so->consave; bp; bp = bp->b_next) {
				conn_ind = (struct T_conn_ind *)bp->b_rptr;
				if (conn_ind->SEQ_number == 
							conn_res->SEQ_number)
					break;
				pmp = bp;
			}
			if (bp != NULL) {
				if (pmp)
					pmp->b_next = bp->b_next;
				else	so->consave = bp->b_next;
			}
			if (((sockaddr_t)so->laddr.buf)->sa_family == AF_UNIX) {
				register struct so_so		*nso;

				if ((nso = ux_findlink(bp->b_rptr +
					       conn_ind->SRC_offset,
				   	       conn_ind->SRC_length)) == NULL) {
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: UNIX: No peer\n", 0);
					oso->raddr.len = 0;
				}
				else	{
					save_addr(&oso->raddr, nso->laddr.buf,
							nso->laddr.len);

					/* Give each end of the connection
					 * a pointer to the other, so that
					 * the intransit count can work.
					 */
					oso->so_conn = nso;
					nso->so_conn = oso;
				}
			}
			else	save_addr(&oso->raddr, 
					bp->b_rptr+conn_ind->SRC_offset,
					conn_ind->SRC_length);

			freemsg(bp);

			putnext(q, mp);

			goto wgetnext;
		}	/* case T_CONN_RES */
	}	/* switch (pptr->type) */

	case M_READ:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Got M_READ\n", 0);
		freemsg(mp);

		/* If the socket is marked SS_CANTRCVMORE then
		 * send up a zero length message, to make the user
		 * get EOF. Otherwise just forget it.
		 */
		if (so->udata.so_state & SS_CANTRCVMORE)
			snd_ZERO(RD(q));

		goto wgetnext;
	
	case M_IOCTL:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Got M_IOCTL\n", 0);
		ASSERT(MSGBLEN(mp) == sizeof(struct iocblk));

		iocbp = (struct iocblk *)mp->b_rptr;
		if (so->flags & WAITIOCACK) {
			snd_IOCNAK(q, mp, EPROTO);
			goto wgetnext;
		}

		switch (iocbp->ioc_cmd) {
		default:
			putnext(q, mp);
			goto wgetnext;

		case SI_TCL_LINK: {
			register char 			*addr;
			register int			addrlen;
			register struct tcl_sictl	*tcl_sictl;

			if (((sockaddr_t)so->laddr.buf)->sa_family != AF_UNIX) {
				snd_IOCNAK(q, mp, EOPNOTSUPP);
				goto wgetnext;
			}
			if (so->udata.servtype != T_CLTS) {
				snd_IOCNAK(q, mp, EOPNOTSUPP);
				goto wgetnext;
			}

			if (mp->b_cont) {
				/* Make sure there is a peer.
				 */
				if (ux_findlink(mp->b_cont->b_rptr,
						MSGBLEN(mp->b_cont)) == NULL) {
					snd_IOCNAK(q, mp, ECONNREFUSED);
					goto wgetnext;
				}

				size = sizeof(*tcl_sictl) + MSGBLEN(mp->b_cont);
				if ((bp = _s_getmblk(NULL, size)) == NULL) {
					_S_RECOVER(q, mp, size);
					return 0;
				}
				addr = (caddr_t)mp->b_cont->b_rptr;
				addrlen = MSGBLEN(mp->b_cont);
			}
			else	{
				/* Connected, verify remote address.
				*/
				if (so->rux_dev.size == 0) {
					snd_IOCNAK(q, mp, ECONNREFUSED);
					goto wgetnext;
				}

				size = sizeof(*tcl_sictl) + so->rux_dev.size;
				if ((bp = _s_getmblk(NULL, size)) == NULL) {
					_S_RECOVER(q, mp, size);
					return 0;
				}
				addr = (caddr_t)&so->rux_dev.addr;
				addrlen = so->rux_dev.size;
			}

			tcl_sictl = (struct tcl_sictl *)bp->b_wptr;
			tcl_sictl->type = TCL_LINK;
			tcl_sictl->ADDR_len = addrlen;
			tcl_sictl->ADDR_offset = sizeof(*tcl_sictl);
			(void)bcopy(addr, bp->b_wptr + tcl_sictl->ADDR_offset,
					tcl_sictl->ADDR_len);
			bp->b_datap->db_type = M_CTL;
			bp->b_wptr += (tcl_sictl->ADDR_offset +
							tcl_sictl->ADDR_len);

			putnext(q, bp);

			iocbp->ioc_count = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			goto wgetnext;
		}

		case SI_TCL_UNLINK:
			if (((sockaddr_t)so->laddr.buf)->sa_family != AF_UNIX) {
				snd_IOCNAK(q, mp, EOPNOTSUPP);
				goto wgetnext;
			}
			if (so->udata.servtype != T_CLTS) {
				snd_IOCNAK(q, mp, EOPNOTSUPP);
				goto wgetnext;
			}

			/* Format an M_CTL and send it down.
			 */
			size = sizeof(long);
			if ((bp = _s_getmblk(NULL, size)) == NULL) {
				_S_RECOVER(q, mp, size);
				goto wgetnext;
			}
			*(long *)bp->b_wptr = TCL_UNLINK;
			bp->b_datap->db_type = M_CTL;
			bp->b_wptr += sizeof(long);
			putnext(q, bp);

			iocbp->ioc_count = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			goto wgetnext;

		case MSG_OOB:
		case MSG_PEEK:
		case MSG_OOB|MSG_PEEK: {
			register int		ilen;
			register int		olen;
			register mblk_t		*ibp;
			register mblk_t		*obp;
			register u_char		*pos;
			int			error;

			error = 0;
			if ((so->udata.so_state & SS_ISBOUND) == 0 ||
				((sockaddr_t)so->laddr.buf)->sa_family
							 != AF_INET ||
			(so->udata.servtype & (T_COTS | T_COTS_ORD)) == 0)
				error = EOPNOTSUPP;
			else
			if (mp->b_cont == NULL || 
					so->udata.so_options & SO_OOBINLINE)
				error = EINVAL;
			else
			if (so->oob == NULL)
				error = EWOULDBLOCK;

			if (error) {
				snd_IOCNAK(q, mp, error);
				goto wgetnext;
			}

			/*
			 * Process the data.
			 */
			iocbp->ioc_count = 0;
			obp = mp->b_cont;
			ibp = so->oob;
			pos = ibp->b_rptr;
			for (;;) {
				ilen = MSGBLEN(ibp);
				olen = MSGBLEN(obp);
				size = MIN(olen, ilen);
				obp->b_wptr = obp->b_rptr;

				(void)bcopy(pos, obp->b_wptr, size);

				pos += size;
				if ( !(iocbp->ioc_cmd & MSG_PEEK))
					ibp->b_rptr += size;
				obp->b_wptr += size;
				iocbp->ioc_count += size;
				ilen -= size;
				olen -= size;
				if ( !olen) {
					/* This user block is exhausted, see
					 * if there is another.
					 */
					if (obp->b_cont) {
						/* Keep going
						 */
						obp = obp->b_cont;
						continue;
					}
					/* No more user blocks, finished.
					 */
					break;
				}
				else	{
					/* This oob block is exhausted, see
					 * if there is another.
					 */
					if (ibp->b_cont) {
						ibp = ibp->b_cont;
						pos = ibp->b_rptr;
						continue;
					}
					/* No more oob data, finished.
					 */
					break;
				}
			}
			if ( !ilen && !(iocbp->ioc_cmd & MSG_PEEK)) {
				freemsg(so->oob);
				so->oob = NULL;
			}

			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			goto wgetnext;
		}

		case SI_LISTEN: {
			register struct T_bind_req	*bind_req;
			register int			error;
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Got SI_LISTEN\n", 0);

			/* If we are already bound, we have to do an
			 * unbind followed by the callers bind, in
			 * order to set the number of connect
			 * indications correctly. When we have done what
			 * we have needed to do we just change the callers
			 * ioctl type and start again.
			 */
			if ((so->udata.so_state & SS_ISBOUND) == 0) {
				/* Change it to a T_BIND_REQ and
				 * try again.
				 */
				iocbp->ioc_cmd = TI_BIND;
				so->udata.so_options |= SO_ACCEPTCONN;
				goto again;
			}
			if (iocbp->ioc_count < 
				(sizeof(*bind_req) + so->laddr.len) ||
							mp->b_cont == NULL) {
				snd_IOCNAK(q, mp, EINVAL);
				goto wgetnext;
			}

			/* Set up the T_UNBIND_REQ request.
			 */
			size = sizeof(struct T_unbind_req);
			if ((bp = _s_getmblk(NULL, size)) == NULL) {
				_S_RECOVER(q, mp, size);
				return 0;
			}

			bp->b_datap->db_type = M_PROTO;
			*(long *)bp->b_wptr = T_UNBIND_REQ;
			bp->b_wptr += sizeof(struct T_unbind_req);

			/* Set up the subsequent T_BIND_REQ.
			 */
			error = 0;
			iocbp->ioc_cmd = TI_BIND;
			bind_req = (struct T_bind_req *)mp->b_cont->b_rptr;
			if (((sockaddr_t)so->laddr.buf)->sa_family == AF_UNIX) {
#ifdef _AF_UNIX
				/* UNIX domain.
				 */
				size = so->lux_dev.size;
				if (bind_req->ADDR_length < size)
					error = EINVAL;
				else	{
					(void)bcopy((caddr_t)&so->lux_dev.addr,
				(caddr_t)bind_req + bind_req->ADDR_offset,
							size);
					bind_req->ADDR_length = size;
				}
#endif
			}
			else	{
				size = so->laddr.len;
				if (bind_req->ADDR_length < size) 
					error = EINVAL;
				else	{
					(void)bcopy(so->laddr.buf,
						(caddr_t)bind_req +
						bind_req->ADDR_offset,
						size);
					bind_req->ADDR_length = size;
				}
			}
			if (error) {
				snd_IOCNAK(q, mp, error);
				freemsg(bp);
				goto wgetnext;
			}
			/* No error, so send down the unbind.
			 */
			so->flags |= S_WUNBIND;
			putnext(q, bp);

			so->udata.so_options |= SO_ACCEPTCONN;
			goto again;
		}

		case SI_GETUDATA:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Got SI_GETUDATA\n", 0);
			if (iocbp->ioc_count < sizeof(struct si_udata) ||
							mp->b_cont == NULL) {
				snd_IOCNAK(q, mp, EINVAL);
				goto wgetnext;
			}

			(void)bcopy((caddr_t)&so->udata, mp->b_cont->b_rptr,
						sizeof(struct si_udata));
			mp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_count = sizeof(struct si_udata);
			qreply(q, mp);
			goto wgetnext;

		case TI_GETPEERNAME:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Got TI_GETPEERNAME state %x\n", so->udata.so_state);
			if ( !(so->udata.so_state & SS_ISCONNECTED)) {
				snd_IOCNAK(q, mp, ENOTCONN);
				goto wgetnext;
			}

			/* See if this is a UNIX
			 * domain endpoint.
			 */
			if (so->raddr.len &&
				((sockaddr_t)so->laddr.buf)->sa_family ==
							 AF_UNIX) {
#ifdef _AF_UNIX
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: peer len %d\n", so->raddr.len);
				so->flags |= NAMEPROC;
				if (ti_doname(q, mp, so->laddr.buf,
					so->laddr.len, so->raddr.buf,
					so->raddr.len) != DONAME_CONT) {
					so->flags &= ~NAMEPROC;
				}
				goto wgetnext;
#endif
			}
			/* See if the transport provider supports it.
			 */
			if ((bp = copymsg(mp)) == NULL) {
				snd_IOCNAK(q, mp, EAGAIN);
				goto wgetnext;
			}

			so->iocsave = mp;
			so->flags |= WAITIOCACK;

			putnext(q, bp);
			goto wgetnext;

		case TI_GETMYNAME:
if (iocbp->ioc_cmd == TI_GETMYNAME)
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Got TI_GETMYNAME state %x\n", so->udata.so_state);
			/* See if we have a copy of the address, or
			 * more importantly, see if this is a UNIX
			 * domain endpoint.
			 */
			if (so->laddr.len &&
				((sockaddr_t)so->laddr.buf)->sa_family ==
							 AF_UNIX) {
#ifdef _AF_UNIX
				so->flags |= NAMEPROC;
				if (ti_doname(q, mp, so->laddr.buf,
					so->laddr.len, so->raddr.buf,
					so->raddr.len) != DONAME_CONT) {
					so->flags &= ~NAMEPROC;
				}
				goto wgetnext;
#endif
			}

			/* See if the transport provider supports it.
			 */
			if ((bp = copymsg(mp)) == NULL) {
				snd_IOCNAK(q, mp, EAGAIN);
				goto wgetnext;
			}

			so->iocsave = mp;
			so->flags |= WAITIOCACK;

			putnext(q, bp);
			goto wgetnext;

		case SI_SETPEERNAME:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Got SI_SETPEERNAME\n", 0);
			if (iocbp->ioc_uid != 0)
				iocbp->ioc_error = EPERM;
			else
			if (so->udata.servtype != T_CLTS &&
					!(so->udata.so_state & SS_ISCONNECTED))
				iocbp->ioc_error = ENOTCONN;
			else
			if (iocbp->ioc_count == 0 || 
				iocbp->ioc_count > so->raddr.maxlen || 
						(bp = mp->b_cont) == NULL)
				iocbp->ioc_error = EINVAL;

			if (iocbp->ioc_error) {
				snd_IOCNAK(q, mp, iocbp->ioc_error);
				goto wgetnext;
			}

			so->udata.so_state |= SS_ISCONNECTED;
			save_addr(&so->raddr, bp->b_rptr, iocbp->ioc_count);

			mp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_count = 0; 
			qreply(q, mp);
			goto wgetnext;

		case SI_SETMYNAME:
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Got SI_SETMYNAME\n", 0);
			if (iocbp->ioc_uid != 0)
				iocbp->ioc_error = EPERM;
			else
			if ( !iocbp->ioc_count || 
				 !(so->udata.so_state & SS_ISBOUND) ||
				iocbp->ioc_count > so->laddr.maxlen || 
						(bp = mp->b_cont) == NULL)
				iocbp->ioc_error = EINVAL;

			if (iocbp->ioc_error) {
				snd_IOCNAK(q, mp, iocbp->ioc_error);
				goto wgetnext;
			}

			save_addr(&so->laddr, bp->b_rptr, iocbp->ioc_count);

			mp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_count = 0; 
			qreply(q, mp);
			goto wgetnext;

		case SI_SHUTDOWN: { 
			register int	how;
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Got SI_SHUTDOWN\n", 0);

			if (iocbp->ioc_count < sizeof(int) ||
							mp->b_cont == NULL)
				iocbp->ioc_error = EINVAL;

			if ((how = *(int *)mp->b_cont->b_rptr) > 2 || how < 0)
				iocbp->ioc_error = EINVAL;

SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: SI_SHUTDOWN how %d\n", how);
			if (iocbp->ioc_error) {
				snd_IOCNAK(q, mp, iocbp->ioc_error);
				goto wgetnext;
			}

			if (how == 0) {
				so->udata.so_state |= SS_CANTRCVMORE;

				/* Send an M_FLUSH(FLUSHR) message upstream.
				 */
				snd_FLUSHR(RD(q));
			}
			else
			if (how == 1) {
				so->udata.so_state |= SS_CANTSENDMORE;

				if (so->udata.servtype == T_COTS_ORD) {
					/* Send an orderly release.
					 */
					size = sizeof(struct T_ordrel_req);
					if ((bp = _s_getmblk(NULL, size))
							== NULL) {
						_S_RECOVER(q, mp, size);
						return 0;
					}

					bp->b_datap->db_type = M_PROTO;
					*(long *)bp->b_wptr = T_ORDREL_REQ;
					bp->b_wptr +=
						sizeof(struct T_ordrel_req);
					putnext(q, bp);
				}
			}
			else
			if (how == 2) {
				/* If orderly release is supported then send
				 * one, else send a disconnect.
				 */
				so->udata.so_state |= (SS_CANTRCVMORE|
							     SS_CANTSENDMORE);
				if (so->udata.servtype == T_COTS_ORD) {
					size = sizeof(struct T_ordrel_req);
					if ((bp = _s_getmblk(NULL, size))
							== NULL) {
						_S_RECOVER(q, mp, size);
						return 0;
					}
				
					bp->b_datap->db_type = M_PROTO;
					*(long *)bp->b_wptr = T_ORDREL_REQ;
					bp->b_wptr +=
						sizeof(struct T_ordrel_req);
					putnext(q, bp);
				}
				else	
				if (so->udata.servtype == T_COTS) {
					register struct T_discon_req *req;

					size = sizeof(struct T_discon_req);
					if ((bp = _s_getmblk(NULL, size))
							== NULL) {
						_S_RECOVER(q, mp, size);
						return 0;
					}

					req = (struct T_discon_req *)bp->b_wptr;
					req->PRIM_type = T_DISCON_REQ;
					req->SEQ_number = -1;

					bp->b_datap->db_type = M_PROTO;
					bp->b_wptr += sizeof(*req);
					putnext(q, bp);
				}
				/* Send an M_FLUSH(FLUSHR) message upstream.
				 */
				snd_FLUSHR(RD(q));
			}

			/* All is well, send an ioctl ACK back to the user.
			 */
			mp->b_datap->db_type = M_IOCACK;
			iocbp->ioc_count = 0;
			qreply(q, mp);

			/* Send up a zero length message if reads are
			 * to fail. Writes will prompt a response when
			 * next attempted.
			 */
			if (how == 0 || how == 2)
				snd_ZERO(RD(q));

			goto wgetnext;
		}	/* case SI_SHUTDOWN: */

		case TI_BIND:
		case TI_UNBIND:
		case TI_GETINFO:
		case TI_OPTMGMT:
			if (mp->b_cont == NULL) {
				snd_IOCNAK(q, mp, EINVAL);
				goto wgetnext;
			}
			if (!pullupmsg(mp->b_cont, -1)) {
				snd_IOCNAK(q, mp, ENOSR);
				goto wgetnext;
			}	
			if (iocbp->ioc_cmd == TI_BIND) {
				register struct T_bind_req	*bind_req;
				register struct sockaddr	*addr;

				bind_req =
					(struct T_bind_req *)mp->b_cont->b_rptr;

				if (MSGBLEN(mp->b_cont) < sizeof(*bind_req)) {
					snd_IOCNAK(q, mp, EPROTO);
					goto wgetnext;
				}
				if (MSGBLEN(mp->b_cont) < 
						(bind_req->ADDR_offset +
						 bind_req->ADDR_length)) {
					snd_IOCNAK(q, mp, EPROTO);
					goto wgetnext;
				}

				addr = (sockaddr_t)(mp->b_cont->b_rptr +
						 bind_req->ADDR_offset);

				if (bind_req->ADDR_length >= 2 &&
					addr->sa_family == AF_UNIX) {
#ifdef _AF_UNIX
					register struct bind_ux *bind_ux;

					/* Sanity check on size.
					 */
					if (bind_req->ADDR_length <
							sizeof(*bind_ux)) {
						snd_IOCNAK(q, mp, EPROTO);
						goto wgetnext;
					}
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: UNIX domain BIND\n", 0);
					/* Remember the address string
				 	 */
					bind_ux = (struct bind_ux *)addr;
					save_addr((caddr_t)&so->laddr,
						(caddr_t)addr,
						sizeof(struct sockaddr_un));

					/* If the user specified an address
					 * to bind to then save it and adjust
					 * the address so that the transport
					 * provider sees what we want it to.
					 */
					size = bind_ux->extsize;
					if (size) {
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: Non null BIND\n", 0);
						/* Non-Null bind request.
						 */
						(void)bcopy(
					(caddr_t)&bind_ux->extaddr,
						  (caddr_t)&so->lux_dev.addr,
						  size);
						so->lux_dev.size = size;

						/* Adjust destination, by moving
						 * the bind part of bind_ux
						 * to the beginning of the 
						 * address.
						 */
						(void)bcopy(
						(caddr_t)&so->lux_dev.addr,
						  	(caddr_t)bind_ux,
						  	so->lux_dev.size);

						bind_req->ADDR_length =
						 	so->lux_dev.size;
						mp->b_cont->b_wptr = 
							mp->b_cont->b_rptr +
							sizeof(*bind_req) +
							so->lux_dev.size;
					}
					else	{
						bind_req->ADDR_length = 0;
						bind_req->ADDR_offset = 0;
						mp->b_cont->b_wptr =
							mp->b_cont->b_rptr +
							sizeof(*bind_req);

						so->lux_dev.size = 0;
					}
					iocbp->ioc_count = MSGBLEN(mp->b_cont);
SOCKLOG(so, SIMOD_ID, so-so_so, 0, SL_TRACE, "sockmodwsrv: BIND length %d\n", bind_req->ADDR_length);
					/* Add it to the list of UNIX
					 * domain endpoints.
					 */
					ux_addlink(so);
#endif
				}
			}
			if (iocbp->ioc_cmd == TI_OPTMGMT) {
				register int	retval;

				/* Do any socket level options
				 * processing.
				 */
				retval = so_options(q, mp->b_cont);
				if (retval == 1) {
					mp->b_datap->db_type = M_IOCACK;
					qreply(q, mp);
					goto wgetnext;
				}
				if (retval < 0) {
					snd_IOCNAK(q, mp, -retval);
					goto wgetnext;
				}
			}

			if ((bp = copymsg(mp->b_cont)) == NULL) {
				snd_IOCNAK(q, mp, ENOSR);
				goto wgetnext;
			}

			so->iocsave = mp;
			so->flags |= WAITIOCACK;

			if (iocbp->ioc_cmd == TI_GETINFO)
				bp->b_datap->db_type = M_PCPROTO;
			else	bp->b_datap->db_type = M_PROTO;

			putnext(q, bp);
			goto wgetnext;
		}	/* switch (iocbp->ioc_cmd) */
	}	/* switch (mp->b_datap->db_type) */
}

/* Returns  -<error number> 
 *	    0 if option needs to be passed down
 *	    1 if option has been serviced
 *
 *	Should not assume the T_OPTMGMT_REQ buffer is large enough to hold
 *	the T_OPTMGMT_ACK message.
 */
STATIC int
so_options(q, mp)
	register queue_t	*q;
	register mblk_t		*mp;

{
	/* Trap the ones that we must handle directly
	 * or that we must take action on in addition
	 * to sending downstream for the TP.
	 */
	register struct T_optmgmt_req	*opt_req;
	register struct opthdr		*opt;
	register int			*optval;
	register struct so_so		*so;

	so = (struct so_so *)q->q_ptr;
	opt_req = (struct T_optmgmt_req *)mp->b_rptr;
	if (MSGBLEN(mp) < sizeof(*opt_req))
		return -EINVAL;

	opt = (struct opthdr *)(mp->b_rptr + opt_req->OPT_offset);

	if (MSGBLEN(mp) < (opt_req->OPT_length + sizeof(*opt_req)))
		return -EINVAL;

	if (opt->level != SOL_SOCKET) 
		return 0;

	switch(opt_req->MGMT_flags) {
	case T_CHECK:
		/* Retrieve current value.
		 */
		switch(opt->name) {
		case SO_DEBUG:
			*(int *)OPTVAL(opt) = so->udata.so_options & SO_DEBUG;
			opt_req->PRIM_type = T_OPTMGMT_ACK;
			opt->len = sizeof(int);
			return 1;

		case SO_ERROR:
			*(int *)OPTVAL(opt) = so->so_error;
			opt_req->PRIM_type = T_OPTMGMT_ACK;
			opt->len = sizeof(int);
			so->so_error = 0;
			return 1;

		case SO_OOBINLINE:
			*(int *)OPTVAL(opt) = 
					so->udata.so_options & SO_OOBINLINE;
			opt_req->PRIM_type = T_OPTMGMT_ACK;
			opt->len = sizeof(int);
			return 1;

		default:
			return 0;
		}
		break;

	case T_NEGOTIATE:
		/* Set new value.
		 * if value is non-zero, then the option should
		 * be set, otherwise it is reset.
		 */
		switch(opt->name) {
		case SO_DEBUG:
		case SO_ERROR:
		case SO_OOBINLINE:
		case SO_SNDBUF:
		case SO_RCVBUF:
			if (opt->len != OPTLEN(sizeof(int)))
				return -EINVAL;

			optval = (int *)OPTVAL(opt);

			switch (opt->name) {
			case SO_DEBUG:
				if (*optval)
					so->udata.so_options |= SO_DEBUG;
				else	so->udata.so_options &= ~SO_DEBUG;
				return 0;

			case SO_ERROR:
				so->so_error = *optval;
				return 1;
	
			case SO_OOBINLINE:
				if (*optval)
					so->udata.so_options |=  SO_OOBINLINE; 
				else	so->udata.so_options &= ~SO_OOBINLINE;
				opt_req->PRIM_type = T_OPTMGMT_ACK;
				return 1;
	
			case SO_SNDBUF:
				/* Never allow it above the max.
				 */
				q->q_maxpsz = so->udata.tidusize =
						MIN(*optval * 2, SB_MAX);	
				return 0;
	
			case SO_RCVBUF:
				RD(q)->q_maxpsz = MIN(*optval * 2, SB_MAX);	
				return 0;
			}
		default:
			return 0;
		}
	}
	return 0;
}

/*
 * Set sizes of buffers
 */
#define	DEFSIZE	128
STATIC long
_t_setsize(infosize)
	long	infosize;
{
	switch(infosize)
	{
		case -1:
			return DEFSIZE;
		case -2:
			return 0;
		default:
			return infosize;
	}
}

/*
 * Translate a TLI error into a system error as best we can.
 */
STATIC ushort tli_errs[] = {
	      0,		/* no error	 */
	      EADDRNOTAVAIL,    /* TBADADDR      */
	      ENOPROTOOPT,      /* TBADOPT       */
	      EACCES,		/* TACCES	 */
	      EBADF,		/* TBADF	 */
	      EADDRNOTAVAIL,	/* TNOADDR       */
	      EPROTO,		/* TOUTSTATE     */
	      EPROTO,		/* TBADSEQ       */
	      0,		/* TSYSERR - will never get */
	      EPROTO,		/* TLOOK - should never be sent by transport */
	      EMSGSIZE,		/* TBADDATA      */
	      EMSGSIZE,		/* TBUFOVFLW     */
	      EPROTO,		/* TFLOW	 */
	      EWOULDBLOCK,      /* TNODATA       */
	      EPROTO,		/* TNODIS	 */
	      EPROTO,		/* TNOUDERR      */
	      EINVAL,		/* TBADFLAG      */
	      EPROTO,		/* TNOREL	 */
	      EOPNOTSUPP,       /* TNOTSUPPORT   */
	      EPROTO,		/* TSTATECHNG    */
};
 
STATIC int
tlitosyserr(terr)
	register int	terr;
{
	if (terr > (sizeof(tli_errs) / sizeof(ushort)))
		return EPROTO;
	else	return tli_errs[terr];
}

/* This function will walk through the message block given
 * looking for a single data block large enough to hold 
 * size bytes. If it finds one it will free the surrounding
 * blocks and return a pointer to the one of the appropriate
 * size. If no component of the passed in message is large enough,
 * then if the system can't provide one of suitable size the
 * passed in message block is untouched. If the system can provide
 * one then the passed in message block is freed.
 */
STATIC mblk_t *
_s_getmblk(mp, size)
	register mblk_t		*mp;
	register int		size;
{
	register mblk_t		*nmp;
	register mblk_t		*bp;

	bp = mp;
	while (bp) {
		if (MBLKLEN(bp) >= size) {
			bp->b_rptr = bp->b_wptr = bp->b_datap->db_base;
			while (mp && bp != mp) {
				/* Free each block up to the one
				 * we want.
				 */
				nmp = mp->b_cont;
				freeb(mp);
				mp = nmp;
			}
			if (bp->b_cont) {
				/* Free each block after the one
				 * we want.
				 */
				nmp = bp->b_cont;
				freemsg(nmp);
				bp->b_cont = 0;
			}
			return bp;
		}
		bp = bp->b_cont;

	}
	if ((bp =  allocb(size, BPRI_MED)) == NULL) {
		/* But we have not touched mp.
		 */
SOCKLOG((struct so_so *)NULL, SIMOD_ID, -1, 0, SL_TRACE, "_s_getmblk: No memory\n", 0);
		return NULL;
	}
	else	{
SOCKLOG((struct so_so *)NULL, SIMOD_ID, -1, 0, SL_TRACE, "_s_getmblk: Allocated %d bytes\n", size);
		freemsg(mp);
		return bp;
	}
}

STATIC void
snd_ERRACK(q, bp, prim, serr)
	register queue_t	*q;
	register mblk_t 	*bp;
	register int		serr;
	register int		prim;
{
	register struct T_error_ack	*tea;
	register struct so_so		*so;
 
	so = (struct so_so *)q->q_ptr;
 
	tea = (struct T_error_ack *) bp->b_rptr;
	bp->b_wptr += sizeof(struct T_error_ack);
	bp->b_datap->db_type = M_PCPROTO;
	tea->ERROR_prim = prim;
	tea->PRIM_type = T_ERROR_ACK;
	tea->TLI_error = TSYSERR;
	tea->UNIX_error = serr;
	qreply(q, bp);

	so->so_error = serr;
}
 
STATIC void
snd_OKACK(q, mp, prim)
	register queue_t	*q;
	register mblk_t		*mp;
	register int		prim;
{
	register struct T_ok_ack	*ok_ack;
 
	mp->b_datap->db_type = M_PCPROTO;
	ok_ack = (struct T_ok_ack *)mp->b_rptr;
	mp->b_wptr += sizeof(struct T_ok_ack);
	ok_ack->CORRECT_prim = prim;
	ok_ack->PRIM_type = T_OK_ACK;
	qreply(q, mp);
}
 

STATIC 
so_init(so, info_ack)
	register struct so_so		*so;
	register struct T_info_ack	*info_ack;
{
	/* Common stuff.
	 */
	so->udata.servtype = info_ack->SERV_type;
	so->udata.tidusize = so->tp_info.tsdu =
		_t_setsize(info_ack->TIDU_size);
	
	so->udata.addrsize = so->tp_info.addr =
		_t_setsize(info_ack->ADDR_size);
	
	so->udata.optsize = so->tp_info.options = 
		_t_setsize(info_ack->OPT_size);

	switch(info_ack->SERV_type) {
	case T_CLTS:
		switch(info_ack->CURRENT_state) {
		case TS_UNBND:
			so->udata.so_state = 0;
			so->udata.so_options = 0;
			break;

		case TS_IDLE:
			so->udata.so_state |= SS_ISBOUND;
			so->udata.so_options = 0;
			break;

		default:
			return EINVAL;
		}
		break;

	case T_COTS:
	case T_COTS_ORD:
		switch(info_ack->CURRENT_state) {
		case TS_UNBND:
			so->udata.so_state = 0;
			so->udata.so_options = 0;
			break;

		case TS_IDLE:
			so->udata.so_state |= SS_ISBOUND;
			so->udata.so_options = 0;
			break;

		case TS_DATA_XFER:
			so->udata.so_state |= (SS_ISBOUND|SS_ISCONNECTED);
			so->udata.so_options = 0;
			break;

		default:
			return EINVAL;
		}
		break;

	default:
		return EINVAL;
	}
	return 0;
}

STATIC void
strip_zerolen(mp)
	register mblk_t	*mp;
{
	register mblk_t *bp;

	/* Assumes the first mblk is never zero length,
	 * and is actually some kind of header.
	 */
	for (bp = mp, mp = mp->b_cont; mp && mp->b_cont; mp = mp->b_cont) {
		if ( MSGBLEN(mp) == 0) {
			bp->b_cont = mp->b_cont;
			mp->b_cont = NULL;
			freeb(mp);
			mp = bp;
		}
		else	bp = mp;
	}
}

STATIC void
save_addr(save, buf, len)
	register struct netbuf	*save;
	register char		*buf;
	register int		len;
{
	register int llen;

	llen = min(save->maxlen, len);
SOCKLOG((struct so_so *)NULL, SIMOD_ID, -1, 0, SL_TRACE, "save_addr: Copying %d bytes\n", llen);
	(void)bcopy(buf, save->buf, llen);
	save->len = llen;
}

STATIC void
snd_ZERO(q)
	register queue_t	*q;
{
	register mblk_t		*mp;

	if ((mp = _s_getmblk(NULL, 1)) == NULL) {
		if (!bufcall(1, BPRI_MED, snd_ZERO, (caddr_t)q))
			(void)timeout(snd_ZERO, (caddr_t)q, SIMWAIT);
	}
	else	{
SOCKLOG((struct so_so *)NULL, SIMOD_ID, -1, 0, SL_TRACE, "snd_ZERO: Sending up zero length msg\n",0);
		putnext(q, mp);
	}
}

STATIC void
snd_ERRORW(q)
	register queue_t	*q;
{
	register mblk_t		*mp;

	if ((mp = _s_getmblk(NULL, 2)) == NULL)
		if (!bufcall(2, BPRI_MED, snd_ERRORW, (caddr_t)q))
			(void)timeout(snd_ERRORW, (caddr_t)q, SIMWAIT);

	mp->b_datap->db_type = M_ERROR;
#ifdef noerror
	*mp->b_wptr++ = NOERROR;
#endif
	*mp->b_wptr++ = EPIPE;
SOCKLOG((struct so_so *)NULL, SIMOD_ID, -1, 0, SL_TRACE, "snd_ERRORW: Sending up M_ERROR\n",0);
	putnext(q, mp);
}
 
STATIC void
snd_SIGPIPE(q)
	register queue_t	*q;
{
	if (putctl1(q, M_PCSIG, SIGPIPE) == 0)
		if (!bufcall(1, BPRI_MED, snd_SIGPIPE, (caddr_t)q))
			(void)timeout(snd_SIGPIPE, (caddr_t)q, SIMWAIT);
}

STATIC void
snd_FLUSHR(q)
	register queue_t	*q;
{
	if (putctl1(q, M_FLUSH, FLUSHR) == 0)
		if (!bufcall(1, BPRI_MED, snd_FLUSHR, (caddr_t)q))
			(void)timeout(snd_FLUSHR, (caddr_t)q, SIMWAIT);
}

STATIC void
snd_IOCNAK(q, mp, error)
	register queue_t	*q;
	register mblk_t		*mp;
	register int		error;
{
	register struct iocblk	*iocbp;
	register struct so_so	*so;

	mp->b_datap->db_type = M_IOCNAK;

	iocbp = (struct iocblk *)mp->b_rptr;
	so = (struct so_so *)q->q_ptr;
	iocbp->ioc_error = so->so_error = error;
	iocbp->ioc_count = 0;

 	qreply(q, mp);
}

#ifdef _AF_UNIX
/* Looks up the socket structure which has as
 * its local dev/ino the same as passed in.
 */
STATIC struct so_so *
ux_findlink(addr, len)
	register char		*addr;
	register int		len;
{
	register struct so_so 	*so;

	for (so = so_ux_list; so != NULL; so = so->so_ux.next) {
		if (bcmp(addr, (caddr_t)&so->lux_dev.addr, len) == 0)
			return so;
	}
	return NULL;
}

STATIC void
ux_dellink(so)
	register struct so_so	*so;
{
	register struct so_so	*oso;

	if (oso = so->so_ux.next)
		oso->so_ux.prev = so->so_ux.prev;

	if (oso = so->so_ux.prev)
		oso->so_ux.next = so->so_ux.next;
	else	so_ux_list = so->so_ux.next;
}

STATIC void
ux_addlink(so)
	register struct so_so	*so;
{
	so->so_ux.next = so_ux_list;
	so->so_ux.prev = NULL;
	if (so_ux_list)
		so_ux_list->so_ux.prev = so;
	so_ux_list = so;
}

/* When a T_BIND_ACK is received, copy back
 * both parts of the address into the right
 * places for the user.
 */
STATIC void
ux_restoreaddr(so, mp, addr, addrlen)
	register struct so_so		*so;
	register mblk_t			*mp;
	register char			*addr;
	register int			addrlen;
{
	struct T_bind_ack		*bind_ack;
	struct bind_ux			*bind_ux;

	bind_ack = (struct T_bind_ack *)mp->b_rptr;
	bind_ux = (struct bind_ux *)(mp->b_rptr + bind_ack->ADDR_offset);

	/* Copy address actually bound to.
	 */
	(void)bcopy(addr, (caddr_t)&bind_ux->extaddr, addrlen);
	bind_ux->extsize = addrlen;

	/* Copy address the user thought was bound to.
	 */
	(void)bzero((caddr_t)&bind_ux->name, sizeof(bind_ux->name));
	(void)bcopy(so->laddr.buf, (caddr_t)&bind_ux->name,
			so->laddr.len);
	bind_ack->ADDR_length = sizeof(*bind_ux);
}

/* In a T_CONN_REQ, save both parts
 * of the address.
 */
STATIC void
ux_saveraddr(so, bind_ux)
	register struct so_so		*so;
	register struct bind_ux		*bind_ux;
{
	save_addr((caddr_t)&so->raddr, (caddr_t)&bind_ux->name,
		sizeof(struct sockaddr_un));

	(void)bcopy((caddr_t)&bind_ux->extaddr,
		(caddr_t)&so->rux_dev.addr, bind_ux->extsize);
	so->rux_dev.size = bind_ux->extsize;
}

/* Fill in a T_UNITDATA_REQ address
 */
STATIC void
fill_udata_req_addr(bp, addr, len)
	register mblk_t			*bp;
	register char			*addr;
	register int			len;
{
	register struct T_unitdata_req	*udata_req;

	udata_req = (struct T_unitdata_req *)bp->b_rptr;
	udata_req->DEST_length = len;
	udata_req->DEST_offset = sizeof(*udata_req);
	(void)bcopy(addr, bp->b_rptr + udata_req->DEST_offset, len);

	udata_req->PRIM_type = T_UNITDATA_REQ;
	udata_req->OPT_length = 0;
	udata_req->OPT_offset = 0;

	bp->b_datap->db_type = M_PROTO;
	bp->b_wptr = bp->b_rptr + sizeof(*udata_req) + len;
}

/* Fill in a T_UNITDATA_IND address
 */
STATIC void
fill_udata_ind_addr(bp, addr, len)
	register mblk_t			*bp;
	register char			*addr;
	register int			len;
{
	register struct T_unitdata_ind	*udata_ind;

	udata_ind = (struct T_unitdata_ind *)bp->b_rptr;
	udata_ind->SRC_length = len;
	udata_ind->SRC_offset = sizeof(*udata_ind);
	(void)bcopy(addr, bp->b_rptr + udata_ind->SRC_offset, len);

	bp->b_datap->db_type = M_PROTO;
	bp->b_wptr = bp->b_rptr + sizeof(*udata_ind) + len;
}
#endif
