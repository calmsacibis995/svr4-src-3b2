/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)librpc:clnt_bcast.c	1.4"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
*          All rights reserved.
*/ 


#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)clnt_bcast.c 1.15 89/04/21 Copyr 1988 Sun Micro";
#endif

/*
 * clnt_bcast.c
 * Client interface to broadcast service.
 *
 * Copyright (C) 1988, Sun Microsystems, Inc.
 *
 * The following is kludged-up support for simple rpc broadcasts.
 * Someday a large, complicated system will replace these trivial 
 * routines.
 */

#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_rmt.h>
#include <rpc/nettype.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include <errno.h>
#ifdef SYSLOG
#include <sys/syslog.h>
#else
#define LOG_ERR 3
#endif /* SYSLOG */

extern char *strdup();
extern int errno;

#define MAXBCASTADDRS	10 /* The max broadcast addresses for a transport */
#define MAXBCAST	20 /* Max no of transports supporting broadcasts */


typedef bool_t (*resultproc_t)();

/*
 * If nettype is NULL, it broadcasts on all the locally available
 * datagram_n transports. May potentially lead to broadacst storms
 * and hence should be used with caution, care and courage.
 *
 * The current parameter xdr packet size is limited by the max tsdu size
 * of the transport. If the max tsdu size of any transport is smaller
 * than the parameter xdr packet, then broadcast is not sent on that
 * transport.
 */
