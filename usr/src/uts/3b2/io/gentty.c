/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/gentty.c	1.20"
/*
 * Indirect driver for controlling tty.
 */
#include "sys/types.h"
#include "sys/sbd.h"
#include "sys/param.h"
#include "sys/immu.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/proc.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/conf.h"
#include "sys/tty.h"
#include "sys/stream.h"
#include "sys/strsubr.h"
#include "sys/cred.h"
#include "sys/uio.h"
#include "sys/session.h"
#include "sys/ddi.h"
#include "sys/debug.h"

int sydevflag = 2;

STATIC int
sycheck(sp)
register sess_t *sp;
{
	if (sp->s_dev == NODEV)
		return ENXIO;
	if (sp->s_vp == NULL)
		return EIO;
	return 0;
}

/* ARGSUSED */
int
syopen(devp, flag, otyp, cr)
	dev_t *devp;
	int flag;
	int otyp;
	struct cred *cr;
{
	dev_t sydev;
	int error;
	sess_t *sp = u.u_procp->p_sessp;

	if (error = sycheck(sp))
		return error;
	sydev = sp->s_vp->v_rdev;
	if (cdevsw[getmajor(sydev)].d_str)
		error = stropen(sp->s_vp, &sydev, flag, cr);
	else if (*cdevsw[getmajor(sydev)].d_flag & D_OLD) {
		(void)(*cdevsw[getmajor(sydev)].d_open)(cmpdev(sydev), flag);
		error = u.u_error;		/* XXX */
	} else
		error = (*cdevsw[getmajor(sydev)].d_open)(&sydev, flag, otyp, cr);
	return error;
}

/* ARGSUSED */
int
syclose(dev, flag, otyp, cr)
	dev_t dev;
	int flag;
	int otyp;
	struct cred *cr;
{
	return 0;
}

/* ARGSUSED */
int
syread(dev, uiop, cr)
	dev_t dev;
	struct uio *uiop;
	struct cred *cr;
{
	int error;
	sess_t *sp = u.u_procp->p_sessp;
	dev_t ttyd = sp->s_vp->v_rdev;

	if (error = sycheck(sp))
		return error;
	if (cdevsw[getmajor(ttyd)].d_str)
		error = strread(sp->s_vp, uiop, cr);
	else if (*cdevsw[getmajor(ttyd)].d_flag & D_OLD) {
		u.u_offset = uiop->uio_offset;
		u.u_base = uiop->uio_iov->iov_base;
		u.u_count = uiop->uio_resid;
		u.u_segflg = uiop->uio_segflg;
		u.u_fmode = uiop->uio_fmode;
		(void)(*cdevsw[getmajor(ttyd)].d_read)(cmpdev(ttyd));
		uiop->uio_resid = u.u_count;
		uiop->uio_offset = u.u_offset;
		error = u.u_error;		/* XXX */
	} else
		error = (*cdevsw[getmajor(ttyd)].d_read) (ttyd, uiop, cr);
	return error;
}

/* ARGSUSED */
int
sywrite(dev, uiop, cr)
	dev_t dev;
	struct uio *uiop;
	struct cred *cr;
{
	int error;
	sess_t *sp = u.u_procp->p_sessp;
	dev_t ttyd = sp->s_vp->v_rdev;

	if (error = sycheck(sp))
		return error;
	if (cdevsw[getmajor(ttyd)].d_str)
		error = strwrite(sp->s_vp, uiop, cr);
	else if (*cdevsw[getmajor(ttyd)].d_flag & D_OLD) {
		u.u_offset = uiop->uio_offset;
		u.u_base = uiop->uio_iov->iov_base;
		u.u_count = uiop->uio_resid;
		u.u_segflg = uiop->uio_segflg;
		u.u_fmode = uiop->uio_fmode;
		(void)(*cdevsw[getmajor(ttyd)].d_write)(cmpdev(ttyd));
		uiop->uio_resid = u.u_count;
		uiop->uio_offset = u.u_offset;
		error = u.u_error;		/* XXX */
	} else
		error = (*cdevsw[getmajor(ttyd)].d_write) (ttyd, uiop, cr);
	return error;
}


/* ARGSUSED */
int
syioctl(dev, cmd, arg, mode, cr, rvalp)
	dev_t dev;
	int cmd;
	int arg;
	int mode;
	struct cred *cr;
	int *rvalp;
{
	int error;
	sess_t *sp = u.u_procp->p_sessp;
	dev_t ttyd = sp->s_vp->v_rdev;

	if (error = sycheck(sp))
		return error;
	if (cdevsw[getmajor(ttyd)].d_str)
		error = strioctl(sp->s_vp, cmd, arg, mode, U_TO_K, cr, rvalp);
	else if (*cdevsw[getmajor(ttyd)].d_flag & D_OLD) {
		(void)(*cdevsw[getmajor(ttyd)].d_ioctl) (cmpdev(ttyd), cmd, arg, mode);
		error = u.u_error;		/* XXX */
		*rvalp = u.u_rval1;	/* XXX */
	} else
		error = (*cdevsw[getmajor(ttyd)].d_ioctl)
		  (ttyd, cmd, arg, mode, cr, rvalp);
	return error;
}
