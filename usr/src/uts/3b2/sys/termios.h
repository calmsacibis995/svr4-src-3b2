/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_TERMIOS_H
#define _SYS_TERMIOS_H

#ident	"@(#)head.sys:sys/termios.h	1.20"

/*
 * ttold.h should always be included after termios.h. Otherwise, there
 * will be clashes in defines. We want termios.h codes to take precedence
 * over ttold.h codes.
 */

#ifndef _SYS_TYPES_H
#include "sys/types.h"
#endif

#define	CTRL(c)	((c)&037)
#define IBSHIFT 16

/* required by termio.h and VCEOF/VCEOL */
#define	NCC	8

/* some defines required by POSIX */
#define	NCCS	19

#ifndef _POSIX_VDISABLE
#define _POSIX_VDISABLE 0 /* Disable special character functions */
#endif

#ifndef MAX_INPUT
#define MAX_INPUT 512     /* Maximum bytes stored in the input queue */
#endif

#ifndef MAX_CANON
#define MAX_CANON 256     /* Maximum bytes in a line for canoical processing */
#endif
 
/*
 * types defined by POSIX. These are better off in types.h, but 
 * the standard says that they have to be in termios.h.
 */
typedef unsigned long tcflag_t;
typedef unsigned char cc_t;
typedef unsigned long speed_t;

/* 
 * POSIX termios functions
 * These functions get mapped into ioctls.
 */

extern speed_t cfgetospeed (/* termios *termios_p */);
extern int cfsetospeed (/* termios *termios_p, speed_t speed */);
extern speed_t cfgetispeed (/* termios *termios_p */);
extern int cfsetispeed (/* termios *termios_p, speed_t speed */);
extern int tcgetattr (/* int fildes, termios *termios_p */);
extern int tcsetattr (/* int fildes, int optional_actions, termios *termios_p */);
extern int tcsendbreak (/* int fildes, int duration */);
extern int tcdrain (/* int fildes */);
extern int tcflush (/* int fildes, int queue_selector */);
extern int tcflow (/* int fildes, int action */);
extern pid_t tcgetpgrp (/* int fildes */);
extern int tcsetpgrp (/* int fildes, pid_t pgrp_id */);



/* control characters */
#define	VINTR	0
#define	VQUIT	1
#define	VERASE	2
#define	VKILL	3
#define	VEOF	4
#define	VEOL	5
#define	VEOL2	6
#define	VMIN	4
#define	VTIME	5
#define	VSWTCH	7
#define	VSTART		8
#define	VSTOP		9
#define	VSUSP		10
#define	VDSUSP		11
#define	VREPRINT	12
#define	VDISCARD	13
#define	VWERASE		14
#define	VLNEXT		15
/* 16 thru 19 reserved for future use */

/*
 * control characters form Xenix termio.h
 */
#define	VCEOF	NCC		/* RESERVED true EOF char (V7 compatability) */
#define	VCEOL	(NCC + 1)	/* RESERVED true EOL char */

#define	CNUL	0
#define	CDEL	0377

/* S5 default control chars */
#define	CESC	'\\'
#define	CINTR	0177	/* DEL */
#define	CQUIT	034	/* FS, cntl | */
#define	CERASE	'#'
#define	CKILL	'@'
#define CEOT	04
#define CEOL	0
#define CEOL2	0
#define	CEOF	04	/* cntl d */
#define	CSTART	021	/* cntl q */
#define	CSTOP	023	/* cntl s */
#define	CSWTCH	032	/* cntl z */
#define	CNSWTCH	0
#define	CSUSP	CTRL('z')
#define	CDSUSP	CTRL('y')
#define	CRPRNT	CTRL('r')
#define	CFLUSH	CTRL('o')
#define	CWERASE	CTRL('w')
#define	CLNEXT	CTRL('v')


/* input modes */
#define	IGNBRK	0000001
#define	BRKINT	0000002
#define	IGNPAR	0000004
#define	PARMRK	0000010
#define	INPCK	0000020
#define	ISTRIP	0000040
#define	INLCR	0000100
#define	IGNCR	0000200
#define	ICRNL	0000400
#define	IUCLC	0001000
#define	IXON	0002000
#define	IXANY	0004000
#define	IXOFF	0010000
#define IMAXBEL 0020000
#define DOSMODE	0100000  /* for 386 compatibility */

