/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)krpc:krpc/clnt_gen.c	1.6"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)clnt_gen.c 1.2 89/02/24 SMI"
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
 *  		  All rights reserved.
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
#include <rpc/xdr.h>
#include <sys/file.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/stream.h>
#include <sys/tihdr.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/cred.h>

CLIENT *clnt_clts_kcreate();

CLIENT *
clnt_tli_kcreate(config, svcaddr, prog, vers, sendsz, recvsz, retrys, cred)
register struct knetconfig *config;
struct netbuf *svcaddr;		/* Servers address */
u_long prog;			/* Program number */
u_long vers;			/* Version number */
u_int sendsz;			/* send size */
u_int recvsz;			/* recv size */
int   retrys;
struct cred *cred;
{
	register CLIENT *cl = NULL;	/* Client handle */
	/* register int fd, state; */	/* Current state of provider */
	register TIUSER *tiptr;
	register struct file *fp;
	struct cred *tmpcred, *savecred;

#ifdef RPCDEBUG
printf("clnt_tli_kcreate: Entered fd %d, svcaddr %x\n", fd, svcaddr);
#endif
	if (config == NULL) {
		u.u_error = EINVAL;
		printf("clnt_tli_kcreate: null config\n");
		return NULL;
	}

	/* the transport should be opened as root */
	tmpcred = crdup(u.u_cred);
	savecred = u.u_cred;
	u.u_cred = tmpcred;
	u.u_cred->cr_uid = 0;
	fp = NULL;
	if ((tiptr = t_kopen(fp, config->nc_rdev, FREAD|FWRITE|FNDELAY,
				(struct t_info *)NULL)) == NULL) {
		printf("clnt_tli_kcreate: t_kopen: %d\n", u.u_error);
		return NULL;
	}
	u.u_cred = savecred;
	crfree(tmpcred);

	/* must bind the endpoint.
	 */
	if (config->nc_protofmly == AF_INET) {
		if (bindresvport(tiptr) < 0) {
			printf("clnt_tli_kcreate: bindresvport failed\n");
			goto err;
		}
	}
	else	{
		if (t_kbind(tiptr, NULL, NULL) < 0) {
			printf("clnt_tli_kcreate: t_kbind: %d\n", u.u_error);
			t_kclose(tiptr, 1);
			return NULL;
		}
	}

	switch(tiptr->tp_info.servtype) {
		case T_CLTS:
			cl = clnt_clts_kcreate(tiptr, svcaddr, prog, vers, sendsz, recvsz, retrys, cred);
			if (cl == (CLIENT *)NULL) {
				printf("clnt_tli_kcreate: clnt_clts_kcreate failed\n");
				goto err;
			}
			break;

		default:
			printf("clnt_tli_kcreate: Bad service type\n");
			u.u_error = EINVAL;
			goto err;
	}
	return cl;
err:
	t_kclose(tiptr, 1);
	return (CLIENT *)NULL;
}

/*
 * try to bind to a reserved port
 */
bindresvport(tiptr)
register TIUSER *tiptr;
{
	struct taddr_in *sin;
	register int i;
	int error;
	struct cred *tmpcred;
	struct cred *savecred;
	struct t_bind *req, *ret;

#	define MAX_PRIV	(IPPORT_RESERVED-1)
#	define MIN_PRIV	(IPPORT_RESERVED/2)

#ifdef RPCDEBUG
printf("bindresvport: calling t_kalloc tiptr = %x\n", tiptr);
#endif
	/* LINTED pointer alignment */
	if ((req = (struct t_bind *)t_kalloc(tiptr, T_BIND, T_ADDR)) == (struct t_bind *)NULL) {
		printf("bindresvport: t_kalloc %d\n", u.u_error);
		return u.u_error;
	}

#ifdef RPCDEBUG
printf("bindresvport: calling t_kalloc tiptr = %x\n", tiptr);
#endif
	/* LINTED pointer alignment */
	if ((ret = (struct t_bind *)t_kalloc(tiptr, T_BIND, T_ADDR)) == (struct t_bind *)NULL) {
		printf("bindresvport: t_kalloc %d\n", u.u_error);
		return u.u_error;
	}
	/* LINTED pointer alignment */
	sin = (struct taddr_in *)req->addr.buf;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	req->addr.len = sizeof(struct taddr_in);

	/*
	 * Only root can bind to a privileged port number, so
	 * temporarily change the uid to 0 to do the bind.
	 */
	tmpcred = (struct cred *)crdup(u.u_procp->p_cred);
	savecred = u.u_procp->p_cred;
	u.u_procp->p_cred = tmpcred;
	tmpcred->cr_uid = 0;

	error = EADDRINUSE;
	for (i = MAX_PRIV; error == EADDRINUSE && i >= MIN_PRIV; i--) {
		sin->sin_port = htons(i);
#ifdef RPCDEBUG
printf("bindresvport: calling t_kbind tiptr = %x\n", tiptr);
#endif
		if ((error = t_kbind(tiptr, req, ret)) < 0) {
			printf("bindresvport: t_kbind: %d\n", u.u_error);
			error = u.u_error;
		}
		else
		if (bcmp((caddr_t)req, (caddr_t)ret, sizeof(req)) != 0) {
			printf("bindresvport: bcmp error\n");
			error = EIO;
		}
	}
	if (t_kfree(tiptr, req, T_BIND) < 0 ||
		t_kfree(tiptr, ret, T_BIND) < 0) {
		printf("bindresvport: error on t_kfree\n");
	}
	u.u_procp->p_cred = savecred;
	crfree(tmpcred);
	return (error);
}
