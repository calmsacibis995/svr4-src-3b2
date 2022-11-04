/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)krpc:krpc/clnt_clts.c	1.6"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)clnt_clts.c 1.3 89/01/11 SMI"
#endif

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

#ifdef	_KERNEL
/*
 * Implements a kernel based, client side RPC.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
/*#include <net/if.h>*/
/*#include <net/route.h>*/
/*#include <netinet/in.h>*/
/*#include <netinet/in_pcb.h>*/
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <sys/file.h>
#include <rpc/rpc_msg.h>
#include <sys/stream.h>
#include <sys/strsubr.h>
#include <sys/tiuser.h>
#include <sys/tihdr.h>
#include <sys/t_kuser.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/kmem.h>
#include <sys/debug.h>

int		ckuwakeup();
void		clnt_clts_init();

enum clnt_stat	clnt_clts_kcallit();
void		clnt_clts_kabort();
void		clnt_clts_kerror();
STATIC bool_t	clnt_clts_kfreeres();
bool_t		clnt_clts_kcontrol();
void		clnt_clts_kdestroy();

/*
 * Operations vector for UDP/IP based RPC
 */
static struct clnt_ops udp_ops = {
	clnt_clts_kcallit,	/* do rpc call */
	clnt_clts_kabort,	/* abort call */
	clnt_clts_kerror,	/* return error status */
	clnt_clts_kfreeres,	/* free results */
	clnt_clts_kdestroy,	/* destroy rpc handle */
	clnt_clts_kcontrol	/* the ioctl() of rpc */
};

/*
 * Private data per rpc handle.  This structure is allocated by
 * clnt_clts_kcreate, and freed by clnt_clts_kdestroy.
 */
struct cku_private {
	u_int			 cku_flags;	/* see below */
	CLIENT			 cku_client;	/* client handle */
	int			 cku_retrys;	/* request retrys */
	TIUSER 			*cku_tiptr;	/* open tli file pointer */
	struct netbuf		 cku_addr;	/* remote address */
	struct rpc_err		 cku_err;	/* error status */
	XDR			 cku_outxdr;	/* xdr routine for output */
	XDR			 cku_inxdr;	/* xdr routine for input */
	u_int			 cku_outpos;	/* position of in output mbuf */
	char			*cku_outbuf;	/* output buffer */
	char			*cku_inbuf;	/* input buffer */
	struct t_kunitdata	*cku_inudata;	/* input tli buf */
	struct cred		*cku_cred;	/* credentials */
	struct rpc_timers	*cku_timers;	/* for estimating RTT */
	struct rpc_timers	*cku_timeall;	/* for estimating RTT */
	void			 (*cku_feedback)();
	caddr_t			 cku_feedarg;	/* argument for feedback func */
	u_long			 cku_xid;	/* current XID */
	frtn_t			 cku_frtn;	/* message free routine */
};

struct {
	int	rccalls;
	int	rcbadcalls;
	int	rcretrans;
	int	rcbadxids;
	int	rctimeouts;
	int	rcwaits;
	int	rcnewcreds;
	int	rcbadverfs;
	int	rctimers;
	int	rctoobig;/*cpj*/
	int	rcnomem;/*cpj*/
	int	rccantsend;/*cpj*/
	int	rcbufbusy1;/*cpj*/
	int	rcbufbusy2;/*cpj*/
	int	rcbufulocks;
	int	kludge1;
	int	kludge2;
} rcstat;


#define	ptoh(p)		(&((p)->cku_client))
#define	htop(h)		((struct cku_private *)((h)->cl_private))

/* cku_flags */
#define	CKU_TIMEDOUT	0x001
#define	CKU_BUSY	0x002
#define	CKU_WANTED	0x004
#define	CKU_BUFBUSY	0x008
#define	CKU_BUFWANTED	0x010

/* Times to retry 
*/
#define	RECVTRIES	2
#define	SNDTRIES	4


int	clnt_clts_xid;		/* transaction id used by all clients */

