/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_TTOLD_H
#define _SYS_TTOLD_H

#ident	"@(#)head.sys:sys/ttold.h	11.10"

/* All the ioctls (BSD, V7, XENIX, S5) defines are in termios.h */
/* this file has mostly BSD structures and flags */
/* this also has the contents of SUN/BSD ttychars.h and ttydev.h  */
/* there are some defines that are common between ttold.h and termios.h. */
/* ttold.h should always follow termios.h so the defines in termios.h have */
/* precedence over ttold.h */

/* BSD special characters. Structure for TIOCSTC/TIOCGTC */
struct tchars {
	char	t_intrc;	/* interrupt */
	char	t_quitc;	/* quit */
	char	t_startc;	/* start output */
	char	t_stopc;	/* stop output */
	char	t_eofc;		/* end-of-file */
	char	t_brkc;		/* input delimiter (like nl) */
};

/* note xenix defines tchars as tc */
struct tc {
	char	t_intrc;	/* interrupt */
	char	t_quitc;	/* quit */
	char	t_startc;	/* start output */
	char	t_stopc;	/* stop output */
	char	t_eofc;		/* end-of-file */
	char	t_brkc;		/* input delimiter (like nl) */
};
/* BSD local special chars. Structure for TIOCSLTC/TIOCGLTC */
struct ltchars {
	char	t_suspc;	/* stop process signal */
	char	t_dsuspc;	/* delayed stop process signal */
	char	t_rprntc;	/* reprint line */
	char	t_flushc;	/* flush output (toggles) */
	char	t_werasc;	/* word erase */
	char	t_lnextc;	/* literal next character */
};

/*
 * Structure for TIOCGETP and TIOCSETP ioctls.
 */

#ifndef _SGTTY_H
/*  sg_flags value changed from short (in SUN/BSD) to int in System V to */
/*  match sgtty.h definition */
struct	sgttyb {
	char	sg_ispeed;		/* input speed */
	char	sg_ospeed;		/* output speed */
	char	sg_erase;		/* erase character */
	char	sg_kill;		/* kill character */
	int	sg_flags;		/* mode flags */
};
#endif /* _SGTTY_H */

/*
 * 4.3BSD/XENIX tty ioctl commands that are either:
 *  1) deprecated
 *  2) not implemented (and never were implemented)
 *  3) implemented on top of new-style "ioctl"s.
 */

/*
 * These ioctls are also defined in termios.h.
 * because XENIX expects to see them in termio.h
 */

#ifndef tIOC
#define	tIOC	('t'<<8)
#endif

#ifndef _SYS_TERMIOS_H
/* defined in termios.h also so that all the ioctl codes are visible*/

#define TIOCGETD	(tIOC|0)
#define TIOCSETD	(tIOC|1)
#ifndef _SGTTY_H
#define TIOCHPCL	(tIOC|2)
#define TIOCGETP	(tIOC|8)
#define TIOCSETP  	(tIOC|9)
#endif /* _SGTTY_H */
#define TIOCSETN	(tIOC|10)
#ifndef _SGTTY_H
#define TIOCEXCL	(tIOC|13)
#define TIOCNXCL	(tIOC|14)
#endif /* _SGTTY_H */
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

#define	TIOCREMOTE	(tIOC|30)	/* remote input editing */

/* windowing ioctls and structure also defined in termios.h */

#define	TIOC	('T'<<8)

#define TIOCGWINSZ (TIOC|104)
#define TIOCSWINSZ (TIOC|103)

/* Windowing structure to support TIOCSWINSZ/TIOCGWINSZ */
struct winsize {
	unsigned short ws_row;       /* rows, in characters*/
	unsigned short ws_col;       /* columns, in character */
	unsigned short ws_xpixel;    /* horizontal size, pixels */
	unsigned short ws_ypixel;    /* vertical size, pixels */
};


#endif /* end _SYS_TERMIOS_H */

/* Old SVR3.0 flags - should be removed if there is no problem */
/* note they are commented out */

#if 0

