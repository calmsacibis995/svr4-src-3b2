/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rpcbind:check_bound.c	1.6"

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


#ifndef lint
static	char sccsid[] = "@(#)check_bound.c 1.11 89/04/21 Copyr 1989 Sun Micro";
#endif

/*
 * check_bound.c
 * Checks to see whether the program is still bound to the
 * claimed address and returns the univeral merged address
 *
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <netconfig.h>
#include <netdir.h>

extern char *malloc();

struct fdlist {
	int fd;
	struct netconfig *nconf;
	struct fdlist *next;
	int check_binding;
};

static struct fdlist *fdhead;	/* Link list of the check fd's */
static struct fdlist *fdtail;
static char *nullstring = "";

/*
 * Returns 1 if the given address is bound for the given addr & transport
 * For all error cases, we assume that the address is bound
 * Returns 0 for success.
 */
static bool_t
check_bound(fdl, uaddr)
	struct fdlist *fdl;	/* My FD list */
	char *uaddr;		/* the universal address */
{
	int fd;
	struct netbuf *na;
	struct t_bind taddr, *baddr;
	int ans;

	if (fdl->check_binding == FALSE)
		return (TRUE);

	na = uaddr2taddr(fdl->nconf, uaddr);
	if (!na)
		return (TRUE); /* punt, should never happen */

	fd = fdl->fd;
	taddr.addr = *na;
	taddr.qlen = 1;
	baddr = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR);
	if (baddr == NULL) {
		netdir_free((char *)na, ND_ADDR);
		return (TRUE);
	}
	if (t_bind(fd, &taddr, baddr) != 0) {
		netdir_free((char *)na, ND_ADDR);
		(void) t_free((char *)baddr, T_BIND);
		(void) t_unbind(fd);
		return (TRUE);
	}
	ans = memcmp(taddr.addr.buf, baddr->addr.buf, baddr->addr.len);
	netdir_free((char *)na, ND_ADDR);
	(void) t_free((char *)baddr, T_BIND);
	if (t_unbind(fd) != 0) {
		/* Bad fd. Purge this fd */
		(void) t_close(fd);
		fdl->fd = t_open(fdl->nconf->nc_device, O_RDWR, NULL);
		if (fdl->fd == -1)
			fdl->check_binding = FALSE;
	}
	return (ans == 0 ? FALSE : TRUE);
}

/*
 * Keep open one more file descriptor for this transport, which
 * will be used to determine whether the given service is up
 * or not by trying to bind to the registered address.
 * We are ignoring errors here.
 */
int
add_bndlist(nconf, check_binding)
	struct netconfig *nconf;
	int check_binding;
{
	int fd;
	struct fdlist *fdl;
	struct netconfig *newnconf;

	newnconf = getnetconfigent(nconf->nc_netid);
	if (newnconf == NULL)
		return (-1);
	fdl = (struct fdlist *)malloc((u_int)sizeof (struct fdlist));
	if (fdl == NULL) {
		syslog("no memory!");
		exit(1);
	}
	fdl->nconf = newnconf;
	fdl->check_binding = check_binding;
	if (check_binding)
		fdl->fd = t_open(nconf->nc_device, O_RDWR, NULL);
	else
		fdl->fd = -1;
	fdl->next = NULL;
	if (fdhead == NULL) {
		fdhead = fdl;
		fdtail = fdl;
	} else {
		fdtail->next = fdl;
		fdtail = fdl;
	}
	return (0);
}

bool_t
is_bound(netid, uaddr)
	char *netid;
	char *uaddr;
{
	struct fdlist *fdl;

	for (fdl = fdhead; fdl; fdl = fdl->next)
		if (strcmp(fdl->nconf->nc_netid, netid) == 0)
			break;
	if (fdl == NULL)
		return (TRUE);
	return (check_bound(fdl, uaddr));
}

/*
 * Returns NULL if there was some system error.
 * Returns "" if the address was not bound, i.e the server crashed.
 * Returns the merged address otherwise.
 */
char *
mergeaddr(xprt, uaddr)
	SVCXPRT *xprt;
	char *uaddr;
{
	struct fdlist *fdl;
	char *netid = xprt->xp_netid;
	struct nd_mergearg ma;
	int stat;
	ma.s_uaddr = uaddr;

	for (fdl = fdhead; fdl; fdl = fdl->next)
		if (strcmp(fdl->nconf->nc_netid, netid) == 0)
			break;
	if (fdl == NULL)
		return (NULL);
	if (check_bound(fdl, uaddr) == FALSE)
		/* that server died */
		return (nullstring);
	ma.c_uaddr = taddr2uaddr(fdl->nconf, svc_getrpccaller(xprt));
	if (ma.c_uaddr == NULL) {
		syslog("taddr2uaddr failed for %s: %s",
			fdl->nconf->nc_netid, netdir_sperror());
		return (NULL);
	}
#ifdef ND_DEBUG
	fprintf(stderr, "mergeaddr: client uaddr = %s\n", ma.c_uaddr);
#endif
	stat = netdir_options(fdl->nconf, ND_MERGEADDR, 0, &ma);
	(void) free(ma.c_uaddr);
	if (stat) {
		syslog("netdir_merge failed for %s: %s",
			fdl->nconf->nc_netid, netdir_sperror());
		return (NULL);
	}
#ifdef ND_DEBUG
	fprintf(stderr, "mergeaddr: uaddr = %s, merged uaddr = %s\n",
				uaddr, ma.m_uaddr);
#endif
	return (ma.m_uaddr);
}