STATIC void
buffree(p)
	struct cku_private *p;
{
	p->cku_flags &= ~CKU_BUFBUSY;
	if (p->cku_flags & CKU_BUFWANTED) {
		p->cku_flags &= ~CKU_BUFWANTED;
		rcstat.rcbufulocks++;
		wakeup((caddr_t)&p->cku_outbuf);
	}
}

/*
 * Create an rpc handle for a clts rpc connection.
 * Allocates space for the handle structure and the private data, and
 * opens a socket.  Note sockets and handles are one to one.
 */
/*ARGSUSED*/
CLIENT *
clnt_clts_kcreate(tiptr, addr, pgm, vers, sendsz, recvsz, retrys, cred)
register TIUSER *tiptr;
struct netbuf *addr;
u_long	pgm;
u_long	vers;
int	retrys;
struct	cred *cred;
u_int	sendsz, recvsz;
{
	register CLIENT *h;
	register struct cku_private *p;
	struct	 rpc_msg call_msg;

#ifdef RPCDEBUG
	printf("clnt_clts_kcreate(%d, %d, %d\n", pgm, vers, retrys);
#endif
	p = (struct cku_private *)kmem_zalloc(sizeof(*p), KM_SLEEP);
	h = ptoh(p);

	if (!clnt_clts_xid) {
/*
		clnt_clts_xid = hrestime.tv_usec;
*/
		clnt_clts_xid = lbolt;
		
	}

	/* handle */
	h->cl_ops = &udp_ops;
	h->cl_private = (caddr_t) p;
	h->cl_auth = authkern_create();

	/* call message, just used to pre-serialize below */
	call_msg.rm_xid = 0;
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = pgm;
	call_msg.rm_call.cb_vers = vers;

	/* private */
	clnt_clts_init(h, addr, retrys, cred);
	p->cku_outbuf = (char *)kmem_alloc((u_int)UDPMSGSIZE, KM_SLEEP);
	xdrmem_create(&p->cku_outxdr, p->cku_outbuf, UDPMSGSIZE, XDR_ENCODE);

	/* pre-serialize call message header */
	if (! xdr_callhdr(&(p->cku_outxdr), &call_msg)) {
		printf("clnt_clts_kcreate - Fatal header serialization error.");
		goto bad;
	}
	p->cku_outpos = XDR_GETPOS(&(p->cku_outxdr));
	p->cku_tiptr = tiptr; 
	return (h);

bad:
	kmem_free((caddr_t)p->cku_outbuf, (u_int)UDPMSGSIZE);
	kmem_free((caddr_t)p, (u_int)sizeof (struct cku_private));
#ifdef RPCDEBUG
	printf("create failed\n");
#endif
	return ((CLIENT *)NULL);
}

void
clnt_clts_init(h, addr, retrys, cred)
CLIENT   *h;
struct	 netbuf *addr;
register int retrys;
struct 	 cred *cred;
{
	/* LINTED pointer alignment */
	struct cku_private *p = htop(h);

	p->cku_retrys = retrys;

	p->cku_addr.buf = (char *)kmem_zalloc(addr->maxlen, KM_SLEEP);
	p->cku_addr.len = addr->len;
	p->cku_addr.maxlen = addr->len;
	bcopy(addr->buf, p->cku_addr.buf, addr->len);

	p->cku_cred = cred;
	p->cku_xid = 0;
	p->cku_flags &= (CKU_BUFBUSY | CKU_BUFWANTED);
}

/*
 * set the timers.  Return current retransmission timeout.
 */
clnt_clts_settimers(h, t, all, minimum, feedback, arg)
CLIENT   *h;
struct   rpc_timers *t, *all;
unsigned int minimum;
void     (*feedback)();
caddr_t  arg;
{
	/* LINTED pointer alignment */
	struct cku_private *p = htop(h);
	int value;

	p->cku_feedback = feedback;
	p->cku_feedarg = arg;
	p->cku_timers = t;
	p->cku_timeall = all;
	p->cku_xid = clnt_clts_xid++;
	value = all->rt_rtxcur;
	value += t->rt_rtxcur;
	if (value < minimum)
		return(minimum);
	rcstat.rctimers++;
	return(value);
}

/*
 * Time out back off function. tim is in HZ
 */
