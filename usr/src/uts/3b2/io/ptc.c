/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/ptc.c	1.4"
/**********************************************************************
 * 	PSEUDO TELETYPE DRIVER
 * 	(Actually two drivers, requiring two entries in 'cdevsw')
 *
 * 	A pseudo-teletype is a special device which is similar to a pipe.
 *	It is used to communicate between two processes.  However, it allows
 *	one to simulate a teletype, including mode setting, interrupt, and
 *	multiple end of files (all not possible on a pipe).
 *
 *	There are really two drivers here.  One is the device which looks like
 *	a terminal and can be thought of as the slave device, and hence
 *	its routines are prefixed with 'pts' (PTY Slave).
 *	The other driver can be thought of as the controlling device,
 *	and its routines are prefixed
 *	by 'ptc' (PTY Controller).  To type on the simulated keyboard of the
 *	PTY, one does a 'write' to the controlling device.  To get the
 *	simulated printout from the PTY, one does a 'read' on the controlling
 *	device.  Normally, the controlling device is called 'ptyx' and the
 *	slave device is called 'ttyx' (where x is a number)
 *	(to make programs like 'who' happy).
 *********************************************************************/

/* TTY HEADERS FOR PTY'S */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sbd.h"
#include "sys/signal.h"
#include "sys/immu.h"
#include "sys/errno.h"
#include "sys/sysmacros.h"
#include "macros.h"
#include "sys/systm.h"
#include "sys/termio.h"
#include "sys/tty.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/conf.h"
#include "sys/buf.h"
#include "sys/file.h"
#include "sys/proc.h"

extern int	pts_cnt;
extern struct tty pts_tty[];

int	ptsproc(), nulldev();


/*******************************************************************
 *				CONTROLLING HALF
 *
 *	The half that simulates hardware.
 ******************************************************************/


/*******************************************************************
 *				PTCOPEN
 *
 *	Open controlling half (simulated "hardware" side)
 ******************************************************************/

ptcopen(dev, flag)
dev_t dev;
{
	register struct tty *tp;

	if (minor(dev) >= pts_cnt) {
		u.u_error = ENXIO;
		return;
	}
	tp = &pts_tty[minor(dev)];

	/* already open is an error */
	if (tp->t_proc == ptsproc) {
		u.u_error = EIO;
		return;
	}

	tp->t_proc = ptsproc;		/* Set address of proc routine */
	if (tp->t_state & WOPEN)
		wakeup((caddr_t) & tp->t_rawq);
	tp->t_state |= CARR_ON;
}


/*********************************************************************
 *				PTCCLOSE
 *
 *	Close controlling half.
 ********************************************************************/

ptcclose(dev)
dev_t dev;
{
	register struct tty *tp;

	tp = &pts_tty[minor(dev)];
	if (tp->t_state & ISOPEN)
		signal(tp->t_pgrp, SIGHUP);
	tp->t_state &= ~CARR_ON;	/* Virtual carrier is gone */
	ttyflush(tp, FREAD | FWRITE);	/* Clean things up */
	tp->t_proc = nulldev;		/* Mark as closed */
}


/***********************************************************************
 *				PTCREAD
 *
 *	Pretend you are a device interupt routine and are
 *	putting data out to the hardware by taking data from
 *	the slave side buffers and passing it to the controlling process.
 **********************************************************************/

ptcread(dev)
dev_t dev;
{
	register struct tty *tp;
	register done;

	tp = &pts_tty[minor(dev)];

	/* BUSY means there are chars ready for us */
	/* passc returns negative when read call is done */

	done = 0;

loop:
	if (tp->t_state & (CARR_ON | ISOPEN) == 0) 
		return;
	while ((tp->t_state & BUSY) == 0)
		sleep((caddr_t) & tp->t_outq.c_cf, TTIPRI);
	if (tp->t_tbuf.c_count) {
		done = passc(*tp->t_tbuf.c_ptr++);
		tp->t_tbuf.c_count--;
	}
	tp->t_state &= ~BUSY;
	
	ptsproc(tp, T_OUTPUT);
	if (tp->t_outq.c_cc == 0)
		return;	
	if (done >= 0)
		goto loop;
	return;
}


/*********************************************************************
 *				PTCWRITE
 *
 *	Pretend you are a device interrupt routine and put
 *	characters into the slave input buffer.
 *	Depending on compilation flag PTYFLOW, either
 *	perform flow control, or don't.  However
 *	However, must scan all that is sent to us for
 *	STOP/START/INTERRUPT characters anyway.
 ********************************************************************/

ptcwrite(dev)
dev_t dev;
{
	register struct tty *tp;
	register int	cc;
	register int	s;
	register int	c;

	tp = &pts_tty[minor(dev)];
	if ((tp->t_state & (CARR_ON | ISOPEN)) == 0)
		return;

	s = spl6();

	/* allocate an input buffer if necessary */
	if (tp->t_rbuf.c_ptr == NULL) {
		while ((tp->t_rbuf.c_ptr = getcf()->c_data)
		     == ((struct cblock *)NULL)->c_data) {
			tp->t_rbuf.c_ptr = NULL;
			cfreelist.c_flag = 1;
			sleep(&cfreelist, TTOPRI);
		}
		tp->t_rbuf.c_count = cfreelist.c_size;
		tp->t_rbuf.c_size  = cfreelist.c_size;
	}

	/* get a character and perform optional IXON processing */
	while ((c = cpass()) >= 0) {
		if (tp->t_iflag & IXON) {
			register char	ctmp;
			ctmp = c & 0177;
			if (tp->t_state & TTSTOP) {
				if (ctmp == CSTART || tp->t_iflag & IXANY)
					(*tp->t_proc)(tp, T_RESUME);
			} else {
				if (ctmp == CSTOP)
					(*tp->t_proc)(tp, T_SUSPEND);
			}
			if (ctmp == CSTART || ctmp == CSTOP)
				continue;
		}

		/* perform optional ISTRIP processing */
		if (tp->t_iflag & ISTRIP)
			c &= 0177;
		else
			c &= 0377;

		/* put character into rbuf */
		*tp->t_rbuf.c_ptr++ = c;
		tp->t_rbuf.c_count--;
		tp->t_rbuf.c_ptr -= 
		    tp->t_rbuf.c_size - tp->t_rbuf.c_count;
		(*linesw[tp->t_line].l_input)(tp);
	}
	splx(s);
}


/**********************************************************************
 *				IOCTL
 *
 *	Slave and Controlling half IOCTL calls.
 *	Only the Slave half has terminal modes.  However, we let
 *	Controlling half change the modes of the slave half.
 *********************************************************************/

ptsioctl(dev, cmd, arg, mode)
dev_t dev;
{
	ttiocom(&pts_tty[minor(dev)], cmd, arg, mode);
}


ptcioctl(dev, cmd, arg, mode)
dev_t dev;
{
	register struct tty *tp;

	tp = &pts_tty[minor(dev)];
	/* Flush to prevent a hang */
	while (getc(&tp->t_outq) >= 0)
		;
	ttiocom(tp, cmd, arg, mode);
}





