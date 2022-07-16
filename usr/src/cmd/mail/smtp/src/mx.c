/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:smtp/src/mx.c	1.3"
#ident "@(#)mx.c	1.3 'attmail mail(1) command'"
#ifdef BIND
#include <stdio.h>
#include <netdb.h>
#include <sysexits.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include "miscerrs.h"

/* imports */
extern int errno;
extern char *malloc(), *strcpy(), *inet_ntoa();

/* exports */
int mxconnect();

/* private */
#define MAXMXLIST 10
static struct mxitem {
	char *host;
	u_short pref;
	u_char localflag;
} MXlist[MAXMXLIST + 1];
static char *strsave();
static int buildmxlist();
static void mxsave(), mxinsert(), mxlocal();
static struct hostent *getmxhost();

#ifdef MXMAIN

#define bomb return
#include <resolv.h>

main(argc, argv)
	char **argv;
{	int fd;
	char buf[BUFSIZ], *crlf, *index();
	struct mxitem *mxp;

	_res.options |= RES_DEBUG;
	for (;;) {
		printf("domain: ");
		if (argc > 1)
			strcpy(buf, argv[1]);
		else if (gets(buf) == 0)
			break;
		if ((fd = mxconnect(buf)) >= 0)
			if (read(fd, buf, 512) > 0) {
				if ((crlf = index(buf, '\r')) != 0)
					strcpy(crlf, "\n");
				puts(buf);
			} else
				perror("read");
		close(fd);
		if (argc > 1)
			break;
		for (mxp = MXlist; mxp < MXlist + MAXMXLIST + 1; mxp++)
			mxp->host = 0;
	}
	return 0;
}
#endif

mxconnect(host)
	char *host;
{	int s, lport, mxfatal;
	char **addr, errbuf[256];
	struct hostent *hp;
	struct servent *sp;
	struct sockaddr_in sin;
	struct mxitem *mxp;
	extern debug;

	mxfatal = buildmxlist(host);
	if (MXlist[0].host == 0)
		MXlist[0].host = host;
	if ((sp = getservbyname ("smtp", "tcp")) == NULL) {
		(void)fprintf(stderr,"unknown service TCP/smtp\n");
		bomb(E_OSFILE);
	}
	(void) setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *) 0, 0);

	/* slop in the loop -- i hate the socket dance */
	for (mxp = MXlist; mxp->host; mxp++) {
		int i;

		if ((hp = getmxhost(mxp->host)) == 0) {
			if (mxfatal)
				bomb(E_NOHOST);
			continue;
		}
		for (i=0; hp->h_addr_list[i]; i++) {
			s = socket(AF_INET, SOCK_STREAM, 0);
			if (s < 0) {
				perror("socket");
				bomb(E_CANTOPEN);
			}
			bzero((char *)&sin, sizeof(sin));
			sin.sin_port = sp->s_port;
			sin.sin_family = hp->h_addrtype;
			bcopy(hp->h_addr_list[i], (char *) &sin.sin_addr,
				hp->h_length);
			if (debug)
				fprintf(stderr, "try %s [%s]", mxp->host,
				     inet_ntoa(sin.sin_addr));
			if (connect(s, (struct sockaddr *)&sin, sizeof(sin))>=0) {
				if (debug)
					fprintf(stderr, " connected\n");
				return s;
			}
			if (debug)
				fprintf(stderr, "NG, err %d\n", errno);
			close(s);
		}
		sprintf(errbuf, "%s [%s]", mxp->host, inet_ntoa(sin.sin_addr));
		perror(errbuf);
		close(s);
	}

	bomb(E_TEMPFAIL);
}