#define MAXTIMO	(20 * HZ)
#define backoff(tim)	((((tim) << 1) > MAXTIMO) ? MAXTIMO : ((tim) << 1))

/*
 * Call remote procedure.
 * Most of the work of rpc is done here.  We serialize what is left
 * of the header (some was pre-serialized in the handle), serialize
 * the arguments, and send it off.  We wait for a reply or a time out.
 * Timeout causes an immediate return, other packet problems may cause
 * a retry on the receive.  When a good packet is received we deserialize
 * it, and check verification.  A bad reply code will cause one retry
 * with full (longhand) credentials.
 */

enum clnt_stat
clnt_clts_kcallit_addr(h, procnum, xdr_args, argsp, xdr_results, resultsp, wait,
		sin)
register  CLIENT *h;
u_long	  procnum;
xdrproc_t xdr_args;
caddr_t	  argsp;
xdrproc_t xdr_results;
caddr_t	  resultsp;
struct    timeval wait;
struct    netbuf *sin;
{
	/* LINTED pointer alignment */
	register struct cku_private *p = htop(h);
	register XDR *xdrs;
	register TIUSER *tiptr = p->cku_tiptr;
	int	 rtries;
	int	 stries = p->cku_retrys;
	int	 s;
	struct   cred *tmpcred;
	int	timohz, ret;
	u_long	xid;
	u_int	rempos = 0;
	int	refreshes = 2;	/* number of times to refresh credential */
	int	round_trip;	/* time the RPC */
	struct	t_kunitdata *unitdata;
	int	flags;

/*
# define time_in_hz (hrestime.tv_sec*hz + hrestime.tv_usec/(1000000/hz))
*/
#define time_in_hz lbolt

#ifdef RPCDEBUG
	printf("cku_callit\n");
#endif
	rcstat.rccalls++;

	while (p->cku_flags & CKU_BUSY) {
		rcstat.rcwaits++;
		p->cku_flags |= CKU_WANTED;
		(void) sleep((caddr_t)h, PZERO-2);
	}
	p->cku_flags |= CKU_BUSY;

	/*
	 * Set credentials into the u structure
	 */
	tmpcred = u.u_procp->p_cred;
	u.u_procp->p_cred = p->cku_cred;

	if (p->cku_xid == 0)
		xid = clnt_clts_xid++;
	else
		xid = p->cku_xid;

	/*
	 * This is dumb but easy: keep the time out in units of hz
	 * so it is easy to call timeout and modify the value.
	 */
	timohz = wait.tv_sec * HZ + (wait.tv_usec * HZ) / 1000000;

call_again:

	/*
	 * Wait til buffer gets freed then make a type 2 mbuf point at it
	 * The buffree routine clears CKU_BUFBUSY and does a wakeup when
	 * the mbuf gets freed.
	 */
	s = splstr();
	while (p->cku_flags & CKU_BUFBUSY) {
		p->cku_flags |= CKU_BUFWANTED;
		rcstat.rcbufbusy1++;
		/*
		 * This is a kludge to avoid deadlock in the case of a
		 * the server to free the mbuf while the server is blocked
		 * waiting for the client to free the reply mbuf.  Avoid this
		 * by flushing the input queue every once in a while while
		 * we are waiting.
		 */
		if (cpjsleep((caddr_t)&p->cku_outbuf, PZERO-3, 10*100)<0){
		printf("cpjsleep: kludge1 buffer not sent in 10 secs\n");
		rcstat.kludge1++;
		buffree(p); /*bogus*/
		}
	 }
	 p->cku_flags |= CKU_BUFBUSY;
	 (void) splx(s);

	xdrs = &p->cku_outxdr;
	/*
	 * The transaction id is the first thing in the
	 * preserialized output buffer.
	 */
	/* LINTED pointer alignment */
	(*(u_long *)(p->cku_outbuf)) = xid;

	xdrmem_create(xdrs, p->cku_outbuf, UDPMSGSIZE, XDR_ENCODE);

