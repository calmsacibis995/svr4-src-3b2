/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/netdir.h	1.1"
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
 * netdir.h
 *
 * This is the include file that defines various structures and
 * constants used by the netdir routines.
 */

#ifndef _netdir_h
#define _netdir_h

struct nd_addrlist {
	int 		n_cnt;		/* number of netbufs */
	struct netbuf 	*n_addrs;	/* the netbufs */
};

struct nd_hostservlist {
	int			h_cnt;		/* number of nd_hostservs */
	struct nd_hostserv	*h_hostservs;	/* the entries */
};

struct nd_hostserv {
	char		*h_host;	/* the host name */
	char		*h_serv;	/* the service name */
};

#ifdef ANSI
int netdir_getbyname(struct netconfig *, struct nd_hostserv *, struct nd_addrlist **);
int netdir_getbyaddr(struct netconfig *, struct nd_hostservlist **, struct netbuf *);
int netdir_mergeaddr(struct netconfig *, char **muaddr, char *ruaddr, char *uaddr);
int netdir_control(struct netconfig *, int option, int fd, char *par);
void netdir_free(char *, int);
struct netbuf *uaddr2taddr(struct netconfig *, char *);
char *taddr2uaddr(struct netconfig *, struct netbuf *);
void netdir_perror(char *);
char *netdir_sperror();
struct nd_addrlist *_netdir_getbyname(struct netconfig *, struct nd_hostserv *);
struct nd_hostservlist *_netdir_getbyaddr(struct netconfig *, struct netbuf *);
char *_netdir_mergeaddr(struct netconfig *, char *ruaddr, char *uaddr);
struct netbuf *_uaddr2taddr(struct netconfig *, char *);
char *_taddr2uaddr(struct netconfig *, struct netbuf *);

#else

int netdir_getbyname();
int netdir_getbyaddr();
int netdir_mergeaddr();
int netdir_control();
void netdir_free();
struct netbuf *uaddr2taddr();
void netdir_perror();
char *netdir_sperror();
char *taddr2uaddr();
struct nd_addrlist *_netdir_getbyname();
struct nd_hostservlist *_netdir_getbyaddr();
char *_netdir_mergeaddr();
struct netbuf *_uaddr2taddr();
char *_taddr2uaddr();

#endif /* ANSI */

/*
 * These are all objects that can be freed by netdir_free
 */
#define ND_HOSTSERV	0
#define ND_HOSTSERVLIST	1
#define ND_ADDR		2
#define ND_ADDRLIST	3

/* 
 * These are the various errors that can be encountered while attempting
 * to translate names to addresses. Note that none of them (except maybe
 * no memory) are truely fatal unless the ntoa deamon is on its last attempt
 * to translate the name. 
 *
 * Negative errors terminate the search resolution process, positive errors
 * are treated as warnings.
 */
#define ND_BADARG	-2	/* Bad arguments passed 	*/
#define ND_NOMEM 	-1	/* No virtual memory left	*/
#define ND_OK		0	/* Translation successful	*/
#define ND_NOHOST	1	/* Hostname was not resolvable	*/
#define ND_NOSERV	2	/* Service was unknown		*/
#define ND_NOSYM	3	/* Couldn't resolve symbol	*/
#define ND_OPEN		4	/* File couldn't be opened	*/
#define ND_ACCESS	5	/* File is not accessable	*/
#define ND_UKNWN	6	/* Unknown object to be freed	*/
#define ND_NOCTRL	7	/* Unknown option passed to netdir_control */
#define ND_FAILCTRL	8	/* Option failed in netdir_control */
#define ND_SYSTEM	9	/* Other System error		*/

/*
 * The following special case host names are used to give the underlying
 * transport provider a clue as to the intent of the request. 
 */

#define HOST_SELF	"\001"	/* The generic bind address for this tp */
#define HOST_ANY	"\002"	/* A "don't care" option for the host   */
#define HOST_BROADCAST	"\003"	/* The broadcast address for this tp    */

/*
 * The following netdir_control commands can be given to the fd. These is
 * a way of providing for any transport specific action which the caller
 * may want to initiate on his transport. It is up to the trasport provider
 * to support the netdir_control he wants to support.
 */
#define ND_SET_BROADCAST	1	/* Do t_optmgmt to support broadcast*/
#define ND_SET_RESERVEDPORT	2	/* bind it to reserve address */
#define ND_CHECK_RESERVEDPORT	3	/* check if address is reserved */
#endif /* !_netdir_h */
