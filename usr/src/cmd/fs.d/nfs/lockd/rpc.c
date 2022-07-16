/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/rpc.c	1.3"
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
 * this file consists of routines to support call_rpc();
 * client handles are cached in a hash table;
 * clntudp_create is only called if (site, prog#, vers#) cannot
 * be found in the hash table;
 * a cached entry is destroyed, when remote site crashes
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <string.h>
#include <sys/param.h>

#define MAX_HASHSIZE 100

char *malloc();
char *xmalloc();
extern int debug;
extern int HASH_SIZE;
extern void nlm_prog(), klm_prog(), priv_prog();

struct cache {
	char *host;
	int prognum;
	int versnum;
	int sock;
	CLIENT *client;
	struct cache *nxt;
};

struct cache *table[MAX_HASHSIZE];
int cache_len = sizeof (struct cache);

hash(name)
	char *name;
{
	int len;
	int i, c;

	c = 0;
	len = strlen(name);
	for (i = 0; i< len; i++) {
		c = c +(int) name[i];
	}
	c = c %HASH_SIZE;
	return (c);
}

/*
 * find_hash returns the cached entry;
 * it returns NULL if not found;
 */
struct cache *
find_hash(host, prognum, versnum)
	char *host;
	int prognum, versnum;
{
	struct cache *cp;

	if (debug)
		printf("enter find_hash() ...\n");

	cp = table[hash(host)];
	while ( cp != (struct cache *)NULL) {
		if (strcmp(cp->host, host) == 0 &&
		 cp->prognum == prognum && cp->versnum == versnum) {
			/* found */
			return (cp);
		}
		cp = cp->nxt;
	}
	return (NULL);
}

struct cache *
add_hash(host, prognum, versnum)
	char *host;
	int prognum, versnum;
{
	struct cache *cp;
	int h;

	if (debug)
		printf("enter add_hash() ...\n");

	if ((cp = (struct cache *) xmalloc(cache_len)) == (struct cache *)NULL ) {
		return (NULL);	/* malloc error */
	}
	if ((cp->host = xmalloc(strlen(host)+1)) == (char *)NULL ) {
		if (cp != NULL) free(cp);
		return (NULL);	/* malloc error */
	}
	(void) strcpy(cp->host, host);
	cp->prognum = prognum;
	cp->versnum = versnum;
	h = hash(host);
	cp->nxt = table[h];
	table[h] = cp;
	return (cp);
}

void
delete_hash(host)
	char *host;
{
	struct cache *cp;
	struct cache *cp_prev = (struct cache *)NULL;
	struct cache *next;
	int h;

	if (debug)
		printf("enter delete_hash() ...\n");

	/*
	 * if there is more than one entry with same host name;
	 * delete has to be recurrsively called
	 */

	h = hash(host);
	next = table[h];
	while ((cp = next) != (struct cache *)NULL) {
		next = cp->nxt;
		if (strcmp(cp->host, host) == 0) {
			if (cp_prev == (struct cache *)NULL) {
				table[h] = cp->nxt;
			}
			else {
				cp_prev->nxt = cp->nxt;
			}
			if (debug)
				printf("delete hash entry (%x), %s \n", cp, host);
			if (cp->client)
				clnt_destroy(cp->client);
			if (cp->host != NULL) free(cp->host);
			if (cp != NULL) free(cp);
		}
		else {
			cp_prev = cp;
		}
	}
}

call_rpc(host, prognum, versnum, procnum, inproc, in, outproc, out, valid_in, t)
	char *host;
	u_long prognum, versnum;
	xdrproc_t inproc, outproc;
	char *in, *out;
	int valid_in;
	int t;
{
        enum clnt_stat clnt_stat;
        struct timeval timeout, tottimeout;
        struct cache *cp;
	struct t_info tinfo;
        int fd, port = 0;
	extern int t_errno;

	if (debug)
		printf("enter call_rpc() ...\n");

	if ((cp = find_hash(host, prognum, versnum)) == (struct cache *)NULL) {
		if ((cp = add_hash(host, prognum, versnum)) == (struct cache *)NULL) {
			fprintf(stderr, "udp cannot send due to out of cache\n");
			return (-1);
		}
		if (debug)
			printf("(%x):[%s, %d, %d] is a new connection\n",
				cp, host, prognum, versnum);

		cp->client = clnt_create(host, prognum, versnum, "netpath");
		if (cp->client == (CLIENT *)NULL) {
			delete_hash(host);
			return (rpc_createerr.cf_stat);
		}
		(void) CLNT_CONTROL(cp->client, CLGET_FD, &fd);
		if (t_getinfo(fd, &tinfo) != -1) {
			if (tinfo.servtype == T_CLTS) {
				struct timeval timeout;

				/*
				 * Set time outs for connectionless case
				 */
				timeout.tv_usec = 0;
				timeout.tv_sec = 15;
				(void) CLNT_CONTROL(cp->client,
					CLSET_RETRY_TIMEOUT, &timeout);
			}
		} else {
			rpc_createerr.cf_stat = RPC_TLIERROR;
			rpc_createerr.cf_error.re_terrno = t_errno;
			delete_hash(host);
			return (rpc_createerr.cf_stat);
		}
	} else {
		if (valid_in == 0) { /* cannot use cache */
			if (debug)
				printf("(%x):[%s, %d, %d] is a new connection\n",
					cp, host, prognum, versnum);

			cp->client = clnt_create(host, prognum, versnum, "netpath");
                	if (cp->client == (CLIENT *)NULL) {
                        	return (rpc_createerr.cf_stat);
                	}
                	(void) CLNT_CONTROL(cp->client, CLGET_FD, &fd);
                	if (t_getinfo(fd, &tinfo) != -1) {
                        	if (tinfo.servtype == T_CLTS) {
                                	struct timeval timeout;
 
                                	/*
                                 	 * Set time outs for connectionless case
                                 	 */
                                	timeout.tv_usec = 0;
                                	timeout.tv_sec = 15;
                                	(void) CLNT_CONTROL(cp->client,
                                        	CLSET_RETRY_TIMEOUT, &timeout);
                        	}                          
                	} else { 
                        	rpc_createerr.cf_stat = RPC_TLIERROR;
                        	rpc_createerr.cf_error.re_terrno = t_errno;
                        	return (rpc_createerr.cf_stat);
                	}
		}
	}

	tottimeout.tv_sec = t;
	tottimeout.tv_usec = 0;
	clnt_stat = clnt_call(cp->client, procnum, inproc, in,
	    outproc, out, tottimeout);
	if (debug) {
		printf("clnt_stat=%d\n", (int) clnt_stat);
	}

	return ((int) clnt_stat);
}
