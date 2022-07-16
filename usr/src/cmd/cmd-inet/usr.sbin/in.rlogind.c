/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:usr.sbin/in.rlogind.c	1.9"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

/*
 * remote login server:
 *	remuser\0
 *	locuser\0
 *	terminal info\0
 *	data
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/file.h>
#ifdef SYSV
#include <sys/stream.h>
#include <sys/stropts.h>
#endif SYSV

#include <netinet/in.h>

#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <syslog.h>
#include <string.h>
#include <utmp.h>
#ifdef SYSV
#include <sac.h>	/* for SC_WILDC */
#include <utmpx.h>
#include <sys/filio.h>
#include <sys/ioctl.h>

#define bzero(s,n)	  memset((s), 0, (n))
#define bcopy(a,b,c)	  memcpy(b,a,c)
#define signal(s,f)	  sigset(s,f)

/* 
 * XXX - can't include both termios.h and ioctl.h so here's a few defines 
 * from termios.h.  This stuff will go away as soon as we figure out how
 * both include files can live harmoniously together.
 */
#define	VSTART		8
#define	VSTOP		9

/* input modes */
#define	IXON	0002000

#define	TIOC	('T'<<8)

#define	TCGETA	(TIOC|1)
#define	TCSETA	(TIOC|2)
#define	TCSETAW	(TIOC|3)
#define	TCSETAF	(TIOC|4)

/* termios ioctls */

#define TCGETS		(TIOC|13)
#define TCSETS		(TIOC|14)
#define TCSANOW		(TIOC|14) /* same as TCSETS */
#define TCSETSW		(TIOC|15)
#define TCSADRAIN	(TIOC|15) /* same as TCSETSW */
#define	TCSETSF		(TIOC|16)
#define	TCSADFLUSH	(TIOC|16) /* same as TCSETSF */

#define OCRNL   0000010
#define IGNPAR  0000004
#define ICRNL   0000400
#define ICANON  0000002

#define NCCS    19
typedef unsigned char cc_t;
typedef unsigned long tcflag_t;
/*
 * Ioctl control packet
 */
struct termios {
	tcflag_t	c_iflag;	/* input modes */
	tcflag_t	c_oflag;	/* output modes */
	tcflag_t	c_cflag;	/* control modes */
	tcflag_t	c_lflag;	/* line discipline modes */
	cc_t		c_cc[NCCS];	/* control chars */
};

#define       NCC     8
/*
 * Ioctl control packet
 */
struct termio {
	unsigned short	c_iflag;	/* input modes */
	unsigned short	c_oflag;	/* output modes */
	unsigned short	c_cflag;	/* control modes */
	unsigned short	c_lflag;	/* line discipline modes */
	char	c_line;		/* line discipline */
	unsigned char	c_cc[NCC];	/* control chars */
};

#endif SYSV

# ifndef TIOCPKT_WINDOW
# define TIOCPKT_WINDOW 0x80
# endif TIOCPKT_WINDOW

#ifdef SYSV
/* XXX - These should be in a header file somewhere */
#define         TIOCPKT_FLUSHWRITE      0x02    /* flush data read from
						   controller but not yet
						   processed */
#define         TIOCPKT_NOSTOP          0x10    /* no more ^S, ^Q */
#define         TIOCPKT_DOSTOP          0x20    /* now do ^S, ^Q */
#endif SYSV


extern	errno;
int	reapchild();
struct	passwd *getpwnam();
char	*malloc();

main(argc, argv)
	int argc;
	char **argv;
{
	int on = 1, options = 0, fromlen;
	struct sockaddr_in from;

	openlog("rlogind", LOG_PID | LOG_AUTH, LOG_AUTH);
	fromlen = sizeof (from);
	if (getpeername(0, &from, &fromlen) < 0) {
		fprintf(stderr, "%s: ", argv[0]);
		perror("getpeername");
		_exit(1);
	}
	if (setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof (on)) < 0) {
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");
	}
	doit(0, &from);
	/* NOTREACHED */
}

