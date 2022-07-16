/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:smtp/src/smtpaux.c	1.3"
#ident "@(#)smtpaux.c	1.3 'attmail mail(1) command'"
#include <sys/types.h>		/* needed for socket.h */
#include "s5sysexits.h"
#ifdef	SVR4
#include <sys/uio.h>
#endif
#ifdef SOCKET
#include <sys/socket.h>
#endif
#ifdef SVR3
#include <sys/in.h>		/* WIN/3B */
#include <sys/inet.h>		/* WIN/3B */
#include <netdb.h>
#endif
#ifdef SOCKET
#include <netinet/in.h>
#include <netdb.h>
#endif
#include <stdio.h>
#include <errno.h>
#ifdef	DATAKIT
#include <datakit/dk.h>
#endif
#include "miscerrs.h"
#include "s_string.h"

#ifdef	SVR3
dont_ask()
{
	read();				/* Force read()/write() to be loaded */
	write();			/* from /usr/lib/libnet.a for WIN/3B */
}
#endif

#ifndef SERVNAME
#define SERVNAME	"smtp"		/* service we wanna talk to */
#endif

#ifdef TLI
extern int tli_connect();
#endif

#ifdef SOCKET
#ifdef BIND
#define	TCPconnect	mxconnect
#else
#define	TCPconnect	nomxconnect
#endif
extern int TCPconnect();
#endif

#ifdef DATAKIT
extern int dkconnect();
#endif

struct	conntype {
	char	*prefix;
	int	(*connector)();
} conntype[] = {
	{"tli", tli_connect},
#ifdef SOCKET
	{"tcp", TCPconnect},
#endif
#ifdef	DATAKIT
	{"dk", dkconnect},
#endif
	{NULL, NULL}
};


/*
 * setup -- setup tcp/ip connection to/from server
 */
setup(host, sfip, sfop)
	char *host;
	FILE **sfip, **sfop;
{	int s;
	struct	conntype *ct;
	char *p;

	p = strchr(host, '!');
	if (p) {
		*p = '\0';
		for (ct = conntype; ct->prefix; ct++)
			if (strcmp(ct->prefix, host) == 0)
				break;
		*p++ = '!';
		if (!ct->prefix) ct = conntype;
	}
	else {
		p = host;
		ct = conntype;
	}

	s = (ct->connector)(p);

	if (((*sfip = fdopen(s, "r")) == (FILE *) NULL) ||
	    ((*sfop = fdopen(s, "w")) == (FILE *) NULL)) {
		perror("setup - fdopen");
		bomb(E_CANTOPEN);
	}
#ifdef	u3b2
	setbuf(*sfip, (char *) 0);
#endif
}

#ifndef BIND
#ifdef SOCKET
/*
 * nomxconnect -- open socket to/from server
 */
nomxconnect(host)
	char *host;
{	struct hostent *hp;
	struct servent *sp;
	struct sockaddr_in sin;
	int s, i;
	extern int errno;

	bzero((char *)&sin, sizeof(sin));

	if ((hp = gethostbyname(host)) == (struct hostent *) NULL) {
		(void) fprintf(stderr, "unknown host (%s).\n", host);
		bomb(E_NOHOST);
	}

	for (i=0; hp->h_addr_list[i]; i++) {
		bzero((char *)&sin, sizeof(sin));
		bcopy(hp->h_addr_list[i], &sin.sin_addr, hp->h_length);
	
		if ((sp = getservbyname (SERVNAME, "tcp")) == NULL) {
			(void)fprintf(stderr,"unknown service TCP/%s\n", SERVNAME);
			bomb(E_OSFILE);
		}
		sin.sin_port = sp->s_port;
		sin.sin_family = hp->h_addrtype;
	
		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s < 0) {
			perror("setup - socket");
			bomb(E_CANTOPEN);
		}
	        if (debug) {
	                int debugval = 1;
	
	                if (setsockopt(s, SOL_SOCKET, SO_DEBUG,
	                    (char *)&debugval, sizeof(int)) < 0)
				fprintf(stderr, "Can't set SO_DEBUG\n");
	        }
	
		if (connect(s, (struct sockaddr *) &sin, sizeof (sin)) >= 0)
			return s;
	}
	perror("setup - connect");
	/*
	 * check for conditions that (we think) are temporary;
	 * try them later; bomb utterly on all others.
	 */
	if (errno == ETIMEDOUT || errno == ECONNREFUSED || 
		errno == EHOSTDOWN || errno == EHOSTUNREACH)
		bomb(E_TEMPFAIL);
	else
		bomb(E_CANTOPEN);

}
#endif
#endif

