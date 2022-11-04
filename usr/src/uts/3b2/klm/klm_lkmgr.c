/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)klm:klm/klm_lkmgr.c	1.4"
#ifndef lint
static char sccsid[] = "@(#)klm_lkmgr.c 1.4 89/07/12 SMI";
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

        /*
         * Copyright (c) 1989 by Sun Microsystems, Inc.
         */

/*
 * Kernel<->Network Lock-Manager Interface
 *
 * File- and Record-locking requests are forwarded (via RPC) to a
 * Network Lock-Manager running on the local machine.  The protocol
 * for these transactions is defined in /usr/src/protocols/klm_prot.x
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/errno.h>
#include <sys/cred.h>
#include <sys/socket.h>
/* #include <sys/socketvar.h> */
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <rpc/pmap_prot.h>
#include <sys/cmn_err.h>
/*
#include <sys/procfs.h>
*/

/* files included by <rpc/rpc.h> */
#include <rpc/types.h>
#include <netinet/in.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc.h>

#include <klm/lockmgr.h>
#include <rpcsvc/klm_prot.h>
/* #include <net/if.h> */
#include <nfs/nfs.h>
#include <nfs/nfs_clnt.h>
#include <nfs/rnode.h>

#undef  wakeup

extern void     wakeup();		/* reference the function, not the */
					/* macro 			   */

static struct sockaddr_in lm_sa;	/* talk to portmapper & lock-manager */

static talk_to_lockmgr();

/* Define static parameters for run-time tuning */

#ifdef NOTUSE
static int backoff_timeout = 1;		/* time to wait on klm_denied_nolocks */
static int first_retry = 0;		/* first attempt if klm port# known */
static int first_timeout = 1;
static int normal_retry = 1;		/* attempts after new port# obtained */
static int normal_timeout = 1;
static int working_retry = 0;		/* attempts after klm_working */
static int working_timeout = 1;
#endif

static int backoff_timeout = 30;	/* time to wait on klm_denied_nolocks */
static int first_retry = 0;		/* first attempt if klm port# known */
static int first_timeout = 30;
static int normal_retry = 1;		/* attempts after new port# obtained */
static int normal_timeout = 30;
static int working_retry = 0;		/* attempts after klm_working */
static int working_timeout = 30;


/*
 * klm_lockctl - process a lock/unlock/test-lock request
 *
 * Calls (via RPC) the local lock manager to register the request.
 * Lock requests are cancelled if interrupted by signals.
 */