/* return 1 for fatal MX error (authoritative NXDOMAIN), 0 o.w. */
static int
buildmxlist(host)
	char *host;
{	register HEADER *hp;
	register char *cp;
	register int n;
	char q[PACKETSZ], a[PACKETSZ];	/* query, answer */
	char *eom, *bp;
	int buflen, ancount, qdcount;
	char hostbuf[BUFSIZ+1];
	u_short preference, reclen;
	int niter = 0;
	char nhostbuf[BUFSIZ+1];

again:
	if ((n = res_mkquery(QUERY, host, C_IN, T_MX, (char *) 0, 0, (struct rrec *) 0, q, sizeof(q))) < 0)
		return 0;
	n = res_send(q, n, a, sizeof(a));
	if (n < 0)
		return 0;
	if (debug)
		fprintf(stderr, "buildmxlist got %d\n", n);
	eom = a + n;
	hp = (HEADER *) a;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);
	if (debug)
		fprintf(stderr, "buildmx rcode %d ancount %d qdcount %d aa %d\n",
		 hp->rcode, ancount, qdcount, hp->aa);
	if (hp->rcode != NOERROR || ancount == 0) {
		if (hp->aa == 0)
			return 0;	/* non-authoritative in any event */
		return hp->rcode == NOERROR || hp->rcode == NXDOMAIN;
	}
	bp = hostbuf;
	buflen = sizeof(hostbuf);
	cp = a + sizeof(HEADER);
	if (debug>1) {
		int i;
		fprintf(stderr, "buildmx got:");
		for (i=0; i<n; i++)
			if (a[i]>=' '&& a[i]<0177)
				fprintf(stderr, "%c", a[i]);
			else fprintf(stderr, "\\%.3o", a[i]&0377);
		fprintf(stderr, "\n");
	}
	while (--qdcount >= 0)
		cp += dn_skipname(cp,eom) + QFIXEDSZ;
	/* TODO: if type is CNAME, reissue query */
	while (--ancount >= 0 && cp < eom) {
		int type;
		cp += dn_skipname(cp,eom);	/* name */
		type = _getshort(cp);	
		cp += sizeof(u_short)	/* type */
		    + sizeof(u_short)	/* class */
		    + sizeof(u_long);	/* ttl (see rfc973) */
		reclen = _getshort(cp);
		cp += sizeof(u_short);
		if (type==T_CNAME) {	/* canonical name */
			if (dn_expand(a, eom, cp, nhostbuf, BUFSIZ) < 0)
				if (debug)
					fprintf(stderr, "CNAME? in mxexpand\n");
			host = nhostbuf;
			if (++niter>10)
				return 0;
			if (debug)
				fprintf(stderr, "CNAME, try with %s\n", host);
			goto again;
		}
		preference = _getshort(cp);
		if ((n = dn_expand(a, eom, cp + sizeof(u_short), bp, buflen)) < 0)
			break;
		mxsave(bp, preference);
		cp += reclen;
	}
	mxlocal();
	return 0;
}

/* NOT TODO: issue WKS query.  (just try to connect.) */

static void
mxsave(host, pref)
	char *host;
	u_short pref;
{	struct mxitem *mxp;
	int localflag;
	static char thishost[64];

	if (*thishost == 0)
		gethostname(thishost, sizeof(thishost));
	if (debug)
		fprintf(stderr, "MXsave %s\n", host);

	if (MXlist[MAXMXLIST].host)
		return;				/* full */

	localflag = (strcmp(thishost, host) == 0);

	/* insertion sort */
	for (mxp = MXlist; mxp < MXlist + MAXMXLIST; mxp++) {
		if (mxp->host == 0) {
			mxinsert(mxp, host, pref, localflag);
			return;
		}
		if (pref < mxp->pref) {
			mxinsert(mxp, host, pref, localflag);
			return;
		}
		if (pref == mxp->pref) {
			if (mxp->localflag)
				return;
			if (localflag) {
				mxp->host = strsave(host);
				mxp->pref = pref;
				mxp->localflag = localflag;
				(++mxp)->host = 0;
				return;
			}
			mxinsert(mxp, host, pref, localflag);
			return;
		}
	}
}

static void
mxinsert(mxlistp, host, pref, localflag)
	struct mxitem *mxlistp;
	char *host;
	u_short pref;
{	register struct mxitem *mxp;

	for (mxp = MXlist + MAXMXLIST - 1; mxp > mxlistp; --mxp)
		*mxp = mxp[-1];
	mxp->host = strsave(host);
	mxp->pref = pref;
	mxp->localflag = localflag;
}

static char *
strsave(str)
	register char *str;
{	register char *rval;

	if ((rval = malloc(strlen(str) + 1)) == 0) {
		perror("malloc");
		bomb(-EX_SOFTWARE);
	}
	strcpy(rval, str);
	return rval;
}

static void
mxlocal()
{	register struct mxitem *mxp;

	if (MXlist[0].host == 0)
		return;

	for (mxp = MXlist; mxp->host; mxp++) {
		if (mxp->localflag) {
			mxp->host = 0;
			break;
		}
	}
}

static struct hostent *
getmxhost(host)
	char *host;
{	struct hostent *hp, *gethostbyname(), *gethostbyaddr();
	long inet_addr();
	static struct in_addr ia;
	static char *hlist[2] = {0, 0};
	static struct hostent he = {0, 0, AF_INET, sizeof(struct in_addr), hlist};

	if (!host)
		return 0;

	if ((hp = gethostbyname(host)) != 0)
		return hp;

	if ((ia.s_addr = inet_addr(host)) != -1) {
		he.h_addr = (char *)&ia;
		return &he;
	}
	(void) fprintf(stderr, "Error looking up Host \"%s\".\n", host);
	return 0;
}

#endif