/*
 * bomb(code) - exit program, map smtp error code into mailsystem code
 * Codes with E_ are defined in miscerrs.h.
 * Codes with EX_ are from <sysexits.h>
 * Lines with FOO are placeholders until we decrypt more appropriate codes.
 */
bomb(code)
int code;
{
	switch(code) {
	case 451:			/* host not responding */
		exit(EX_UNAVAILABLE);	/* service unavailable */
		/*NOTREACHED*/
	case 550:			/* no such user */
	case 554:			/* syntax error in address */
	case 501:			/* data format error */
		exit(EX_NOUSER);	/*   == addressee unknown */
		/*NOTREACHED*/
	case E_IOERR:
		exit(EX_IOERR);		/* input/output error */
		/*NOTREACHED*/
	case E_NOHOST:
		exit(EX_NOHOST);	/* host name unknown */
		/*NOTREACHED*/
	case E_OSFILE:			/* no "smtp" -> /etc/services f'd */
		exit(EX_OSFILE);	/* critical OS file missing */
		/*NOTREACHED*/
	case E_USAGE:
		exit(EX_USAGE);		/* command line usage error */
		/*NOTREACHED*/
	case E_TEMPFAIL:
		exit(EX_TEMPFAIL);	/* temp failure; user can retry */
		/*NOTREACHED*/
#ifdef	NOTDEF				/* remainder not used here */
	case FOO:
		exit(EX_OSERR);		/* system error (e.g., can't fork) */
		/*NOTREACHED*/
	case FOO:
		exit(EX_NOINPUT);	/* cannot open input */
		/*NOTREACHED*/
	case FOO:
		exit(EX_CANTCREAT);	/* can't create (user) output file */
		/*NOTREACHED*/
	case FOO:
		exit(EX_PROTOCOL);	/* remote error in protocol */
		/*NOTREACHED*/
	case FOO:
		exit(EX_NOPERM);	/* permission denied */
		/*NOTREACHED*/
#endif	/* NOTDEF */
	default:			/* can't happen? */
		exit(EX_SOFTWARE);	/* internal software error */
		/*NOTREACHED*/
	}
}

#ifdef DATAKIT
dkconnect(host)
	char *host;
{
	extern int dk_verbose, dk_errno;
	static short dkrmode = DKR_BLOCK | DKR_TIME;
	int s;

	dk_verbose = 0;
	s = dkdial(maphost(host, 's', "smtp", "", ""));
	if (s < 0)
		bomb(-dk_errno);

	ioctl(s, DIOCRMODE, &dkrmode);
	return s;
}
#endif


#ifdef TLI
#include <netconfig.h>
#include <netdir.h>
#include <tiuser.h>
#include <fcntl.h>
#include <sys/stropts.h>

int tli_connect(host)
char *host;
{
	struct nd_hostserv	ndh;
	struct netconfig *	ncp;
	struct nd_addrlist *	nap;
	struct t_call *		tcp;
	int			fd;
	void *			handle;
	extern struct netconfig *getnetpath();
	extern void *		setnetpath();

	/* Lookup address of service "smtp" on host `host' (any transport) */
	ndh.h_host = host;
	ndh.h_serv = SERVNAME;

	if ((handle = setnetpath()) != NULL) {
		while ((ncp = getnetpath(handle)) != NULL) {
			if (netdir_getbyname(ncp, &ndh, &nap) != 0)
				continue;

			if (nap->n_cnt <= 0)
				continue;

			if ((fd = t_open(ncp->nc_device, O_RDWR, NULL)) < 0) {
				t_log("can't t_open transport");
				continue;
			}

			if (t_bind(fd, NULL, NULL) < 0) {
				t_log("can't bind address for client");
				t_close(fd);
				continue;
			}

			if ((tcp = (struct t_call *) t_alloc(fd, T_CALL, 0)) == NULL) {
				t_log("can't alloc for client");
				t_close(fd);
				continue;
			}

			tcp->addr = *nap->n_addrs;
			if (t_connect(fd, tcp, NULL) < 0) {
				t_log("t_connect failed");
				t_close(fd);
				continue;
			}

			/* Set up file descriptor for read/write use */
			if (ioctl(fd, I_PUSH, "tirdwr") < 0) {
				t_log("push tirdwr failed");
				t_close(fd);
				continue;
			}

			endnetpath(handle);
			return fd;
		}

		endnetpath(handle);
	}

	/* Not found - fail */
	bomb(451);
	/*NOTREACHED*/
}
#endif
