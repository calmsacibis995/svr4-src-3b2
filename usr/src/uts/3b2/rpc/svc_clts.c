/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)krpc:krpc/svc_clts.c	1.5"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)svc_clts.c 1.2 89/01/11 SMI"
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

#ifdef _KERNEL
/*
 * svc_clts.c 
 * Server side for RPC in the kernel.
 *
 */

#include <sys/param.h>
#include <sys/types.h>
#include <rpc/types.h>
#include <netinet/in.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <sys/tiuser.h>
#include <sys/t_kuser.h>
#include <rpc/svc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/file.h>
#include <sys/user.h>
#include <sys/stream.h>
#include <sys/tihdr.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/kmem.h>

#define rpc_buffer(xprt) ((xprt)->xp_p1)

static void unhash();

/*
 * Routines exported through ops vector.
 */
bool_t		svc_clts_krecv();
bool_t		svc_clts_ksend();
enum xprt_stat	svc_clts_kstat();
bool_t		svc_clts_kgetargs();
bool_t		svc_clts_kfreeargs();
void		svc_clts_kdestroy();


/*
 * Server transport operations vector.
 */
struct xp_ops svc_clts_op = {
	svc_clts_krecv,		/* Get requests */
	svc_clts_kstat,		/* Return status */
	svc_clts_kgetargs,	/* Deserialize arguments */
	svc_clts_ksend,		/* Send reply */
	svc_clts_kfreeargs,	/* Free argument data space */
	svc_clts_kdestroy	/* Destroy transport handle */
};

/*
 * Transport private data.
 * Kept in xprt->xp_p2.
 */
struct udp_data {
	int	ud_flags;			/* flag bits, see below */
	u_long 	ud_xid;				/* id */
	struct  t_kunitdata *ud_inudata;
	XDR	ud_xdrin;			/* input xdr stream */
	XDR	ud_xdrout;			/* output xdr stream */
	char	ud_verfbody[MAX_AUTH_BYTES];	/* verifier */
	frtn_t	ud_frtn;			/* message free routine */
};


/*
 * Flags
 */
#define	UD_BUSY		0x001		/* buffer is busy */
#define	UD_WANTED	0x002		/* buffer wanted */

/*
 * Server statistics
 */
struct {
	int	rscalls;
	int	rsbadcalls;
	int	rsnullrecv;
	int	rsbadlen;
	int	rsxdrcall;
} rsstat;

/*
 * Create a transport record.
 * The transport record, output buffer, and private data structure
 * are allocated.  The output buffer is serialized into using xdrmem.
 * There is one transport record per user process which implements a
 * set of services.
 */

/*ARGSUSED*/
SVCXPRT *
svc_clts_kcreate(tiptr, sendsz, recvsz)
register TIUSER *tiptr;
register u_int  sendsz, recvsz;
{
	register SVCXPRT	 *xprt;
	register struct udp_data *ud;

#ifdef RPCDEBUG
printf("svc_clts_kcreate: Entered tiptr %x\n", tiptr);
#endif

	xprt = (SVCXPRT *)kmem_alloc((u_int)sizeof(SVCXPRT), KM_SLEEP);
	rpc_buffer(xprt) = (caddr_t)kmem_alloc((u_int)UDPMSGSIZE, KM_SLEEP);
	ud = (struct udp_data *)kmem_alloc((u_int)sizeof(struct udp_data), KM_SLEEP);
	bzero((caddr_t)ud, sizeof(*ud));
	xprt->xp_p2 = (caddr_t)ud;
	xprt->xp_p3 = NULL;
	xprt->xp_verf.oa_base = ud->ud_verfbody;
	xprt->xp_ops = &svc_clts_op;
	xprt->xp_tiptr = tiptr;
	bzero((caddr_t)&xprt->xp_ltaddr, sizeof(struct netbuf));
	return (xprt);
}
 
/*
 * Destroy a transport record.
 * Frees the space allocated for a transport record.
 */