	if (rempos != 0) {
		XDR_SETPOS(xdrs, rempos);
	} else {
		/*
		 * Serialize dynamic stuff into the output buffer.
		 */
		XDR_SETPOS(xdrs, p->cku_outpos);
		if ((! XDR_PUTLONG(xdrs, (long *)&procnum)) ||
		    (! AUTH_MARSHALL(h->cl_auth, xdrs)) ||
		    (! (*xdr_args)(xdrs, argsp))) {
			p->cku_err.re_status = RPC_CANTENCODEARGS;
			p->cku_err.re_errno = EIO;
			goto done;
		}
		rempos = XDR_GETPOS(xdrs);
	}

	round_trip = time_in_hz;
	if ((unitdata = (struct t_kunitdata *)t_kalloc(tiptr, T_UNITDATA, 
						/* LINTED pointer alignment */
						T_UDATA|T_ADDR)) == (struct t_kunitdata *)NULL) {
		rcstat.rcnomem++;
		buffree(p);     /*cpj*/
		goto done;
	}
	
	bcopy(p->cku_addr.buf, unitdata->addr.buf, unitdata->addr.maxlen);
	unitdata->addr.len = unitdata->addr.maxlen;
 
	unitdata->udata.buf = p->cku_outbuf;
	if (rempos >unitdata->udata.maxlen) {
		printf("clnt_clts_kcallit_addr: buffer too long, need %d, maxlen %d\n",rempos, unitdata->udata.maxlen);
		rcstat.rctoobig++;
		if (t_kfree(tiptr, unitdata, T_UNITDATA) < 0)
			printf("clnt_clts_kcallit_addr: t_kfree: error %d\n", u.u_error);
		buffree(p);
		goto done;
	}        
	unitdata->udata.len = rempos;
 
	p->cku_frtn.free_func = buffree;
	p->cku_frtn.free_arg = (char *)p;
 
	if ((ret = t_ksndudata(tiptr, unitdata, &p->cku_frtn)) < 0) {
		p->cku_err.re_status = RPC_CANTSEND;
		p->cku_err.re_errno = u.u_error;
		printf("clnt_clts_kcallit_addr: t_ksndudata: error %d\n", u.u_error); /*cpj*/
		if (t_kfree(tiptr, unitdata, T_UNITDATA) < 0)
			printf ("clnt_clts_kcallit_addr: t_kfree: error %d\n", u.u_error);
		rcstat.rccantsend++;
		buffree(p);	/*cpj*/
		goto done;
	}
	if (t_kfree(tiptr, unitdata, T_UNITDATA) < 0)
		printf("clnt_clts_kcallit_addr: t_kfree: error %d\n", u.u_error);

	for (rtries = RECVTRIES; rtries; rtries--) {
		if ((unitdata = (struct t_kunitdata *)t_kalloc(tiptr, T_UNITDATA, 
						/* LINTED pointer alignment */
						T_UDATA|T_ADDR)) == (struct t_kunitdata *)NULL) {
			goto done;
		}
		if ((ret = t_kspoll(tiptr, timohz, READWAIT)) < 0) {
			if (u.u_error == EINTR) {
				p->cku_err.re_status = RPC_INTR;
				p->cku_err.re_errno = EINTR;
				goto done;
			}
			continue;       /* is this correct? */
		}
		if (ret == 0) {
			p->cku_err.re_status = RPC_TIMEDOUT;
			p->cku_err.re_errno = ETIMEDOUT;
			rcstat.rctimeouts++;
			goto done;
		}

		/* something waiting, so read it in
		 */
		if (t_krcvudata(tiptr, unitdata, &flags) < 0) {
			goto done;
		}
		if (sin) {
			bcopy(unitdata->addr.buf, sin->buf, unitdata->addr.len);
			sin->len = unitdata->addr.len;
		}
		p->cku_inudata = unitdata;
 
		p->cku_inbuf = unitdata->udata.buf;

		if (p->cku_inudata->udata.len < sizeof (u_long)) {
			printf("clnt_clts_kcallit_addr: len too small %d\n", p->cku_inudata->udata.len);
			t_kfree(tiptr, p->cku_inudata, T_UNITDATA);
			continue;
		}

		/*
		 * If reply transaction id matches id sent
		 * we have a good packet.
		 */
		/* LINTED pointer alignment */
		if (*((u_long *)(p->cku_inbuf)) != *((u_long *)(p->cku_outbuf))) {
			rcstat.rcbadxids++;
			if (t_kfree(tiptr, p->cku_inudata, T_UNITDATA) < 0)
			        printf("clnt_clts_kcallit_addr: t_kfree: %d\n", u.u_error);
			continue;
		}
		break;
	}