klm_lockctl(lh, bfp, cmd, cred, clid)
	lockhandle_t *lh;
	struct flock *bfp;
	int cmd;
	struct cred *cred;
	pid_t clid;
{
	register int	error;
	char 		*args;
	klm_lockargs	klm_lockargs_args;
	klm_unlockargs  klm_unlockargs_args;
	klm_testargs    klm_testargs_args;
	klm_testrply	reply;
	u_long		xdrproc;
	xdrproc_t	xdrargs;
	xdrproc_t	xdrreply;
	int		timeid;

	/* initialize sockaddr_in used to talk to local processes */
	if (lm_sa.sin_port == 0) {
		lm_sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		lm_sa.sin_family = AF_INET;
	}

	if (!bfp->l_pid) bfp->l_pid = clid; /* FIXME */

	switch (cmd) {
	case F_SETLK:
	case F_SETLKW:
		if (bfp->l_type != F_UNLCK) {
			if (cmd == F_SETLKW)
				klm_lockargs_args.block = TRUE;
			else
				klm_lockargs_args.block = FALSE;
			if (bfp->l_type == F_WRLCK) {
				klm_lockargs_args.exclusive = TRUE;
			} else {
				klm_lockargs_args.exclusive = FALSE;
			}
			klm_lockargs_args.alock.fh.n_bytes = (char *)&lh->lh_id;
			klm_lockargs_args.alock.fh.n_len = sizeof (lh->lh_id);
			klm_lockargs_args.alock.server_name = lh->lh_servername;
			klm_lockargs_args.alock.pid = clid;
			klm_lockargs_args.alock.base = bfp->l_start;
			klm_lockargs_args.alock.length = bfp->l_len;
			klm_lockargs_args.alock.rsys = bfp->l_sysid;
			args = (char *) &klm_lockargs_args;
			xdrproc = KLM_LOCK;
			xdrargs = (xdrproc_t)xdr_klm_lockargs;
			xdrreply = (xdrproc_t)xdr_klm_stat;
		} else {
			klm_unlockargs_args.alock.fh.n_bytes = (char *)&lh->lh_id;
			klm_unlockargs_args.alock.fh.n_len = sizeof (lh->lh_id);
			klm_unlockargs_args.alock.server_name = lh->lh_servername;
			klm_unlockargs_args.alock.pid = clid;
			klm_unlockargs_args.alock.base = bfp->l_start;
			klm_unlockargs_args.alock.length = bfp->l_len;
			klm_unlockargs_args.alock.rsys = bfp->l_sysid;
			args = (char *) &klm_unlockargs_args;
			xdrreply = (xdrproc_t)xdr_klm_stat;
			xdrproc = KLM_UNLOCK;
			xdrargs = (xdrproc_t)xdr_klm_unlockargs;
		}
		break;

	case F_GETLK:
		if (bfp->l_type == F_WRLCK) {
			klm_testargs_args.exclusive = TRUE;
		} else {
			klm_testargs_args.exclusive = FALSE;
		}
		klm_testargs_args.alock.fh.n_bytes = (char *)&lh->lh_id;
		klm_testargs_args.alock.fh.n_len = sizeof (lh->lh_id);
		klm_testargs_args.alock.server_name = lh->lh_servername;
		klm_testargs_args.alock.pid = clid;
		klm_testargs_args.alock.base = bfp->l_start;
		klm_testargs_args.alock.length = bfp->l_len;
		klm_testargs_args.alock.rsys = bfp->l_sysid;
		args = (char *) &klm_testargs_args;
		xdrproc = KLM_TEST;
		xdrargs = (xdrproc_t)xdr_klm_testargs;
		xdrreply = (xdrproc_t)xdr_klm_testrply;
		break;
	}

requestloop:
	/* send the request out to the local lock-manager and wait for reply */
	error = talk_to_lockmgr(xdrproc, xdrargs, args, xdrreply, &reply, cred);
	if (error == ENOLCK) {
		goto ereturn;	/* no way the request could have gotten out */
	}

	/*
	 * The only other possible return values are:
	 *   klm_granted  |  klm_denied  | klm_denied_nolocks |  EINTR
	 */
	switch (xdrproc) {
	case KLM_LOCK:
		switch (error) {
		case klm_granted:
			error = 0;		/* got the requested lock */
			goto ereturn;
		case klm_denied:
			if (klm_lockargs_args.block) {
				cmn_err(CE_CONT,
					"klm_lockmgr: blocking lock denied?!\n");
				goto requestloop;	/* loop forever */
			}
			error = EACCES;		/* EAGAIN?? */
			goto ereturn;
		case klm_denied_nolocks:
			error = ENOLCK;		/* no resources available?! */
			goto ereturn;
		case klm_deadlck:
			error = EDEADLK;	/* deadlock condition */
			goto ereturn;
		case EINTR:
			if (klm_lockargs_args.block)
				goto cancel;	/* cancel blocking locks */
			else
				goto requestloop;	/* loop forever */
		}

	case KLM_UNLOCK:
		switch (error) {
		case klm_granted:
			error = 0;
			goto ereturn;
		case klm_denied:
		case EINTR:
#ifdef LOCKDEBUG
			cmn_err(CE_CONT, "klm_lockmgr: unlock denied?!\n");
#endif
			error = EINVAL;
			goto ereturn;
		case klm_denied_nolocks:
			goto nolocks_wait;	/* back off; loop forever */
#ifdef NOTUSE
		case EINTR:
			goto requestloop;	/* loop forever */
#endif
		}

	case KLM_TEST:
		switch (error) {
		case klm_granted:
			bfp->l_type = F_UNLCK;	/* mark lock available */
			error = 0;
#ifdef LOCKDEBUG
			cmn_err(CE_CONT,
				"KLM_GRANTED : pid=%d start=%d len=%d\n",
				bfp->l_pid,bfp->l_start,bfp->l_len);
#endif
			goto ereturn;
		case klm_denied:
			bfp->l_type = (reply.klm_testrply_u.holder.exclusive) ?
			    F_WRLCK : F_RDLCK;
			bfp->l_start = reply.klm_testrply_u.holder.base;
			bfp->l_len = reply.klm_testrply_u.holder.length;
			bfp->l_pid = reply.klm_testrply_u.holder.pid;
			bfp->l_sysid = reply.klm_testrply_u.holder.rsys;
#ifdef LOCKDEBUG
			cmn_err(CE_CONT,
				"KLM_DENIED : pid=%d start=%d len=%d\n", 
                                bfp->l_pid,bfp->l_start,bfp->l_len);
#endif
			error = 0;
			goto ereturn;
		case klm_denied_nolocks:
			goto nolocks_wait;	/* back off; loop forever */
		case EINTR:
			/* may want to take a longjmp here */
			goto requestloop;	/* loop forever */
		}
	}

/* NOTREACHED */
nolocks_wait:
	timeid = timeout(wakeup, (caddr_t)&lm_sa, (backoff_timeout * HZ));
	(void) sleep((caddr_t)&lm_sa, PZERO|PCATCH);
#ifdef NOTUSE
	untimeout(wakeup, (caddr_t)&lm_sa);
#endif
	untimeout(timeid);
	goto requestloop;	/* now try again */

cancel:
	/*
	 * If we get here, a signal interrupted a rqst that must be cancelled.
	 * Change the procedure number to KLM_CANCEL and reissue the exact same
	 * request.  Use the results to decide what return value to give.
	 */
	xdrproc = KLM_CANCEL;
	error = talk_to_lockmgr(xdrproc, xdrargs, args, xdrreply, &reply, cred);
	switch (error) {
	case klm_granted:
		error = 0;		/* lock granted */
		goto ereturn;
	case klm_denied:
		/* may want to take a longjmp here */
		error = EINTR;
		goto ereturn;
	case klm_deadlck:
		error = EDEADLK;
		goto ereturn;
	case EINTR:
		goto cancel;		/* ignore signals til cancel succeeds */

	case klm_denied_nolocks:
		error = ENOLCK;		/* no resources available?! */
		goto ereturn;
	case ENOLCK:
		cmn_err(CE_CONT, "klm_lockctl: ENOLCK on KLM_CANCEL request\n");
		goto ereturn;
	}
/* NOTREACHED */
ereturn:
	return (error);
}


