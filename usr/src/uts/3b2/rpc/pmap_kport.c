/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)krpc:krpc/pmap_kport.c	1.6"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)pmap_kgetport.c 1.5 89/01/13 SMI"
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
 * pmap_kgetport.c
 * Kernel interface to pmap rpc service.
 */

#include <rpc/types.h>
#include <netinet/in.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpc/rpcb_clnt.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <sys/cred.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/cmn_err.h>
static struct cred cred;
#define retries 4
static struct timeval tottimeout = { 1, 0 };
static char nullstring[] = "\000";

#ifndef DEBUG
#define cmn_err(type, msg)
#endif

/*
 * Find the mapped port for program,version.
 * Calls the pmap service remotely to do the lookup.
 * The 'address' argument is used to locate the portmapper.
 *
 * Returns:
 *	port number if successfully looked up.
 *	-1 otherwise.
 */
int
pmap_kgetport(address, program, version, config)
	struct netbuf *address;
	u_long program;
	u_long version;
	struct knetconfig *config;
{
	u_short port = 0;
	register CLIENT *pmapclient, *rpcbclient;
	struct pmap pmapparms;
	struct rpcb rpcbparms;
	char *ua, *uaddress;
	struct netbuf *na, *uaddr2taddr();
	int error = -1;/*cpj*/
	extern bool_t xdr_wrapstring();
	 
	if (cred.cr_ref == 0) {
		/*
		 * Reduce the number of groups in the cred from NGROUPS to 0.
		 */
		cred.cr_ngroups = 0;
		/* cred.cr_ref = 0; */	/* redundant */
	}

	/* now call the proper stuff.
	 */
	/* first try RPCBPROG */
	rpcbclient = (CLIENT *)clnt_tli_kcreate(config, address, (u_long)RPCBPROG,
					RPCBVERS, 0, 0, retries, &cred);
	if (rpcbclient != (CLIENT *)NULL) {
		uaddress = (char *) mem_alloc(1024);
		ua = uaddress;
		rpcbparms.r_prog = program;
		rpcbparms.r_vers = version;
		rpcbparms.r_netid = "udp";	/* for now */
		rpcbparms.r_addr = nullstring;  /* not needed or used */
		if (CLNT_CALL(rpcbclient, RPCBPROC_GETADDR, xdr_rpcb, &rpcbparms,
		    xdr_wrapstring, &ua, tottimeout) != RPC_SUCCESS){
			error = -1;	/* error contacting rpcbinder */
		} else if (ua[0] == NULL) {
			error = 0;	/* program not registered */
		} else {
			na = uaddr2taddr(config, uaddress);
			error = ((struct sockaddr_in *)na->buf)->sin_port;
			mem_free (na->buf, na->maxlen);
			mem_free ((char *)na, sizeof (struct netbuf));
		}
		mem_free (uaddress, 1024);
		AUTH_DESTROY(rpcbclient->cl_auth);
		CLNT_DESTROY(rpcbclient);
	}

	/* if RPCBPROG failed try PMAPPROG */
	if (error <= 0 && (pmapclient = (CLIENT *)clnt_tli_kcreate(config, address,
						(u_long)PMAPPROG, PMAPVERS, 0, 0, retries, &cred)) !=
						(CLIENT *)NULL) {
		pmapparms.pm_prog = program;
		pmapparms.pm_vers = version;
		pmapparms.pm_prot = config->nc_proto;
		pmapparms.pm_port = 0;  /* not needed or used */
		if (CLNT_CALL(pmapclient, PMAPPROC_GETPORT, xdr_pmap, &pmapparms,
		    xdr_u_short, &port, tottimeout) != RPC_SUCCESS){
			error = -1;	/* error contacting portmapper */
		} else if (port == 0) {
			error = 0;	/* program not registered */
		} else {
			error = port;
		}
		AUTH_DESTROY(pmapclient->cl_auth);
		CLNT_DESTROY(pmapclient);
	}

	return (error);
}

/*
 * getport_loop -- kernel interface to pmap_kgetport()
 *
 * Talks to the portmapper using the netbuf supplied by 'address',
 * to lookup the specified 'program'.
 *
 * If the portmapper does not respond, prints console message (once).
 * Retries forever, unless a signal is received.
 *
 * Returns:
 *	-1 on error
 *	0= no port
 *	port number on success.
 */
getport_loop(address, program, version, config)
	struct netbuf *address;
	u_long program;
	u_long version;
	struct knetconfig *config;
{
	register int pe = 0;
	register int i = 0;

	/* sit in a tight loop until the portmapper responds */
	while ((i = pmap_kgetport(address, program, version, config)) < 0) {

		/* test to see if a signal has come in */
		if (ISSIG(u.u_procp, 0)) {
			cmn_err(CE_NOTE, "Portmapper not responding; giving up\n");
			goto out;		/* got a signal */
		}
		/* print this message only once */
		if (pe++ == 0) {
			cmn_err(CE_NOTE, "Portmapper not responding; still trying\n");
		}
	}				/* go try the portmapper again */

	/* got a response...print message if there was a delay */
	if (pe != 0) {
		cmn_err(CE_NOTE, "Portmapper ok\n");
	}
out:
	return(i);	/* may return <0 if portmap dead */
}
