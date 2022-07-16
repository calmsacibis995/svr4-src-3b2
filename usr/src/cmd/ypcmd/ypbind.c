/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libyp:ypbind.c	1.2"

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
/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <stdio.h>
#include <signal.h>
#include <rpc/rpc.h>
#include <memory.h>
#include <netconfig.h>
#ifdef SYSLOG
#include <syslog.h>
#else
#define LOG_ERR 1
#define openlog(a, b, c)
#endif
#include "yp_b.h"

#ifdef DEBUG
#define RPC_SVC_FG
#endif

#define _RPCSVC_CLOSEDOWN 120
#include <netconfig.h>

extern void ypbindprog_3();
extern void closedown();

extern int _rpcpmstart;		/* Started by a port monitor ? */

main(argc, argv)
	int argc;
	char **argv;
{
	int pid, i;

	argc--;
	argv++;
	while (argc > 0) {
		if (!strcmp(*argv,"-ypset")) {
			setok = TRUE;
		} else if (!strcmp(*argv,"-ypsetme")) {
			setok = YPSETLOCAL;
		} else {
			fprintf(stderr, "usage: ypbind [-ypset] [-ypsetme]\n");
			exit(1);
		}
		argc--,
		argv++;
	}

	if (setok==TRUE) {
		fprintf(stderr, 
		    "ypbind -ypset: allowing ypset! (this is insecure)\n");
	}
	if (setok==YPSETLOCAL) {
		fprintf(stderr, 
		    "ypbind -ypsetme: allowing local ypset! (this is insecure)\n");
	}
	if (t_sync(0) != -1) {
		char *netid;
		struct netconfig *nconf;
		SVCXPRT *transp;
		extern char *getenv();

		_rpcpmstart = 1;
		if ((netid = getenv("NLSPROVIDER")) == NULL) {
			_msgout("cannot get transport name");
			exit(1);
		}
		if ((nconf = getnetconfigent(netid)) == NULL) {
			_msgout("cannot get transport info");
			exit(1);
		}
		if ((transp = svc_tli_create(0, nconf, NULL, 0, 0)) == NULL) {
			_msgout("cannot create server handle");
			exit(1);
		}
		if (!svc_reg(transp, YPBINDPROG, YPBINDVERS, ypbindprog_3, 0)) {
			_msgout("unable to register (YPBINDPROG, YPBINDVERS).");
			exit(1);
		}
		(void) signal(SIGALRM, closedown);
		(void) alarm(_RPCSVC_CLOSEDOWN);
		svc_run();
		_msgout("svc_run returned");
		exit(1);
		/* NOTREACHED */
	}
#ifndef RPC_SVC_FG
	pid = fork();
	if (pid < 0) {
		perror("cannot fork");
		exit(1);
	}
	if (pid)
		exit(0);
	for (i = 0 ; i < 20; i++)
		(void) close(i);
	(void) setsid();
	openlog("yp_b", LOG_PID, LOG_DAEMON);
#endif
	if (!svc_create(ypbindprog_3, YPBINDPROG, YPBINDVERS, "netpath")) {
 		_msgout("unable to create (YPBINDPROG, YPBINDVERS) for netpath.");
		exit(1);
	}

	svc_run();
	_msgout("svc_run returned");
	exit(1);
	/* NOTREACHED */
}