/*
 * Send the given request to the local lock-manager.
 * If timeout or error, go back to the portmapper to check the port number.
 * This routine loops forever until one of the following occurs:
 *	1) A legitimate (not 'klm_working') reply is returned (returns 'stat').
 *
 *	2) A signal occurs (returns EINTR).  In this case, at least one try
 *	   has been made to do the RPC; this protects against jamming the
 *	   CPU if a KLM_CANCEL request has yet to go out.
 *
 *	3) A drastic error occurs (e.g., the local lock-manager has never
 *	   been activated OR cannot create a client-handle) (returns ENOLCK).
 */
static
talk_to_lockmgr(xdrproc, xdrargs, args, xdrreply, reply, cred)
	u_long xdrproc;
	xdrproc_t xdrargs;
	char *args;
	xdrproc_t xdrreply;
	klm_testrply *reply;
	struct cred *cred;
{
	extern int clone_no, udp_no;    /* got from ../io/conf.c */
	CLIENT *client;
	struct timeval tmo;
	register int error;
	struct knetconfig config;
        struct netbuf netaddr, addr;
	int timeid;

	/* set up a client handle to talk to the local lock manager */
	netaddr.buf = (char *)&lm_sa;
        netaddr.len = sizeof(lm_sa);
        netaddr.maxlen = sizeof(lm_sa);

        /* 
	 * filch a knetconfig structure.
         */
	if (!udp_no) udp_no = 56;	
        config.nc_protofmly = AF_INET;
        config.nc_rdev = makedevice(clone_no, udp_no);
        config.nc_proto = 17; /*IPPROTO_UDP*/