#define	O_HUPCL	01
#define	O_XTABS	02
#define	O_LCASE	04   /* simulate lower case */
#define	O_ECHO	010  /* echo input */
#define	O_CRMOD	020  /* map \r to \r\n on output */
#define	O_RAW	040  /* no i/o processing */
#define	O_ODDP	0100 /* get/send odd parity */
#define	O_EVENP	0200 /* get/send even parity */
#define	O_NLDELAY	001400	/* \n delay */
#define	O_NL1	000400
#define	O_NL2	001000
#define	O_TBDELAY	002000	/* horizontal tab delay */
#define	O_NOAL	004000
#define	O_CRDELAY	030000	/* \r delay */
#define	O_CR1	010000
#define	O_CR2	020000
#define	O_VTDELAY	040000	/* vertical tab delay */
#define	O_BSDELAY	0100000 /* \b delay */

#endif

/* 4.3 BSD additions. These are new codes  and some of the
 * flags that were there in SVR3.2 ttold.h have been given
 * new codes. Otherwise they will not fit in a word.
 */ 

#define		O_TANDEM	0x00000001	/* send stopc on out q full */
#define		O_CBREAK	0x00000002	/* half-cooked mode */
#define		O_LCASE		0x00000004	/* simulate lower case */
#define		O_ECHO		0x00000008	/* echo input */
#define		O_CRMOD		0x00000010	/* map \r to \r\n on output */
#define		O_RAW		0x00000020	/* no i/o processing */
#define		O_ODDP		0x00000040	/* get/send odd parity */
#define		O_EVENP		0x00000080	/* get/send even parity */
#define		O_ANYP		0x000000c0	/* get any parity/send none */
#define		O_NLDELAY	0x00000300	/* \n delay */
#define			O_NL0	0x00000000
#define			O_NL1	0x00000100	/* tty 37 */
#define			O_NL2	0x00000200	/* vt05 */
#define			O_NL3	0x00000300
#define		O_TBDELAY	0x00000c00	/* horizontal tab delay */
#define			O_TAB0	0x00000000
#define			O_TAB1	0x00000400	/* tty 37 */
#define			O_TAB2	0x00000800
#define		O_XTABS		0x00000c00	/* expand tabs on output */
#define		O_CRDELAY	0x00003000	/* \r delay */
#define			O_CR0	0x00000000
#define			O_CR1	0x00001000	/* tn 300 */
#define			O_CR2	0x00002000	/* tty 37 */
#define			O_CR3	0x00003000	/* concept 100 */
#define		O_VTDELAY	0x00004000	/* vertical tab delay */
#define			O_FF0	0x00000000
#define			O_FF1	0x00004000	/* tty 37 */
#define		O_BSDELAY	0x00008000	/* \b delay */
#define			O_BS0	0x00000000
#define			O_BS1	0x00008000
#define 	O_ALLDELAY	(O_NLDELAY|O_TBDELAY|O_CRDELAY|O_VTDELAY|O_BSDELAY)
#define		O_CRTBS		0x00010000	/* do backspacing for crt */
#define		O_PRTERA	0x00020000	/* \ ... / erase */
#define		O_CRTERA	0x00040000	/* " \b " to wipe out char */
#define		O_TILDE		0x00080000	/* hazeltine tilde kludge */
#define		O_MDMBUF	0x00100000	/* start/stop output on carrier intr */
#define		O_LITOUT	0x00200000	/* literal output */
#define		O_TOSTOP	0x00400000	/* SIGSTOP on background output */
#define		O_FLUSHO	0x00800000	/* flush output to terminal */
#define		O_NOHANG	0x01000000	/* no SIGHUP on carrier drop */
#define		O_L001000	0x02000000
#define		O_CRTKIL	0x04000000	/* kill line with " \b " */
#define		O_PASS8		0x08000000
#define		O_CTLECH	0x10000000	/* echo control chars as ^X */
#define		O_PENDIN	0x20000000	/* tp->t_rawq needs reread */
#define		O_DECCTQ	0x40000000	/* only ^Q starts after ^S */
#define		O_NOFLSH	0x80000000	/* no output flush on signal */

