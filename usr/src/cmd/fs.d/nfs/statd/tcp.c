/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/statd/tcp.c	1.1"
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
	 * make tcp calls
	 */
#include <stdio.h>
#include <netdb.h>
#include <rpc/rpc.h>
/*
#include <sys/time.h>
*/
#include "time.h"
#include <netinet/in.h>
#include <memory.h>
#include <sys/socket.h>

extern int debug;
extern void klm_prog(), nlm_prog(), priv_prog();

/*
 *  routine taken from new_calltcp.c;
 *  no caching is done!
 *  continueously calling if timeout;
 *  in case of error, print put error msg; this msg usually is to be
 *  thrown away
 */
int
call_tcp(host, prognum, versnum, procnum, inproc, in, outproc, out, tot )
	char *host;
	xdrproc_t inproc, outproc;
	char *in, *out;
	int tot;
{
	struct sockaddr_in server_addr;
        struct in_addr *get_addr_cache();
        enum clnt_stat clnt_stat;
        struct hostent *hp;
        struct timeval  tottimeout;
        register CLIENT *client;
        int socket = RPC_ANYSOCK;

	if ((hp = gethostbyname(host)) == NULL) {
                if (debug)
                        printf( "RPC_UNKNOWNHOST\n");
                return ((int) RPC_UNKNOWNHOST);
        }
        memcpy((caddr_t)&server_addr.sin_addr, hp->h_addr, hp->h_length);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port =  0;

	tottimeout.tv_usec = 0;
	tottimeout.tv_sec = tot;

	if ((client = (CLIENT *)clnttcp_create(&server_addr, prognum, versnum, &socket,
                0, 0)) == NULL) {
                clnt_pcreateerror("clnttcp_create");   /* RPC_PMAPFAILURE or RPC_SYSTEMERROR */
                return ((int) rpc_createerr.cf_stat);  /* return (svr_not_avail); */
        }
again:
	clnt_stat = clnt_call(client, procnum, inproc, in, outproc, out,
			tottimeout);
	if (clnt_stat != RPC_SUCCESS)  {
		if (clnt_stat == RPC_TIMEDOUT) {
			if (tot != 0) {
				if (debug)
					printf("call_tcp timeout, retry\n");
				goto again;
			}
			/* if tot == 0, no reply is expected */
		}
		else {
			if (debug) {
				clnt_perrno(clnt_stat);
				fprintf(stderr, "\n");
			}
		}
	}
	/* should do cacheing, rather than always destroy */
	clnt_destroy(client);
	return (int) clnt_stat;
}