int	cleanup();
int	netf;
extern	errno;
char	*line;
extern	char	*inet_ntoa();
#ifdef SYSV
int	stopmode = TIOCPKT_DOSTOP;
char	stopc = CTRL ('s');
char	startc = CTRL('q');
#endif SYSV

struct winsize win = { 0, 0, 0, 0 };
pid_t pid;

doit(f, fromp)
	int f;
	struct sockaddr_in *fromp;
{
	int i, p, t, on = 1;
	register struct hostent *hp;
	struct hostent hostent;
	char c;
#ifdef SYSV
	extern char *ptsname();
	struct termios tp;
#endif SYSV

	alarm(60);
	read(f, &c, 1);
	if (c != 0)
		exit(1);
	alarm(0);
	fromp->sin_port = ntohs((u_short)fromp->sin_port);
	hp = gethostbyaddr(&fromp->sin_addr, sizeof (struct in_addr),
		fromp->sin_family);
	if (hp == 0) {
		/*
		 * Only the name is used below.
		 */
		hp = &hostent;
		hp->h_name = inet_ntoa(fromp->sin_addr);
	}
	if (fromp->sin_family != AF_INET ||
	    fromp->sin_port >= IPPORT_RESERVED ||
	    fromp->sin_port < (u_short) IPPORT_RESERVED/2)
		fatal(f, "Permission denied");
	write(f, "", 1);
#ifdef SYSV
	if ((p = open("/dev/ptmx", O_RDWR)) == -1) {
		fatalperror (f,"open /dev/ptmx", errno);
	}
	if (grantpt(p) == -1)
		fatal(f, "could not grant slave pty");
	if (unlockpt(p) == -1)
		fatal(f, "could not unlock slave pty");
	if ((line = ptsname(p)) == NULL)
		fatal (f, "could not enable slave pty");
	if ((t = open (line, O_RDWR)) == -1)
		fatal (f, "could not open slave pty");
	if (ioctl (t, I_PUSH, "ptem") == -1)
		fatalperror (f, "ioctl I_PUSH ptem", errno);
	if (ioctl (t, I_PUSH, "ldterm") == -1)
		fatalperror (f, "ioctl I_PUSH ldterm", errno);
	if (ioctl (t, I_PUSH, "ttcompat") == -1)
		fatalperror (f, "ioctl I_PUSH ttcompat", errno);
	if (ioctl (p, I_PUSH, "pckt") == -1)
		fatalperror (f, "ioctl I_PUSH pckt", errno);
	/* 
	 * Make sure the pty doesn't modify the strings passed
	 * to login as part of the "rlogin protocol."  The login
	 * program should set these flags to apropriate values
	 * after it has read the strings.
	 */
	if (ioctl (t, TCGETS, &tp) == -1)
		fatalperror(f, "ioctl TCGETS", errno);
	tp.c_lflag &= ~(ECHO|ICANON);
	tp.c_oflag &= ~(XTABS|OCRNL);
	tp.c_iflag &= ~(IGNPAR|ICRNL);
	if (ioctl (t, TCSETS, &tp) == -1)
		fatalperror(f, "ioctl TCSETS", errno);

#else
	for (c = 'p'; c <= 's'; c++) {
		struct stat stb;
		line = "/dev/ptyXX";
		line[strlen("/dev/pty")] = c;
		line[strlen("/dev/ptyp")] = '0';
		if (stat(line, &stb) < 0)
			break;
		for (i = 0; i < 16; i++) {
			line[strlen("/dev/ptyp")] = "0123456789abcdef"[i];
			p = open(line, 2);
			if (p > 0)
				goto gotpty;
		}
	}
	fatal(f, "Out of ptys");
	/*NOTREACHED*/
gotpty:
	line[strlen("/dev/")] = 't';
#ifdef DEBUG
	{ int tt = open("/dev/tty", 2);
	  if (tt > 0) {
		ioctl(tt, TIOCNOTTY, 0);
		close(tt);
	  }
	}
#endif
	t = open(line, 2);
	if (t < 0)
		fatalperror(f, line, errno);
	{ struct sgttyb b;
	  int zero = 0;
	  gtty(t, &b); b.sg_flags = RAW|ANYP; stty(t, &b);
	  /*
	   * Turn off PASS8 mode, since "login" no longer does so.
	   */
	  ioctl(t, TIOCLSET, &zero);
	}
#endif SYSV
	/*
	 * System V ptys allow the TIOC{SG}WINSZ ioctl to be
	 * issued on the master side of the pty.  Luckily, that's
	 * the only tty ioctl we need to do do, so we can close the
	 * slave side in the parent process after the fork.
	 */
	(void) ioctl(p, TIOCSWINSZ, &win);
	netf = f;
	pid = fork();
	if (pid < 0)
		fatalperror(f, "", errno);
	if (pid == 0) {
#ifdef SYSV
		int tt;
		struct utmpx ut;

		/* System V login expects a utmp entry to already be there */
		bzero ((char *) &ut, sizeof (ut));
		(void) strncpy(ut.ut_user, ".rlogin", sizeof(ut.ut_user));
		(void) strncpy(ut.ut_line, line, sizeof(ut.ut_line));
		ut.ut_pid = (o_pid_t)getpid();
		ut.ut_id[0] = 'r';
		ut.ut_id[1] = 'l';
		ut.ut_id[2] = SC_WILDC;
		ut.ut_id[3] = SC_WILDC;
		ut.ut_type = LOGIN_PROCESS;
		ut.ut_exit.e_termination = 0;
		ut.ut_exit.e_exit = 0;
		(void) time (&ut.ut_tv.tv_sec);
		if (makeutx(&ut) == NULL)
			syslog(LOG_INFO, "in.rlogind:\tmakeutx failed");

		/* controlling tty */
		if ( setsid() == -1 )
			fatalperror(f, "setsid", errno);
		if ((tt = open (line, O_RDWR)) == -1)
			fatalperror(f, "could not reopen slave pty", errno);

		close(f), close(p), close(t);
		dup2(tt, 0), dup2(tt, 1), dup2(tt, 2);
		close(tt);
#else
		close(f), close(p);
		dup2(t, 0), dup2(t, 1), dup2(t, 2);
		close(t);
#endif /* SYSV */
		execl("/bin/login", "login", "-r", hp->h_name, (char *)0);
		fatalperror(2, "/bin/login", errno);
		/*NOTREACHED*/
	}
	close(t);
	ioctl(f, FIONBIO, &on);
	ioctl(p, FIONBIO, &on);
#ifndef SYSV
	ioctl(p, TIOCPKT, &on);
#endif
	signal(SIGTSTP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGCHLD, (void (*)()) cleanup);
	setpgrp(0, 0);
	protocol(f, p);
	cleanup();
}

