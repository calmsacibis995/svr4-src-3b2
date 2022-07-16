/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/udp.c	1.3"
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
 * this file consists of routines to support call_udp();
 * client handles are cached in a hash table;
 * clntudp_create is only called if (site, prog#, vers#) cannot
 * be found in the hash table;
 * a cached entry is destroyed, when remote site crashes
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <netdb.h>

#define MAX_HASHSIZE 100

char *malloc();
char *xmalloc();
static int mysock = RPC_ANYSOCK;
extern int debug;
extern int HASH_SIZE;
extern int bindresvport();
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

call_udp(host, prognum, versnum, procnum, inproc, in, outproc, out, valid_in, t)
	char *host;
	u_long prognum, versnum;
	xdrproc_t inproc, outproc;
	char *in, *out;
	int valid_in;
	int t;
{
	struct sockaddr_in server_addr;
        enum clnt_stat clnt_stat;
        struct hostent *hp;
        struct timeval timeout, tottimeout;
        struct cache *cp;
        int port = 0;

	if (debug)
		printf("enter call_udp() ...\n");

	if ((cp = find_hash(host, prognum, versnum)) == (struct cache *)NULL) {
		if ((cp = add_hash(host, prognum, versnum)) == (struct cache *)NULL) {
			fprintf(stderr, "udp cannot send due to out of cache\n");
			return (-1);
		}
		if (debug)
			printf("(%x):[%s, %d, %d] is a new connection\n",
				cp, host, prognum, versnum);

		if ((hp = gethostbyname(host)) == NULL) {
                        delete_hash (host);
                        return ((int) RPC_UNKNOWNHOST);
                }
 
                timeout.tv_usec = 0;
                timeout.tv_sec = 15;
                memcpy((char *)&server_addr.sin_addr, (char *)hp->h_addr,
                        hp->h_length);
 
                server_addr.sin_family = AF_INET;
                server_addr.sin_port =  0;
                cp->client = (CLIENT *)clntudp_create(&server_addr, prognum,
                        versnum, timeout, &mysock);
                if (cp->client == (CLIENT *)NULL) {
                        perror("clntudp_create");
                        delete_hash (host);
                        return ((int) rpc_createerr.cf_stat);
                }
	}
	else {
		if (valid_in == 0) { /* cannot use cache */
			if (debug)
				printf("(%x):[%s, %d, %d] is a new connection\n",
					cp, host, prognum, versnum);

			if ((hp = gethostbyname(host)) == NULL) {
                                delete_hash (host);
                                return ((int) RPC_UNKNOWNHOST);
                        }
 
                        /* get rid of previous client struct */
                        if (cp->client != (CLIENT *)NULL)
                                clnt_destroy(cp->client);
                        timeout.tv_usec = 0;
                        timeout.tv_sec = 15;
                        memcpy((char *)&server_addr.sin_addr,
                                (char *)hp->h_addr, hp->h_length);
 
                        server_addr.sin_family = AF_INET;
                        server_addr.sin_port =  0;
                        cp->client = (CLIENT *)clntudp_create(&server_addr,
                                prognum, versnum, timeout, &mysock);
                        if (cp->client == (CLIENT *)NULL) {
                                delete_hash (host);
                                return ((int) rpc_createerr.cf_stat);
                        }
		}
	}

	tottimeout.tv_sec = t;
	tottimeout.tv_usec = 0;
	clnt_stat = clnt_call(cp->client, procnum, inproc, in,
	    outproc, out, tottimeout);
	if (debug) {
		if (clnt_stat != RPC_SUCCESS && clnt_stat != RPC_TIMEDOUT) {
			clnt_perror(cp->client, "Call failed.");
		}
	}

	return ((int) clnt_stat);
}