/* output modes */
#define	OPOST	0000001
#define	OLCUC	0000002
#define	ONLCR	0000004
#define	OCRNL	0000010
#define	ONOCR	0000020
#define	ONLRET	0000040
#define	OFILL	0000100
#define	OFDEL	0000200
#define	NLDLY	0000400
#define	NL0	0
#define	NL1	0000400
#define	CRDLY	0003000
#define	CR0	0
#define	CR1	0001000
#define	CR2	0002000
#define	CR3	0003000
#define	TABDLY	0014000
#define	TAB0	0
#define	TAB1	0004000
#define	TAB2	0010000
#define	TAB3	0014000
#define XTABS	TAB3
#define	BSDLY	0020000
#define	BS0	0
#define	BS1	0020000
#define	VTDLY	0040000
#define	VT0	0
#define	VT1	0040000
#define	FFDLY	0100000
#define	FF0	0
#define	FF1	0100000
#define PAGEOUT 0200000
#define WRAP	0400000

/* control modes */
#define	CBAUD	0000017
#define	B0	0
#define	B50	0000001
#define	B75	0000002
#define	B110	0000003
#define	B134	0000004
#define	B150	0000005
#define	B200	0000006
#define	B300	0000007
#define	B600	0000010
#define	B1200	0000011
#define	B1800	0000012
#define	B2400	0000013
#define	B4800	0000014
#define	B9600	0000015
#define	B19200	0000016
#define EXTA	0000016
#define	B38400	0000017
#define EXTB	0000017
#define	CSIZE	0000060
#define	CS5	0
#define	CS6	0000020
#define	CS7	0000040
#define	CS8	0000060
#define	CSTOPB	0000100
#define	CREAD	0000200
#define	PARENB	0000400
#define	PARODD	0001000
#define	HUPCL	0002000
#define	CLOCAL	0004000
#define RCV1EN	0010000
#define	XMT1EN	0020000
#define	LOBLK	0040000
#define	XCLUDE	0100000		/* *V7* exclusive use coming fron XENIX */
#define CIBAUD	03600000
#define PAREXT	04000000

/* line discipline 0 modes */
#define	ISIG	0000001
#define	ICANON	0000002
#define	XCASE	0000004
#define	ECHO	0000010
#define	ECHOE	0000020
#define	ECHOK	0000040
#define	ECHONL	0000100
#define	NOFLSH	0000200
#define	TOSTOP	0000400
#define	ECHOCTL	0001000
#define	ECHOPRT	0002000
#define	ECHOKE	0004000
#define	DEFECHO	0010000
#define	FLUSHO	0020000
#define	PENDIN	0040000
#define	IEXTEN	0100000  /* POSIX flag - enable POSIX extensions */

#define	TIOC	('T'<<8)

#define	TCGETA	(TIOC|1)
#define	TCSETA	(TIOC|2)
#define	TCSETAW	(TIOC|3)
#define	TCSETAF	(TIOC|4)
#define	TCSBRK	(TIOC|5)
#define	TCXONC	(TIOC|6)
#define	TCFLSH	(TIOC|7)

/* Slots reserved for 386/XENIX compatibility - keyboard control */

#define TIOCKBON	(TIOC|8)
#define TIOCKBOF 	(TIOC|9)
#define KBENABLED 	(TIOC|10)

#ifndef IOCTYPE
#define	IOCTYPE	0xff00
#endif

#define	TCDSET	(TIOC|32)
#define	RTS_TOG	(TIOC|33)	/* 386 - "RTS" toggle define 8A1 protocol */

#define TIOCGWINSZ (TIOC|104)
#define TIOCSWINSZ (TIOC|103)


/* termios ioctls */

#define TCGETS		(TIOC|13)
#define TCSETS		(TIOC|14)
#define TCSANOW		(TIOC|14) /* same as TCSETS */
#define TCSETSW		(TIOC|15)
#define TCSADRAIN	(TIOC|15) /* same as TCSETSW */
#define	TCSETSF		(TIOC|16)
#define	TCSAFLUSH	(TIOC|16) /* same as TCSETSF */

/* termios option flags */

#define TCIFLUSH	0  /* flush data received but not read */
#define TCOFLUSH	1  /* flush data written but not transmitted */
#define TCIOFLUSH	2  /* flush both data both input and output queues */	

#define TCOOFF		0  /* suspend output */
#define TCOON		1  /* restart suspended output */
#define TCIOFF		2  /* suspend input */
#define TCION		3  /* restart suspended input */

/* TIOC ioctls for BSD, ptys, job control and modem control */

#define	tIOC	('t'<<8)


/* Slots for 386/XENIX compatibility */
/* BSD includes these ioctls in ttold.h */

#ifndef _SYS_TTOLD_H