	/*
	 * now call the proper stuff.
	 */
	client = (CLIENT *)clnt_tli_kcreate(&config, &netaddr, (u_long)KLM_PROG,
		(u_long)KLM_VERS, 0, 0, first_retry, cred);
	if (client == (CLIENT *) NULL) {
		return (ENOLCK);
	}
	tmo.tv_sec = first_timeout;
	tmo.tv_usec = 0;

#ifdef NOTUSE
	/*
	 * If cached port number, go right to CLNT_CALL().
	 * This works because timeouts go back to the portmapper to
	 * refresh the port number.
	 */
	if (lm_sa.sin_port != 0) {
		goto retryloop;		/* skip first portmapper query */
	}
#endif

	for (;;) {
remaploop:
		/*
		 * go get the port number from the portmapper(rpcbinder)...
		 * if return 0, the server is not registered
		 * if return -1, an error in contacting the portmapper
		 * else, got a port number
		 */
		lm_sa.sin_port = htonl(PMAPPORT);
		lm_sa.sin_port = getport_loop(&netaddr,
		    	(u_long)KLM_PROG, (u_long)KLM_VERS, &config);
#ifdef LOCKDEBUG
		cmn_err(CE_CONT, "lm_sa.sin_port=%d\n",lm_sa.sin_port);
#endif
		switch(lm_sa.sin_port) {
		case 0:
		case (u_short)-1:
			cmn_err(CE_CONT,
				"fcntl: local NFS lock manager not registered\n");
			error = ENOLCK;
			goto out;
		}

		/*
		 * If a signal occurred, pop back out to the higher
		 * level to decide what action to take.  If we just
		 * got a port number from the portmapper, the next
		 * call into this subroutine will jump to retryloop.
		 */
		if (ISSIG(u.u_procp, 0)) {
			error = EINTR;
			goto out;
		}

		/* reset the lock-manager client handle */
		addr.buf = (char *)&lm_sa;
        	addr.len = sizeof(lm_sa);
        	addr.maxlen = sizeof(lm_sa);

		clnt_clts_init(client, &addr, normal_retry, cred);
		tmo.tv_sec = normal_timeout;

retryloop:
		/* retry the request until completion, timeout, or error */
		for (;;) {
			error = (int) CLNT_CALL(client, xdrproc,
				xdrargs, (caddr_t)args, xdrreply,
				(caddr_t)reply, tmo);
			switch (error) {
			case RPC_SUCCESS:
			case klm_denied:
				error = (int) reply->stat;
				if (error == (int) klm_working) {
					if (ISSIG(u.u_procp, 0)) {
						error = EINTR;
						goto out;
					}
					/* lock-mgr is up...can wait longer */
					addr.buf = (char *)&lm_sa;
                			addr.len = sizeof(lm_sa);
                			addr.maxlen = sizeof(lm_sa);
                			clnt_clts_init(client, &addr,
						working_retry, cred);
					tmo.tv_sec = working_timeout;
					continue;	/* retry */
				}
				goto out;	/* got a legitimate answer */

			case RPC_TIMEDOUT:
				goto remaploop;	/* ask for port# again */

			case klm_denied_nolocks:
				goto out;

			default:
				cmn_err(CE_CONT,
					"lock-manager: RPC error: %s\n",
				clnt_sperrno((enum clnt_stat) error));

				/* on RPC error, wait a bit and try again */
#ifdef LOCKDEBUG
				cmn_err(CE_CONT, "BEFORE TIMEOUT: %d\n",
                                        normal_timeout * HZ);
#endif
				timeid = timeout(wakeup, (caddr_t)&lm_sa,
				    (normal_timeout * HZ));
				error = sleep((caddr_t)&lm_sa, PZERO|PCATCH);
#ifdef NOTUSE
				untimeout(wakeup, (caddr_t)&lm_sa);
#endif
				untimeout(timeid);
				if (error) {
				    error = EINTR;
				    goto out;
				}
				goto remaploop;	/* ask for port# again */

			} /* switch */

		} /* for */	/* loop until timeout, error, or completion */

	} /* for */		/* loop until signal or completion */

out:
	AUTH_DESTROY(client->cl_auth);	/* drop the authenticator */
	CLNT_DESTROY(client);		/* drop the client handle */
	return (error);
}
