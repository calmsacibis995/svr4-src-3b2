/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_CONF_H
#define _SYS_CONF_H

#ident	"@(#)head.sys:sys/conf.h	11.16"

/*
 * Declaration of block device switch. Each entry (row) is
 * the only link between the main unix code and the driver.
 * The initialization of the device switches is in the file conf.c.
 */
struct bdevsw {
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_strategy)();
	int	(*d_print)();
	int	(*d_size)();
	int	(*d_xpoll)();
	int	(*d_xhalt)();
	int	*d_flag;
};
extern struct bdevsw bdevsw[];

/*
 * Character device switch.
 */
struct cdevsw {
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_read)();
	int	(*d_write)();
	int	(*d_ioctl)();
	int	(*d_mmap)();
	int	(*d_segmap)();
	int	(*d_poll)();
	int	(*d_xpoll)();
	int	(*d_xhalt)();
	struct tty *d_ttys;
	struct streamtab *d_str;
	int	*d_flag;
};
extern struct cdevsw cdevsw[];

/*
 * Device flags.
 */
#define D_NEW		0x00	/* new-style driver */
#define	D_OLD		0x01	/* Old-style driver */
/*
 * Added for UFS.
 */
#define D_SEEKNEG       0x04    /* Negative seek offsets are OK */
#define D_TAPE          0x08    /* Magtape device (no bdwrite when cooked) */

#define	FMNAMESZ	8

struct fmodsw {
	char	f_name[FMNAMESZ+1];
	struct  streamtab *f_str;
	int	*f_flag;		/* same as device flag */
};
extern struct fmodsw fmodsw[];

extern int	bdevcnt;
extern int	cdevcnt;
extern int	fmodcnt;

/*
 * Line discipline switch.
 */
struct linesw {
	int	(*l_open)();
	int	(*l_close)();
	int	(*l_read)();
	int	(*l_write)();
	int	(*l_ioctl)();
	int	(*l_input)();
	int	(*l_output)();
	int	(*l_mdmint)();
};
extern struct linesw linesw[];

extern int	linecnt;
/*
 * Terminal switch
 */
struct termsw {
	int	(*t_input)();
	int	(*t_output)();
	int	(*t_ioctl)();
};
extern struct termsw termsw[];

extern int	termcnt;

#endif	/* _SYS_CONF_H */