	if (rtries == 0) {
		p->cku_err.re_status = RPC_CANTRECV;
		p->cku_err.re_errno = EIO;
		goto done;
	}

	round_trip = time_in_hz - round_trip;
	/*
	 * Van Jacobson timer algorithm here, only if NOT a retransmission.
	 */
	if (p->cku_timers != (struct rpc_timers *)0 &&
	    stries == p->cku_retrys) {
		register int rt;

		rt = round_trip;
		rt -= (p->cku_timers->rt_srtt >> 3);
		p->cku_timers->rt_srtt += rt;
		if (rt < 0)
			rt = - rt;
		rt -= (p->cku_timers->rt_deviate >> 2);
		p->cku_timers->rt_deviate += rt;
		p->cku_timers->rt_rtxcur = 
			(u_long)((p->cku_timers->rt_srtt >> 2) +
			  p->cku_timers->rt_deviate) >> 1;

		rt = round_trip;
		rt -= (p->cku_timeall->rt_srtt >> 3);
		p->cku_timeall->rt_srtt += rt;
		if (rt < 0)
			rt = - rt;
		rt -= (p->cku_timeall->rt_deviate >> 2);
		p->cku_timeall->rt_deviate += rt;
		p->cku_timeall->rt_rtxcur = 
			(u_long)((p->cku_timeall->rt_srtt >> 2) + 
			  p->cku_timeall->rt_deviate) >> 1;
		if (p->cku_feedback != (void (*)()) 0)
		    (*p->cku_feedback)(FEEDBACK_OK, procnum, p->cku_feedarg);
	}

	/*
	 * Process reply
	 */

	xdrs = &(p->cku_inxdr);
	xdrmblk_init(xdrs, unitdata->udata.udata_mp, XDR_DECODE);

	{
		/*
		 * Declare this variable here to have smaller
		 * demand for stack space in this procedure.
		 */
		struct rpc_msg		   reply_msg;

		reply_msg.acpted_rply.ar_verf = _null_auth;
		reply_msg.acpted_rply.ar_results.where = resultsp;
		reply_msg.acpted_rply.ar_results.proc = xdr_results;

		/*
		 * Decode and validate the response.
		 */
		if (xdr_replymsg(xdrs, &reply_msg)) {
			_seterr_reply(&reply_msg, &(p->cku_err));

			if (p->cku_err.re_status == RPC_SUCCESS) {
				/*
				 * Reply is good, check auth.
				 */
				if (! AUTH_VALIDATE(h->cl_auth,
				    &reply_msg.acpted_rply.ar_verf)) {
					p->cku_err.re_status = RPC_AUTHERROR;
					p->cku_err.re_why = AUTH_INVALIDRESP;
					rcstat.rcbadverfs++;
				}
				if (reply_msg.acpted_rply.ar_verf.oa_base !=
				    NULL) {
					/* free auth handle */
					xdrs->x_op = XDR_FREE;
					(void) xdr_opaque_auth(xdrs,
					    &(reply_msg.acpted_rply.ar_verf));
				}
			} else {
				/*
				 * Maybe our credential needs refreshed
				 */
				if (refreshes > 0 && AUTH_REFRESH(h->cl_auth)) {
					refreshes--;
					rcstat.rcnewcreds++;
					rempos = 0;
				}
			}
		} else {
			/* probably buffree() wasn't called */
			buffree(p);
			p->cku_err.re_status = RPC_CANTDECODERES;
			p->cku_err.re_errno = EIO;
		}
	}