char	magic[2] = { 0377, 0377 };
char	oobdata[] = {TIOCPKT_WINDOW};

/*
 * Handle a "control" request (signaled by magic being present)
 * in the data stream.  For now, we are only willing to handle
 * window size changes.
 */
control(pty, cp, n)
	int pty;
	char *cp;
	int n;
{
	struct winsize w;

	if (n < 4+sizeof (w) || cp[2] != 's' || cp[3] != 's')
		return (0);
	oobdata[0] &= ~TIOCPKT_WINDOW;	/* we know he heard */

	bcopy(cp+4, (char *)&w, sizeof(w));
	w.ws_row = ntohs(w.ws_row);
	w.ws_col = ntohs(w.ws_col);
	w.ws_xpixel = ntohs(w.ws_xpixel);
	w.ws_ypixel = ntohs(w.ws_ypixel);
	(void)ioctl(pty, TIOCSWINSZ, &w);
	return (4+sizeof (w));
}

/*
 * rlogin "protocol" machine.
 */
protocol(f, p)
	int f, p;
{
	char pibuf[1024], fibuf[1024], *pbp, *fbp;
	register pcc = 0, fcc = 0;
	int cc, wsize;
	char cntl;

	/*
	 * Must ignore SIGTTOU, otherwise we'll stop
	 * when we try and set slave pty's window shape
	 * (our controlling tty is the master pty).
	 */
	(void) signal(SIGTTOU, SIG_IGN);
	send(f, oobdata, 1, MSG_OOB);	/* indicate new rlogin */
	for (;;) {
		int ibits, obits, ebits;

		ibits = 0;
		obits = 0;
		if (fcc)
			obits |= (1<<p);
		else
			ibits |= (1<<f);
		if (pcc >= 0)
			if (pcc)
				obits |= (1<<f);
			else
				ibits |= (1<<p);
#ifdef SYSV
		if (select(16, &ibits, &obits, 0, 0) < 0) {
#else
		ebits = (1<<p);
		if (select(16, &ibits, &obits, &ebits, 0) < 0) {
#endif SYSV
			if (errno == EINTR)
				continue;
			fatalperror(f, "select", errno);
		}
		if (ibits == 0 && obits == 0 && ebits == 0) {
			/* shouldn't happen... */
			sleep(5);
			continue;
		}
#ifndef SYSV
#define	pkcontrol(c)	((c)&(TIOCPKT_FLUSHWRITE|TIOCPKT_NOSTOP|TIOCPKT_DOSTOP))
		if (ebits & (1<<p)) {
			cc = read(p, &cntl, 1);
			if (cc == 1 && pkcontrol(cntl)) {
				cntl |= oobdata[0];
				send(f, &cntl, 1, MSG_OOB);
				if (cntl & TIOCPKT_FLUSHWRITE) {
					pcc = 0;
					ibits &= ~(1<<p);
				}
			}
		}
#endif SYSV
		if (ibits & (1<<f)) {
			fcc = read(f, fibuf, sizeof (fibuf));
			if (fcc < 0 && errno == EWOULDBLOCK)
				fcc = 0;
			else {
				register char *cp;
				int left, n;

				if (fcc <= 0)
					break;
				fbp = fibuf;

			top:
				for (cp = fibuf; cp < fibuf+fcc-1; cp++)
					if (cp[0] == magic[0] &&
					    cp[1] == magic[1]) {
						left = fcc - (cp-fibuf);
						n = control(p, cp, left);
						if (n) {
							left -= n;
							if (left > 0)
								bcopy(cp+n, cp, left);
							fcc -= n;
							goto top; /* n^2 */
						} /* if (n) */
					} /* for (cp = ) */
				} /* else */
		} /* if (ibits & (1<<f)) */

		if ((obits & (1<<p)) && fcc > 0) {
			wsize = fcc;
			do {
				cc = write(p, fbp, wsize);
				wsize /= 2;
			} while (cc<0 && errno==EWOULDBLOCK && wsize);
			if (cc > 0) {
				fcc -= cc;
				fbp += cc;
			}
		}

		if (ibits & (1<<p)) {
#ifdef SYSV
			pcc = pcktread(p, pibuf, sizeof(pibuf), f);
#else
			pcc = read(p, pibuf, sizeof (pibuf));
#endif SYSV
			pbp = pibuf;
			if (pcc < 0 && errno == EWOULDBLOCK)
				pcc = 0;
#ifndef SYSV
			else if (pcc <= 0)
				break;
#else
			else if (pcc == 0) {
				syslog(LOG_WARNING, "pcktread : suspect zero len");
			}
			else if (pcc < 0) {
				syslog(LOG_WARNING, "pcktread : %m");
				break;
			}
#endif
#ifndef SYSV
			else if (pibuf[0] == 0)
				pbp++, pcc--;
			else {
				if (pkcontrol(pibuf[0])) {
					pibuf[0] |= oobdata[0];
					send(f, &pibuf[0], 1, MSG_OOB);
					
				}
				pcc = 0;
			}
#endif SYSV
		}
		if ((obits & (1<<f)) && pcc > 0) {
			wsize = pcc;
			do {
				cc = write(f, pbp, wsize);
				wsize /= 2;
			} while (cc<0 && errno==EWOULDBLOCK && wsize);
			if (cc > 0) {
				pcc -= cc;
				pbp += cc;
			}
		}
	}
}


#ifdef SYSV
/*
 * Read from a System V pty in "packet mode."  In this mode, we get
 * all streams messages sent down the slave side.  The come up with
 * the streams message type in the control part of the message we read.
 * Because we never know whether what we read will be a streams message
 * or just some normal data until after we have read it, we have to use
 * getmsg() for all reads.
 *
 * To be almost plug-compatible with read, we return 0 only when we
 * get a zero length data message.  This indicates that last user
 * has closed the slave side of the pty.  In all other cases where
 * we got a message, but no user data, we return -1 and EWOULDBLOCK
 * so that the caller will ignore it.
 */
int
pcktread(pty, buf, size, soc)
	int pty;
	char *buf;
	int size, soc;
{
	struct strbuf ctlbuf;
	struct strbuf databuf;
	u_char ctlstuff;
	int flags = 0;
	int ret;
	char cntl;
	struct termios *tp;
	struct termio *ti;
	struct iocblk *ioc;
	int stop;
	int ixon;

	bzero((char *) &ctlbuf, sizeof (ctlbuf));
	bzero((char *) &databuf, sizeof (databuf));
	ctlbuf.maxlen = sizeof(ctlstuff);
	ctlbuf.buf = (char *) &ctlstuff;
	databuf.maxlen = size;
	databuf.buf = buf;

	if ((ret = getmsg(pty, &ctlbuf, &databuf, &flags)) == -1) {
		syslog(LOG_INFO, "pcktread: getmsg: %m");
		return (-1);
	}

	if (ctlbuf.len <= 0) {
		/* no control part, just normal data */
#ifdef DEBUG
		syslog(LOG_INFO, "pcktread: data only %d bytes", databuf.len);
#endif DEBUG
		return (databuf.len);
	}

	if (ret & MORECTL) {
		syslog(LOG_INFO, "pcktread: control buf too big: %d bytes",
		       ctlbuf.len);
		errno = EWOULDBLOCK;
		return (-1);
	}

	if ((ctlstuff != M_DATA) && (ret & MOREDATA)) {
		syslog(LOG_INFO, "pcktread: proto data buf too big: %d bytes",
		       databuf.len);
		errno = EWOULDBLOCK;
		return (-1);
	}

	if (ctlbuf.len != sizeof (ctlstuff)) {
		/* packet mode is broken */
		syslog(LOG_INFO, "pcktread: control buf len wrong: %d bytes", 
		       ctlbuf.len);
		errno = EWOULDBLOCK;
		return (-1);
	}

	switch (ctlstuff) {
	case M_DATA:
#ifdef DEBUG
		syslog(LOG_INFO, "pcktread: M_DATA %d bytes", databuf.len);
#endif DEBUG
		return (databuf.len);
		break;

	case M_FLUSH:
#ifdef DEBUG
		syslog(LOG_INFO, "pcktread: M_FLUSH");
#endif DEBUG
		if (databuf.len <= 0) {
			syslog(LOG_INFO, "pcktread: M_FLUSH data len = %d",
			       databuf.len);
			errno = EWOULDBLOCK;
			return (-1);
		}
		if (databuf.buf[0] & FLUSHW) {
			cntl =  oobdata[0] | TIOCPKT_FLUSHWRITE;
			send(soc, &cntl, 1, MSG_OOB);
#ifdef DEBUG
			syslog(LOG_INFO, "pcktread: sent TIOCPKT_FLUSHWRITE");
#endif DEBUG
		}
		errno = EWOULDBLOCK;	
		return (-1);
		break;

	case M_IOCTL:
		ioc = (struct iocblk *) databuf.buf;

		if (databuf.len < sizeof (struct iocblk)) {
			syslog(LOG_INFO, "pcktread: M_IOCTL data len = %d",
			       databuf.len);
			errno = EWOULDBLOCK;
			return (-1);
		}

		/*
		 * If it is a tty ioctl, save the output flow 
		 * control flag and the start and stop flow control
		 * characters if they are available.
		 */
		switch (ioc->ioc_cmd) {
		case TCSETS:
		case TCSETSW:
		case TCSETSF:
#ifdef DEBUG
			syslog(LOG_INFO, "pcktread: M_IOCTL TCSETS 0x%x", 
			       ioc->ioc_cmd);
#endif DEBUG
			if (databuf.len < (sizeof (struct termios)
					       + sizeof (struct iocblk))) {
				syslog(LOG_INFO, 
				       "pcktread: M_IOCTL len = %d",
				       databuf.len);
				errno = EWOULDBLOCK;
				return (-1);
			}
			tp = (struct termios *) 
				(databuf.buf + sizeof (struct iocblk));
			stopc = tp->c_cc[VSTOP];
			startc = tp->c_cc[VSTART];
			ixon = tp->c_iflag & IXON;
			break;

		case TCSETA:
		case TCSETAW:
		case TCSETAF:
#ifdef DEBUG
			syslog(LOG_INFO, "pcktread: M_IOCTL TCSETA 0x%x", 
			       ioc->ioc_cmd);
#endif DEBUG
			if (databuf.len < (sizeof (struct termio)
					       + sizeof (struct iocblk))) {
				syslog(LOG_INFO, 
				       "pcktread: M_IOCTL len = %d",
				       databuf.len);
				errno = EWOULDBLOCK;
				return (-1);
			}
			ti = (struct termio *)
				(databuf.buf + sizeof (struct iocblk));
			ixon = ti->c_iflag & IXON;
			break;

 		default:
			/* its some random ioctl that we're not interested in */
#ifdef DEBUG
			syslog(LOG_INFO, "pcktread: M_IOCTL 0x%x", 
			       ioc->ioc_cmd);
#endif DEBUG
			errno = EWOULDBLOCK;
			return (-1);
			break;
		} /* switch (ioc->ioc_cmd) */

		/*
		 * Now decide it the tty ioctl represents
		 * a state change that we need to tell the client
		 * about.
		 */
		stop = (ixon && (stopc == CTRL('s')) && (startc == CTRL('q')));

		if (stopmode == TIOCPKT_NOSTOP) {
			if (stop) {
				cntl =  oobdata[0] | TIOCPKT_DOSTOP;
				send(soc, &cntl, 1, MSG_OOB);
				stopmode = TIOCPKT_DOSTOP;
#ifdef DEBUG
				syslog(LOG_INFO, 
				       "pcktread: sent TIOCPKT_DOSTOP");
#endif DEBUG
			} 
		} else {
			if (!stop) {
				cntl =  oobdata[0] | TIOCPKT_NOSTOP;
				send(soc, &cntl, 1, MSG_OOB);
				stopmode = TIOCPKT_NOSTOP;
#ifdef DEBUG
				syslog(LOG_INFO, 
				       "pcktread: sent TIOCPKT_NOSTOP");
#endif DEBUG
			}
		}
		errno = EWOULDBLOCK;
		return (-1);
		break; /* case M_IOCTL */

	default:
		/* some random message that we're not interested in */
#ifdef DEBUG
		syslog(LOG_INFO, "pcktread: msg type 0x%x", (u_int) ctlstuff);
#endif DEBUG
		errno = EWOULDBLOCK;
		return (-1);
		break;
	}
	/*NOTREACHED*/
}
#endif SYSV
	




cleanup()
{

	rmut();
#ifndef SYSV
	vhangup();
#endif /* SYSV */
	shutdown(netf, 2);
	exit(1);
}

fatal(f, msg)
	int f;
	char *msg;
{
	char buf[BUFSIZ];

	buf[0] = '\01';		/* error indicator */
	(void) sprintf(buf + 1, "rlogind: %s.\r\n", msg);
	(void) write(f, buf, strlen(buf));
	exit(1);
}

fatalperror(f, msg, errno)
	int f;
	char *msg;
	int errno;
{
	char buf[BUFSIZ];
	extern int sys_nerr;
	extern char *sys_errlist[];

	if ((unsigned)errno < sys_nerr)
		(void) sprintf(buf, "%s: %s", msg, sys_errlist[errno]);
	else
		(void) sprintf(buf, "%s: Error %d", msg, errno);
	fatal(f, buf);
}


#ifdef SYSV
rmut()
{
	struct utmpx		*up;

	signal(SIGCHLD, SIG_IGN); /* while cleaning up don't allow disruption */

	setutxent();
	while ( (up = getutxent()) ) {
		if (up->ut_pid != (o_pid_t)pid)
			continue;
		up->ut_type = DEAD_PROCESS;
		up->ut_exit.e_termination = 0;
		up->ut_exit.e_exit = 0;
		(void) time (&up->ut_tv.tv_sec);
		if (modutx(up) == NULL)
			syslog(LOG_INFO, "in.rlogind:\tmodutx failed");
		break;
	}
	endutxent();
	signal(SIGCHLD, (void (*)())cleanup);
}

#else /* !SYSV */

#define SCPYN(a, b)	strncpy(a, b, sizeof(a))
#define SCMPN(a, b)	strncmp(a, b, sizeof(a))

rmut()
{
	register f;
	int found = 0;
	struct utmp *u, *utmp;
	int nutmp;
	struct stat statbf;
	struct	utmp wtmp;
	char	wtmpf[]	= WTMP_FILE;
	char	utmpf[] = UTMP_FILE;

	signal(SIGCHLD, SIG_IGN); /* while cleaning up don't allow disruption */
	f = open(utmpf, O_RDWR);
	if (f >= 0) {
		fstat(f, &statbf);
		utmp = (struct utmp *)malloc(statbf.st_size);
		if (!utmp)
			syslog(LOG_ERR, "utmp malloc failed");
		if (statbf.st_size && utmp) {
			nutmp = read(f, utmp, statbf.st_size);
			nutmp /= sizeof(struct utmp);
		
			for (u = utmp ; u < &utmp[nutmp] ; u++) {
				if (SCMPN(u->ut_line, line+5) ||
				    u->ut_name[0]==0)
					continue;
				lseek(f, ((long)u)-((long)utmp), L_SET);
				SCPYN(u->ut_name, "");
				SCPYN(u->ut_host, "");
				time(&u->ut_time);
				write(f, (char *)u, sizeof(wtmp));
				found++;
			}
		}
		close(f);
	}
	if (found) {
		f = open(wtmpf, O_WRONLY|O_APPEND);
		if (f >= 0) {
			SCPYN(wtmp.ut_line, line+5);
			SCPYN(wtmp.ut_name, "");
			SCPYN(wtmp.ut_host, "");
			time(&wtmp.ut_time);
			write(f, (char *)&wtmp, sizeof(wtmp));
			close(f);
		}
	}
	chmod(line, 0666);
	chown(line, 0, 0);
	line[strlen("/dev/")] = 'p';
	chmod(line, 0666);
	chown(line, 0, 0);
	signal(SIGCHLD, (void (*)())cleanup);
}
#endif /* SYSV */