void
svc_clts_kdestroy(xprt)
	register SVCXPRT   *xprt;
{
	/* LINTED pointer alignment */
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;
	/* register struct file *tiptr; */

#ifdef RPCDEBUG
	printf("usr_destroy %x\n", xprt);
#endif
	if (ud->ud_inudata) {
                if (t_kfree(xprt->xp_tiptr, (char *)ud->ud_inudata, T_UNITDATA) < 0)
                        printf("svc_clts_kdestroy: t_kfree: %d\n", u.u_error);
	}
	if (xprt->xp_ltaddr.buf)
		kmem_free(xprt->xp_ltaddr.buf, xprt->xp_ltaddr.maxlen);

	t_kclose(xprt->xp_tiptr, 0);
	kmem_free((caddr_t)ud, (u_int)sizeof(struct udp_data));
	kmem_free((caddr_t)rpc_buffer(xprt), (u_int)UDPMSGSIZE);
	kmem_free((caddr_t)xprt, (u_int)sizeof(SVCXPRT));
}

/*
 * Receive rpc requests.
 * Pulls a request in off the socket, checks if the packet is intact,
 * and deserializes the call packet.
 */
bool_t
svc_clts_krecv(xprt, msg)
	register SVCXPRT	 *xprt;
	struct rpc_msg		 *msg;
{
	/* LINTED pointer alignment */
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;
	register XDR	 *xdrs = &(ud->ud_xdrin);
	register struct t_kunitdata *inudata;
	int flags;

#ifdef RPCDEBUG
	printf("svc_clts_krecv %x\n", xprt);
#endif
	/* get a receive buffer
	 */
	if ((inudata = (struct t_kunitdata *)t_kalloc(xprt->xp_tiptr,
			 /* LINTED pointer alignment */
			 T_UNITDATA, T_ADDR|T_UDATA)) == (struct t_kunitdata *)NULL) {
                printf("svc_clts_krecv: t_kalloc: %d\n", u.u_error);
                goto bad;
        }

	rsstat.rscalls++;
	if (t_krcvudata(xprt->xp_tiptr, inudata, &flags) < 0) {
                printf("svc_clts_krecv: t_krcvudata: %d\n", u.u_error);
                if (u.u_error == EAGAIN) {
                        rsstat.rsnullrecv++;
                        return FALSE;
                }
                else    goto bad;
        }
#ifdef RPCDEBUG
printf("svc_clts_krecv: t_krcvudata returned %d bytes\n", inudata->udata.len);
#endif
	xprt->xp_rtaddr.buf = kmem_alloc (inudata->addr.len, KM_SLEEP);
	xprt->xp_rtaddr.len = inudata->addr.len;
	xprt->xp_rtaddr.maxlen = inudata->addr.maxlen;
	bcopy (inudata->addr.buf, xprt->xp_rtaddr.buf, xprt->xp_rtaddr.len);
 
        if (inudata->udata.len < 4*sizeof(u_long)) {
                printf("svc_clts_krecv: bad length %d\n", inudata->udata.len);  
                rsstat.rsbadlen++;
                goto bad;
        }
	xdrmblk_init(xdrs, inudata->udata.udata_mp, XDR_DECODE);
        if (! xdr_callmsg(xdrs, msg)) {
                printf("svc_clts_krecv: bad xdr_callmsg\n");
                rsstat.rsxdrcall++;
                goto bad;
        }
        ud->ud_xid = msg->rm_xid;
        ud->ud_inudata = inudata;

#ifdef RPCDEBUG
	printf("svc_clts_krecv done\n");
#endif
	return (TRUE);
bad:
	if (t_kfree(xprt->xp_tiptr, (char *)inudata, T_UNITDATA) < 0)
                printf("svc_clts_krecv: t_kfree %d\n", u.u_error);
        ud->ud_inudata = NULL;

	rsstat.rsbadcalls++;
	return (FALSE);
}


static void
buffree(ud)
	register struct udp_data *ud;
{
	ud->ud_flags &= ~UD_BUSY;
	if (ud->ud_flags & UD_WANTED) {
		ud->ud_flags &= ~UD_WANTED;
		wakeup((caddr_t)ud);
	}
}

/*
 * Send rpc reply.
 * Serialize the reply packet into the output buffer then
 * call t_ksndudata to send it.
 */