	t_kfree(tiptr, p->cku_inudata, T_UNITDATA);   
	p->cku_inudata = NULL;


#ifdef RPCDEBUG
	printf("cku_callit done\n");
#endif
done:
	if ((p->cku_err.re_status != RPC_SUCCESS) &&
	    (p->cku_err.re_status != RPC_INTR) &&
	    (p->cku_err.re_status != RPC_CANTENCODEARGS)) {
		if (p->cku_feedback != (void (*)()) 0 &&
		    stries == p->cku_retrys)
			(*p->cku_feedback)(FEEDBACK_REXMIT1, 
				procnum, p->cku_feedarg);
		timohz = backoff(timohz);
		if (p->cku_timeall != (struct rpc_timers *)0)
			p->cku_timeall->rt_rtxcur = timohz;
		if (p->cku_err.re_status == RPC_SYSTEMERROR ||
		    p->cku_err.re_status == RPC_CANTSEND) {
			/*
			 * Errors due to lack o resources, wait a bit
			 * and try again.
			 */
			(void) sleep((caddr_t)&lbolt, PZERO-4);
		}
		if (--stries > 0) {
			rcstat.rcretrans++;
			goto call_again;
		}
	}
	u.u_procp->p_cred = tmpcred;
	/*
	 * Insure that buffer is not busy prior to releasing client handle.
	 */
	s = splstr();
	while (p->cku_flags & CKU_BUFBUSY) {
		rcstat.rcbufbusy2++;
		p->cku_flags |= CKU_BUFWANTED;
		if (cpjsleep((caddr_t)&p->cku_outbuf, PZERO-5, 10*100)<0){
		printf("cpjsleep: kludge2 buffer not sent in 10 secs\n");
		rcstat.kludge2++;
		buffree(p); /*bogus*/
		}
	}
	(void) splx(s);

	p->cku_flags &= ~CKU_BUSY;
	if (p->cku_flags & CKU_WANTED) {
		p->cku_flags &= ~CKU_WANTED;
		wakeup((caddr_t)h);
	}
	if (p->cku_err.re_status != RPC_SUCCESS) {
		rcstat.rcbadcalls++;
	}
	return (p->cku_err.re_status);
}

enum clnt_stat
clnt_clts_kcallit(h, procnum, xdr_args, argsp, xdr_results, resultsp, wait)
register CLIENT *h;
register u_long	 procnum;
register xdrproc_t xdr_args;
register caddr_t argsp;
register xdrproc_t xdr_results;
register caddr_t resultsp;
struct timeval	wait;
{
	return (clnt_clts_kcallit_addr(h, procnum, xdr_args, argsp, xdr_results,
		resultsp, wait, (struct netbuf *)0));
}


/*
 * Return error info on this handle.
 */
void
clnt_clts_kerror(h, err)
register CLIENT *h;
register struct rpc_err *err;
{
	/* LINTED pointer alignment */
	register struct cku_private *p = htop(h);

	*err = p->cku_err;
}

STATIC bool_t
clnt_clts_kfreeres(cl, xdr_res, res_ptr)
register CLIENT *cl;
register xdrproc_t xdr_res;
register caddr_t res_ptr;
{
	/* LINTED pointer alignment */
	register struct cku_private *p = (struct cku_private *)cl->cl_private;
	register XDR *xdrs = &(p->cku_outxdr);

	xdrs->x_op = XDR_FREE;
	return ((*xdr_res)(xdrs, res_ptr));
}

void
clnt_clts_kabort()
{
}

bool_t
clnt_clts_kcontrol()
{
	return (FALSE);
}


/*
 * Destroy rpc handle.
 * Frees the space used for output buffer, private data, and handle
 * structure, and the file pointer/TLI data on last reference.
 */
void
clnt_clts_kdestroy(h)
register CLIENT *h;
{
	/* LINTED pointer alignment */
	register struct cku_private *p = htop(h);
	register TIUSER *tiptr;

#ifdef RPCDEBUG
	printf("clnt_clts_destroy %x\n", h);
#endif
	tiptr = p->cku_tiptr;
	kmem_free((caddr_t)p->cku_outbuf, (u_int)UDPMSGSIZE);
	kmem_free((caddr_t)p->cku_addr.buf, p->cku_addr.maxlen);
	kmem_free((caddr_t)p, sizeof (*p));

	t_kclose(tiptr, 1);
}
#endif
