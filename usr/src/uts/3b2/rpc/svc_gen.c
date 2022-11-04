/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)krpc:krpc/svc_gen.c	1.3"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)svc_generic.c 1.1 88/12/12 SMI"
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
 * svc_generic.c,
 * Server side for RPC in the kernel.
 *
 */

#include <sys/param.h>
#include <sys/types.h>
#include <rpc/types.h>
#include <netinet/in.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <sys/tiuser.h>
#include <sys/t_kuser.h>
#include <rpc/svc.h>
#include <sys/file.h>
#include <sys/user.h>
#include <sys/stream.h>
#include <sys/tihdr.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
 
SVCXPRT *svc_clts_kcreate();

SVCXPRT *
svc_tli_kcreate(fp, sendsz, recvsz)
register struct file *fp;		/* connection end point */
u_int sendsz;				/* max sendsize */
u_int recvsz;				/* max recvsize */
{
	register SVCXPRT *xprt;		/* service handle */
	register TIUSER *tiptr;

	if (fp == NULL) {
		u.u_error = EINVAL;
		return NULL;
	}
	if ((tiptr = t_kopen(fp, -1, O_RDWR|O_NDELAY,
                                        (struct t_info *)NULL)) == NULL) {
                printf("svc_tli_kcreate: t_kopen: %d\n", u.u_error);
         	return NULL;
	}

	/*
	 * call transport specific function.
	 */
	switch(tiptr->tp_info.servtype) {
		case T_CLTS:
			xprt = svc_clts_kcreate(tiptr, sendsz, recvsz);
			break;
		default:
                	(void)printf("svc_tli_kcreate: Bad service type\n");
			u.u_error = EINVAL;
                	goto freedata;
        }
	if (xprt == NULL)
		goto freedata;


	xprt->xp_port = -1UL;	/* To show that it is tli based. Switch */

	/* remote address
	 */
	xprt->xp_rtaddr.buf = NULL;
	xprt->xp_rtaddr.len = 0;
	xprt->xp_rtaddr.maxlen = 0;

	return (xprt);

freedata:

	if (xprt)
		SVC_DESTROY(xprt);
	return ((SVCXPRT *)NULL);
}