bool_t
/* ARGSUSED */
svc_clts_ksend(xprt, msg)
	register SVCXPRT *xprt; 
	struct rpc_msg *msg; 
{
	/* LINTED pointer alignment */
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;
	register XDR *xdrs = &(ud->ud_xdrout);
	register int slen;
	register int stat = FALSE;
	int 	 s;
	struct   t_kunitdata *unitdata;

#ifdef RPCDEBUG
	printf("svc_clts_ksend %x\n", xprt);
#endif
	s = splstr();
	while (ud->ud_flags & UD_BUSY) {
		ud->ud_flags |= UD_WANTED;
		(void) sleep((caddr_t)ud, PZERO-2);
	}
	ud->ud_flags |= UD_BUSY;
	(void) splx(s);
	xdrmem_create(xdrs, rpc_buffer(xprt), UDPMSGSIZE, XDR_ENCODE);
	msg->rm_xid = ud->ud_xid;
	if (xdr_replymsg(xdrs, msg)) {
		slen = (int)XDR_GETPOS(xdrs);
	        if ((unitdata = (struct t_kunitdata *)t_kalloc(xprt->xp_tiptr,   
			/* LINTED pointer alignment */
                        T_UNITDATA, T_ADDR)) == (struct t_kunitdata *)NULL) {
                        printf("svc_clts_ksend: t_kalloc: %d\n", u.u_error);    
                }
                else    {
			unitdata->addr.len = xprt->xp_rtaddr.len;
			bcopy(xprt->xp_rtaddr.buf, unitdata->addr.buf,
						unitdata->addr.len);
 
                        unitdata->udata.buf = rpc_buffer(xprt);
                        unitdata->udata.len = slen;
                        ud->ud_frtn.free_func = buffree;
                        ud->ud_frtn.free_arg  = (char *)ud;
 
#ifdef RPCDEBUG
printf("svc_clts_ksend: calling t_ksndudata fd = %x, bytes = %d\n", xprt->xp_tiptr, unitdata->udata.len);
#endif
                        if (t_ksndudata(xprt->xp_tiptr, unitdata, &ud->ud_frtn) < 0) {
                                printf("svc_clts_ksend: t_ksndudata: %d\n",     
                                        u.u_error);
			}
			else	{
                                stat = TRUE;
	                }
                        /* now we have to free up the unitdata
                         */
                        if (t_kfree(xprt->xp_tiptr, (char *)unitdata, T_UNITDATA) < 0)
                                printf("svc_clts_ksend: t_kfree: %d\n",
u.u_error);
                }
	} else	{
#ifdef RPCDEBUG
printf("svc_clts_ksend: xdr_replymsg failed\n");
#endif
		buffree (ud);
		/* not allocated, so don't free it */
		/* t_kfree(xprt->xp_tiptr, (char *)unitdata, T_UNITDATA); */
	}
	/*
	 * This is completely disgusting.  If public is set it is
	 * a pointer to a structure whose first field is the address
	 * of the function to free that structure and any related
	 * stuff.  (see rrokfree in nfs_xdr.c).
	 */

	if (xdrs->x_public) {
		/* LINTED pointer alignment */
		(**((int (**)())xdrs->x_public))(xdrs->x_public);
	}
#ifdef RPCDEBUG
	printf("svc_clts_ksend done\n");
#endif
	return (stat);
}

/*
 * Return transport status.
 */
/*ARGSUSED*/
enum xprt_stat
svc_clts_kstat(xprt)
	SVCXPRT *xprt;
{

	return (XPRT_IDLE); 
}

/*
 * Deserialize arguments.
 */
bool_t
svc_clts_kgetargs(xprt, xdr_args, args_ptr)
	SVCXPRT	*xprt;
	xdrproc_t	 xdr_args;
	caddr_t		 args_ptr;
{

	/* LINTED pointer alignment */
	return ((*xdr_args)(&(((struct udp_data *)(xprt->xp_p2))->ud_xdrin), args_ptr));
}

bool_t
svc_clts_kfreeargs(xprt, xdr_args, args_ptr)
	SVCXPRT	*xprt;
	xdrproc_t	 xdr_args;
	caddr_t		 args_ptr;
{
	/* LINTED pointer alignment */
	register XDR *xdrs = &(((struct udp_data *)(xprt->xp_p2))->ud_xdrin);
	/* LINTED pointer alignment */
	register struct udp_data *ud = (struct udp_data *)xprt->xp_p2;

	if (ud->ud_inudata) {
                if (t_kfree(xprt->xp_tiptr, (char *)ud->ud_inudata, T_UNITDATA) < 0)
                        printf("svc_clts_kfreeargs: t_kfree: %d\n", u.u_error);
        }
        ud->ud_inudata = (struct t_kunitdata *)NULL;
	if (args_ptr) {
		xdrs->x_op = XDR_FREE;
		return ((*xdr_args)(xdrs, args_ptr));
	} else {
		return (TRUE);
	}
}