enum clnt_stat
rpc_broadcast(prog, vers, proc, xargs, argsp, xresults, resultsp,
			eachresult, nettype)
	u_long		prog;		/* program number */
	u_long		vers;		/* version number */
	u_long		proc;		/* procedure number */
	xdrproc_t	xargs;		/* xdr routine for args */
	caddr_t		argsp;		/* pointer to args */
	xdrproc_t	xresults;	/* xdr routine for results */
	caddr_t		resultsp;	/* pointer to results */
	resultproc_t	eachresult;	/* call with each result obtained */
	char		*nettype;	/* transport type */
{
	enum clnt_stat stat = RPC_SUCCESS;/* Return status */
	XDR xdr_stream;			/* XDR stream */
	register XDR *xdrs = &xdr_stream;
	int outlen;			/* Length of the xdr'ed message */
	fd_set mask;			/* File descriptor mask */
	fd_set readfds;
	register u_long xid;		/* Transaction id */
	u_long port;			/* Not required, but still used - vipin*/
					/* very required for backword comp- cpj */
	struct netbuf addrs[MAXBCASTADDRS];	/* broadcast addresses */
	struct rmtcallargs a;		/* Remote arguments */
	struct rmtcallres r;		/* Remote results */
	struct rpc_msg msg;		/* RPC message */
	struct timeval t;
	char *outbuf = NULL;		/* Broadcasted message buffer */
	char *inbuf = NULL;		/* Reply buf */
	struct {
		int fd;			/* File descriptor */
		struct netconfig *nconf;/* Network config structure */
		u_int asize;		/* Size of the address buf */
		u_int dsize;		/* Size of the data buf */
		struct netbuf raddr;	/* Remote address */
	} fdlist[MAXBCAST];		/* A list of fd and netconfs */
	struct t_unitdata t_udata, t_rdata;
	struct netconfig *nconf;
	register int fdlistno = 0;
	long maxbufsize = 0;
	AUTH *sys_auth = authsys_create_default();
	register int i;
	bool_t done = FALSE;
	int net;

	if (sys_auth == (AUTH *)NULL)
		return(RPC_SYSTEMERROR);
	/*
	 * initialization: create a fd, a broadcast address, and send the
	 * request on the broadcast transport.
	 * Listen on all of them and on replies call the user supplied
	 * function.
	 */
	FD_ZERO(&mask);

	if (nettype == NULL)
		nettype = "datagram_n";
	if ((net = _rpc_setconf(nettype)) == 0)
		return (RPC_UNKNOWNPROTO);
	while (nconf = _rpc_getconf(net)) {
		struct t_bind *taddr;
		struct t_info tinfo;
		int fd;

		if (!(nconf->nc_flag & NC_BROADCAST))
			continue;
		if (fdlistno >= MAXBCAST)
			break;	/* No more slots available */
		if ((fd = t_open(nconf->nc_device, O_RDWR, &tinfo)) == -1) {
			stat = RPC_CANTSEND;
			continue;
		}
		if (t_bind(fd, (struct t_bind *)NULL, (struct t_bind *)NULL) == -1) {
			(void) t_close(fd);
			stat = RPC_CANTSEND;
			continue;
		}
		/* Do protocol specific negotiating for broadcast */
		if (negotiate_broadcast(fd, nconf)) {
			(void) t_close(fd);
			stat = RPC_CANTSEND;
			continue;
		}
		taddr = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR);
		if (taddr == (struct t_bind *)NULL) {
			(void) t_close(fd);
			stat = RPC_SYSTEMERROR;
			goto done_broad;
		}
		FD_SET(fd, &mask);
		fdlist[fdlistno].fd = fd;
		fdlist[fdlistno].nconf = nconf;
		fdlist[fdlistno].asize = _rpc_get_a_size(tinfo.addr);
		fdlist[fdlistno].dsize = _rpc_get_t_size(0, tinfo.tsdu);
		fdlist[fdlistno].raddr = taddr->addr;
		if (maxbufsize <= fdlist[fdlistno].dsize)
			maxbufsize = fdlist[fdlistno].dsize;
		taddr->addr.buf = NULL;
		(void) t_free((char *)taddr, T_BIND);
		fdlistno++;
	}

	if (fdlist == 0) {
		if (stat == RPC_SUCCESS)
			stat = RPC_UNKNOWNPROTO;
		goto done_broad;
	}
	if (maxbufsize == 0) {
		if (stat == RPC_SUCCESS)
			stat = RPC_CANTSEND;
		goto done_broad;
	}
	inbuf = malloc(maxbufsize);
	outbuf = malloc(maxbufsize);
	if ((inbuf == NULL) || (outbuf == NULL)) {
		stat = RPC_SYSTEMERROR;
		goto done_broad;
	}

	/* Serialize all the arguments which have to be sent */
	(void) gettimeofday(&t, (struct timezone *)0);
	msg.rm_xid = xid = getpid() ^ t.tv_sec ^ t.tv_usec;
	msg.rm_direction = CALL;
	msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	msg.rm_call.cb_prog = PMAPPROG;
	msg.rm_call.cb_vers = PMAPVERS;
	msg.rm_call.cb_proc = PMAPPROC_CALLIT;
	msg.rm_call.cb_cred = sys_auth->ah_cred;
	msg.rm_call.cb_verf = sys_auth->ah_verf;
	a.prog = prog;
	a.vers = vers;
	a.proc = proc;
	a.xdr_args = xargs;
	a.args_ptr = argsp;
	/*
	 * This is also being used by the earlier socket broadcasts;
	 * Though port doesnt make sense here, We still continue to use it
	 * and ignore this non-malign part.
	 */
	r.port_ptr = &port;
	r.xdr_results = xresults;
	r.results_ptr = resultsp;
	/* Allocate the space for the max buf size */
	xdrmem_create(xdrs, outbuf, maxbufsize, XDR_ENCODE);
	if ((! xdr_callmsg(xdrs, &msg)) || (! xdr_rmtcall_args(xdrs, &a))) {
		stat = RPC_CANTENCODEARGS;
		goto done_broad;
	}
	outlen = (int)xdr_getpos(xdrs);

	t_udata.opt.len = 0;
	t_udata.udata.buf = outbuf;
	t_udata.udata.len = outlen;
	t_rdata.opt.len = 0;

	/*
	 * Basic loop: broadcast the packets to only those transports which
	 * support data packets of size such that one can encode all the
	 * arguments.
	 * Wait a while for response(s).
	 * The response timeout grows larger per iteration.
	 */
	t.tv_usec = 0;
	for (t.tv_sec = 4; t.tv_sec <= 14; t.tv_sec += 2) {
	    /* Broadcast all the packets now */
	    int j;

	    for (i = 0; i < fdlistno; i++) {
		int nets;

		if (fdlist[i].dsize < outlen) {
			stat = RPC_CANTSEND;
			continue;
		}
		nets = getbroadcastnets(fdlist[i].fd, addrs, fdlist[i].nconf);
		if (nets == 0) {
			stat = RPC_NOBROADCAST;
			continue;
		}

		for (j = 0; j < nets; j++) {
/* XXX Will go away with a decent getbroadcastnets */
			addrs[j].len = addrs[j].maxlen = fdlist[i].asize;
/* */			t_udata.addr = addrs[j];
			if (t_sndudata(fdlist[i].fd, &t_udata) != 0) {
				(void) syslog(LOG_ERR, "Cannot send broadcast packet: %m");
				t_error("broadcast: t_sndudata");
				stat = RPC_CANTSEND;
				continue;
			}
#ifdef DEBUG
			fprintf(stderr, "Broadcast packet sent for %s\n",
					fdlist[i].nconf->nc_netid);
#endif
		}
	    }

	    if (eachresult == NULL) {
		stat = RPC_SUCCESS;
		goto done_broad;;
	    }			

	    /*
	     * Get all the replies from these broadcast requests
	     */
	recv_again:
	    msg.acpted_rply.ar_verf = _null_auth;
	    msg.acpted_rply.ar_results.where = (caddr_t)&r;
	    msg.acpted_rply.ar_results.proc = xdr_rmtcallres;
	    readfds = mask;
	    switch (select(_rpc_dtbsize(), &readfds, (fd_set *)NULL,
		       (fd_set *)NULL, &t)) {

	    case 0:  /* timed out */
		stat = RPC_TIMEDOUT;
		continue;

	    case -1:  /* some kind of error */
		if (errno == EINTR)
			goto recv_again;
		(void) syslog(LOG_ERR, "Broadcast select problem: %m");
		stat = RPC_CANTRECV;
		goto done_broad;
	    }  /* end of select results switch */

	    t_rdata.udata.buf = inbuf;

	    for (i = 0; i < fdlistno; i++) {
		int flag ;

		if (!FD_ISSET(fdlist[i].fd, &readfds))
			continue;
	try_again:
		t_rdata.udata.maxlen = fdlist[i].dsize;
		t_rdata.udata.len = 0;
		t_rdata.addr = fdlist[i].raddr;
		if (t_rcvudata(fdlist[i].fd, &t_rdata, &flag) == -1) {
			if (errno == EINTR)
				goto try_again;
			(void) syslog(LOG_ERR, "Cannot receive reply to broadcast: %m");
			stat = RPC_CANTRECV;
			continue;
		}
		/*
		 * Not taking care of flag for T_MORE. We are assuming that
		 * such calls should not take more than one packet.
		 */
		if (flag & T_MORE)
			continue;	/* Drop that and go ahead */
		if (t_rdata.udata.len < sizeof(u_long))
			continue;	/* Drop that and go ahead */
		/*
		 * see if reply transaction id matches sent id.
		 * If so, decode the results.
		 */
		xdrmem_create(xdrs, inbuf, (u_int)t_rdata.udata.len, XDR_DECODE);
		if (xdr_replymsg(xdrs, &msg)) {
			if ((msg.rm_xid == xid) &&
				(msg.rm_reply.rp_stat == MSG_ACCEPTED) &&
				(msg.acpted_rply.ar_stat == SUCCESS)) {
#ifdef DEBUG
				int k;

				for (k = 0; k < t_rdata.addr.len; k++)
				    fprintf(stderr, "%d ", t_rdata.addr.buf[k]);
				fprintf(stderr, "\n");
#endif
				done = (*eachresult)(resultsp, &t_rdata.addr,
						fdlist[i].nconf);
			}
			/* otherwise, we just ignore the errors ... */
		} else {
#ifdef notdef
			/* some kind of deserialization problem ... */
			if (msg.rm_xid == xid)
				(void) syslog(LOG_ERR, "Broadcast deserialization problem");
			/* otherwise, just random garbage */
#endif
		}
		xdrs->x_op = XDR_FREE;
		msg.acpted_rply.ar_results.proc = xdr_void;
		(void) xdr_replymsg(xdrs, &msg);
		(void) (*xresults)(xdrs, resultsp);
		XDR_DESTROY(xdrs);
		if (done) {
			stat = RPC_SUCCESS;
			goto done_broad;
		} else {
			goto recv_again;
		}
	    } /* The recv for loop */
	} /* The giant for loop */

done_broad:
	if (inbuf)
		(void) free(inbuf);
	if (outbuf)
		(void) free(outbuf);
	for (i = 0; i < fdlistno; i++) {
		(void) t_close(fdlist[i].fd);
		(void) free(fdlist[i].raddr.buf);
	}
	AUTH_DESTROY(sys_auth);
	(void) _rpc_endconf();
	return (stat);
}