#define TIOCGETD	(tIOC|0)
#define TIOCSETD	(tIOC|1)
#define TIOCHPCL	(tIOC|2)
#define TIOCGETP	(tIOC|8)
#define TIOCSETP  	(tIOC|9)
#define TIOCSETN	(tIOC|10)
#define TIOCEXCL	(tIOC|13)
#define TIOCNXCL	(tIOC|14)
#define TIOCFLUSH	(tIOC|16)
#define TIOCSETC	(tIOC|17)
#define TIOCGETC	(tIOC|18)
/* BSD ioctls that are not the same as XENIX */

#define	TIOCLBIS	(tIOC|127)	/* bis local mode bits */
#define	TIOCLBIC	(tIOC|126)	/* bic local mode bits */
#define	TIOCLSET	(tIOC|125)	/* set entire local mode word */
#define	TIOCLGET	(tIOC|124)	/* get local modes */
#define	TIOCSBRK	(tIOC|123)	/* set break bit */
#define	TIOCCBRK	(tIOC|122)	/* clear break bit */
#define	TIOCSDTR	(tIOC|121)	/* set data terminal ready */
#define	TIOCCDTR	(tIOC|120)	/* clear data terminal ready */
#define	TIOCSLTC	(tIOC|117)	/* set local special chars */
#define	TIOCGLTC	(tIOC|116)	/* get local special chars */
#define	TIOCNOTTY	(tIOC|113)	/* void tty association */
#define	TIOCSTOP	(tIOC|111)	/* stop output, like ^S */
#define	TIOCSTART	(tIOC|110)	/* start output, like ^Q */

#endif /* end _SYS_TTOLD_H */

/* POSIX job control ioctls */

#define	TIOCGPGRP	(tIOC|20)	/* get pgrp of tty */
#define	TIOCSPGRP	(tIOC|21)	/* set pgrp of tty */
#define	TIOCGSID	(tIOC|22)	/* get session id on ctty*/
#define	TIOCSSID	(tIOC|24)	/* set session id on ctty*/

/* Miscellanous */
#define	TIOCSTI		(tIOC|23)	/* simulate terminal input */

/* Modem control */
#define	TIOCMSET	(tIOC|26)	/* set all modem bits */
#define	TIOCMBIS	(tIOC|27)	/* bis modem bits */
#define	TIOCMBIC	(tIOC|28)	/* bic modem bits */
#define	TIOCMGET	(tIOC|29)	/* get all modem bits */
#define		TIOCM_LE	0001		/* line enable */
#define		TIOCM_DTR	0002		/* data terminal ready */
#define		TIOCM_RTS	0004		/* request to send */
#define		TIOCM_ST	0010		/* secondary transmit */
#define		TIOCM_SR	0020		/* secondary receive */
#define		TIOCM_CTS	0040		/* clear to send */
#define		TIOCM_CAR	0100		/* carrier detect */
#define		TIOCM_CD	TIOCM_CAR
#define		TIOCM_RNG	0200		/* ring */
#define		TIOCM_RI	TIOCM_RNG
#define		TIOCM_DSR	0400		/* data set ready */

/* pseudo-tty */

#define	TIOCREMOTE	(tIOC|30)	/* remote input editing */
#define TIOCSIGNAL	(tIOC|31)	/* pty: send signal to slave */


/* Some more 386 xenix stuff */

#define	LDIOC	('D'<<8)

#define	LDOPEN	(LDIOC|0)
#define	LDCLOSE	(LDIOC|1)
#define	LDCHG	(LDIOC|2)
#define	LDGETT	(LDIOC|8)
#define	LDSETT	(LDIOC|9)

/* Slots for 386 compatibility */

#define	LDSMAP		(LDIOC|10)
#define	LDGMAP		(LDIOC|11)
#define	LDNMAP		(LDIOC|12)

/*
 * These are retained for 386/XENIX compatibility.
 */

#define	DIOC	('d'<<8)
#define DIOCGETP        (DIOC|8)                /* V7 */
#define DIOCSETP        (DIOC|9)                /* V7 */

/*
 * Returns a non-zero value if there
 * are characters in the input queue.
 */
#define FIORDCHK        (('f'<<8)|3)            /* V7 */


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


/* Windowing structure to support JWINSIZE/TIOCSWINSZ/TIOCGWINSZ */
struct winsize {
	unsigned short ws_row;       /* rows, in characters*/
	unsigned short ws_col;       /* columns, in character */
	unsigned short ws_xpixel;    /* horizontal size, pixels */
	unsigned short ws_ypixel;    /* vertical size, pixels */
};

#endif	/* _SYS_TERMIOS_H */