/*
 * the dup cacheing routines below provide a cache of non-failure
 * transaction id's.  rpc service routines can use this to detect
 * retransmissions and re-send a non-failure response.
 */

struct dupreq {
	u_long		dr_xid;
	struct netbuf	dr_addr;
	u_long		dr_proc;
	u_long		dr_vers;
	u_long		dr_prog;
	struct dupreq	*dr_next;
	struct dupreq	*dr_chain;
};

/*
 * MAXDUPREQS is the number of cached items.  It should be adjusted
 * to the service load so that there is likely to be a response entry
 * when the first retransmission comes in.
 */
#define	MAXDUPREQS	400

#define	DUPREQSZ	(sizeof(struct dupreq) - 2*sizeof(caddr_t))
#define	DRHASHSZ	32
#define	XIDHASH(xid)	((xid) & (DRHASHSZ-1))
#define	DRHASH(dr)	XIDHASH((dr)->dr_xid)
#define	REQTOXID(req)	((struct udp_data *)((req)->rq_xprt->xp_p2))->ud_xid

int	ndupreqs;
int	dupreqs;
int	dupchecks;
struct dupreq *drhashtbl[DRHASHSZ];

/*
 * drmru points to the head of a circular linked list in lru order.
 * drmru->dr_next == drlru
 */
struct dupreq *drmru;

void
svc_clts_kdupsave(req)
	register struct svc_req *req;
{
	register struct dupreq *dr;

	if (ndupreqs < MAXDUPREQS) {
		dr = (struct dupreq *)kmem_alloc(sizeof(*dr), KM_SLEEP);
		if (drmru) {
			dr->dr_next = drmru->dr_next;
			drmru->dr_next = dr;
		} else {
			dr->dr_next = dr;
		}
		ndupreqs++;
	} else {
		dr = drmru->dr_next;
		unhash(dr);
	}
	drmru = dr;

	/* LINTED pointer alignment */
	dr->dr_xid = REQTOXID(req);
	dr->dr_prog = req->rq_prog;
	dr->dr_vers = req->rq_vers;
	dr->dr_proc = req->rq_proc;
	dr->dr_addr = req->rq_xprt->xp_rtaddr;
	dr->dr_chain = drhashtbl[DRHASH(dr)];
	drhashtbl[DRHASH(dr)] = dr;
}

svc_clts_kdup(req)
	register struct svc_req *req;
{
	register struct dupreq *dr;
	u_long xid;
	 
	dupchecks++;
	/* LINTED pointer alignment */
	xid = REQTOXID(req);
	dr = drhashtbl[XIDHASH(xid)]; 
	while (dr != NULL) { 
		if (dr->dr_xid != xid ||
		    dr->dr_prog != req->rq_prog ||
		    dr->dr_vers != req->rq_vers ||
		    dr->dr_proc != req->rq_proc ||
		    bcmp((caddr_t)&dr->dr_addr,
		     (caddr_t)&req->rq_xprt->xp_rtaddr,
		     sizeof(dr->dr_addr)) != 0) {
			dr = dr->dr_chain;
			continue;
		} else {
			dupreqs++;
			return (1);
		}
	}
	return (0);
}

static void
unhash(dr)
	struct dupreq *dr;
{
	struct dupreq *drt;
	struct dupreq *drtprev = NULL;
	 
	drt = drhashtbl[DRHASH(dr)]; 
	while (drt != NULL) { 
		if (drt == dr) { 
			if (drtprev == NULL) {
				drhashtbl[DRHASH(dr)] = drt->dr_chain;
			} else {
				drtprev->dr_chain = drt->dr_chain;
			}
			return; 
		}	
		drtprev = drt;
		drt = drt->dr_chain;
	}	
}
#endif	/* _KERNEL */
