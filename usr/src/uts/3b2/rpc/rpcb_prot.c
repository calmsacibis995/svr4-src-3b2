/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)krpc:rpc/rpcb_prot.c	1.2"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)rpcb_kprot.c 1.1 89/05/18 SMI"
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
 * rpcb_kprot.c.c
 * Kernel rpcb protocol routines.
 * (+ TCP/IP uaddr2taddr )
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/tiuser.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpcb_prot.h>

bool_t
xdr_rpcb(xdrs, objp)
	XDR *xdrs;
	RPCB *objp;
{
	if (!xdr_u_long(xdrs, &objp->r_prog)) {
		return (FALSE);
	}
	if (!xdr_u_long(xdrs, &objp->r_vers)) {
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->r_netid, ~0)) {
		return (FALSE);
	}
	if (!xdr_string(xdrs, &objp->r_addr, ~0)) {
		return (FALSE);
	}
	return (TRUE);
}

/* TCP/IP specific uaddr2taddr - for now we know we're talking to UDP */
/* addr format a1.a2.a3.a4.p1.p2 */
struct netbuf *
uaddr2taddr(tp, addr)
	struct knetconfig	*tp;
	char				*addr;
{
	struct sockaddr_in	*sin;
	struct netbuf *result;
	u_long inaddr = 0;
	u_short inport = 0;
	char *cp = addr;
	int i;

	result = mem_alloc(sizeof(struct netbuf));
	result->maxlen = 8;
	result->len = 8;
	result->buf = mem_alloc(result->maxlen);
	sin = (struct sockaddr_in *)result->buf;
	for (i=3; i>=0; i--) {
		while (*cp != '.') cp++;
		*cp++ = '\0';
		inaddr += atoi(addr) << (i*8);
		addr = cp;
	}
	for (i=1; i>=0; i--) {
		while (*cp != '.' && *cp != '\0') cp++;
		*cp++ = '\0';
		inport += atoi(addr) << (i*8);
		addr = cp;
	}

	sin->sin_family = AF_INET;
	sin->sin_port = inport;
	sin->sin_addr.s_addr = inaddr;
	return (result);
}

static int
atoi(s)
	char *s;
{
	int i, n = 0;

	for (i=0; s[i] >= '0' && s[i] <= '9'; i++)
		n = 10 * n + (s[i] - '0');
	return n;
}