/* more BSD flags */
#define		LCRTBS		(O_CRTBS>>16)
#define		LPRTERA		(O_PRTERA>>16)
#define		LCRTERA		(O_CRTERA>>16)
#define		LTILDE		(O_TILDE>>16)
#define		LMDMBUF		(O_MDMBUF>>16)
#define		LLITOUT		(O_LITOUT>>16)
#define		LTOSTOP		(O_TOSTOP>>16)
#define		LFLUSHO		(O_FLUSHO>>16)
#define		LNOHANG		(O_NOHANG>>16)
#define		LCRTKIL		(O_CRTKIL>>16)
#define		LPASS8		(O_PASS8>>16)
#define		LCTLECH		(O_CTLECH>>16)
#define		LPENDIN		(O_PENDIN>>16)
#define		LDECCTQ		(O_DECCTQ>>16)
#define		LNOFLSH		(O_NOFLSH>>16)


#define		NOPOST		0x00000001	/* no processing on output (LITOUT with 7 bits + parity) */
#define		NOISIG		0x00000002	/* disable all signal-generating characters */
#define		STOPB		0x00000004	/* two stop bits */

#define	OTTYDISC	0		/* old, v7 std tty driver */
#define	NETLDISC	1		/* line discip for berk net */
#define	NTTYDISC	2		/* new tty discipline */
#define	TABLDISC	3		/* hitachi tablet discipline */
#define	NTABLDISC	4		/* gtco tablet discipline */
#define	MOUSELDISC	5		/* mouse discipline */
#define	KBDLDISC	6		/* up/down keyboard trans (console) */

/*
 * contents from SUN/BSD ttychars.h
 * User visible structures and constants
 * related to terminal handling.
 */
struct ttychars {
	char	tc_erase;	/* erase last character */
	char	tc_kill;	/* erase entire line */
	char	tc_intrc;	/* interrupt */
	char	tc_quitc;	/* quit */
	char	tc_startc;	/* start output */
	char	tc_stopc;	/* stop output */
	char	tc_eofc;	/* end-of-file */
	char	tc_brkc;	/* input delimiter (like nl) */
	char	tc_suspc;	/* stop process signal */
	char	tc_dsuspc;	/* delayed stop process signal */
	char	tc_rprntc;	/* reprint line */
	char	tc_flushc;	/* flush output (toggles) */
	char	tc_werasc;	/* word erase */
	char	tc_lnextc;	/* literal next character */
};

#ifndef _SYS_TERMIOS_H

#define	CTRL(c)	((c)&037)

/* default BSD special characters */
#define	CERASE	0177
#define	CKILL	CTRL('u')
#define	CINTR	CTRL('c')
#define	CQUIT	034		/* FS, ^\ */
#define	CSTART	CTRL('q')
#define	CSTOP	CTRL('s')
#define	CEOF	CTRL('d')
#define	CEOT	CEOF
#define	CBRK	0377
#define	CSUSP	CTRL('z')
#define	CDSUSP	CTRL('y')
#define	CRPRNT	CTRL('r')
#define	CFLUSH	CTRL('o')
#define	CWERASE	CTRL('w')
#define	CLNEXT	CTRL('v')


/*
 * Contents from SUN/BSD ttydev.h.
 * Terminal definitions related to underlying hardware.
 */

/*
 * Speeds
 */
#define B0	0
#define B50	1
#define B75	2
#define B110	3
#define B134	4
#define B150	5
#define B200	6
#define B300	7
#define B600	8
#define B1200	9
#define	B1800	10
#define B2400	11
#define B4800	12
#define B9600	13
#define B19200	14
#define B38400	15
#define EXTA	14
#define EXTB	15

#endif /* end _SYS_TERMIOS_H */

#if 0
/*
 * Hardware bits.
 * SHOULD NOT BE HERE.
 */
#define	DONE	0200
#define	IENABLE	0100

/*
 * Modem control commands.
 */
#define	DMSET		0
#define	DMBIS		1
#define	DMBIC		2
#define	DMGET		3

#endif /* end if 0 comment */

#endif	/* _SYS_TTOLD_H */
