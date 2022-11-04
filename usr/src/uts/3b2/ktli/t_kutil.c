/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktli:ktli/t_kutil.c	1.4"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)t_kutil.c 1.2 89/01/11 SMI"
#endif

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
 *	Contains the following utility functions:
 *		t_stropen:
 *		tli_send:
 *		tli_recv:
 *		get_ok_ack:
 *
 *	Returns:
 *		See individual functions.
 *
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/stream.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/strsubr.h>
#include <sys/tihdr.h>
#include <sys/timod.h>
#include <sys/tiuser.h>
#include <sys/t_kuser.h>

extern char qrunflag;	/* should be in stream.h */

int
tli_send(tiptr, bp, fmode)
register TIUSER	*tiptr;
register mblk_t	 *bp;
register int	fmode;

{
	int	 retval;
	register int s;
	register struct file *fp;
	register struct vnode *vp;
	register struct stdata *stp;

	retval = 0;
	fp = tiptr->fp;
	vp = fp->f_vnode;
	stp = vp->v_stream;

	s = splstr();
	while (!canput(stp->sd_wrq->q_next)) {
		if ((u.u_error = strwaitq(stp, WRITEWAIT, (off_t)0, fmode, 
						&retval)) || retval) {
			return -1;
		}
	}
	(void)splx(s);

	putnext(stp->sd_wrq, bp);
	if (qready())
		runqueues();

	return 0;
}

int 
tli_recv(tiptr, bp, fmode)
register TIUSER	 *tiptr;
register mblk_t	 **bp;
register int	fmode;

{
	int	 retval;
	register int s;
	register struct file *fp;
	register struct vnode *vp;
	register struct stdata *stp;

	retval = 0;
	fp = tiptr->fp;
	vp = fp->f_vnode;
	stp = vp->v_stream;

	s = splstr();
	if (stp->sd_flag & (STRDERR|STPLEX)) {
		u.u_error = ((stp->sd_flag&STPLEX) ? EINVAL : stp->sd_rerror);
		(void)splx(s);
		return -1;
	}

	while ( !(*bp = getq(RD(stp->sd_wrq)))) {
		if ((u.u_error = strwaitq(stp, READWAIT, (off_t)0, fmode,
						 &retval)) || retval) {
			(void)splx(s);
			return -1;
		}
	}
	if (stp->sd_flag) {
		stp->sd_flag &= ~STRPRI;
	}
	(void)splx(s);

	if (qready())
		runqueues();

#ifdef KTLIDEBUG
{
mblk_t *tp;
tp = *bp;
while (tp) {
	if (tp->b_datap->db_size < 0)
		printf("tli_recv: tp %x, func %x\n", tp, tp->b_datap->db_frtnp->free_func);
	tp = tp->b_cont;
}
}
#endif
	return 0;
}

int
get_ok_ack(tiptr, type, fmode)
register TIUSER	*tiptr;
register int type, fmode;

{
	register int msgsz, retval;
	register union T_primitives *pptr;
	mblk_t *bp;

	retval = 0;

	/* wait for ack
	 */
	if (tli_recv(tiptr, &bp, fmode) < 0)
		return 1;

	if ((msgsz = (bp->b_wptr - bp->b_rptr)) < sizeof(long)) {
		u.u_error = EPROTO;
		return -1;
	}

	/* LINTED pointer alignment */
	pptr = (union T_primitives *)bp->b_rptr;
	switch(pptr->type) {
		case T_OK_ACK:
			if (msgsz < TOKACKSZ ||
				pptr->ok_ack.CORRECT_prim != type) {
				u.u_error = EPROTO;
				retval = -1;
			}
			break;

		case T_ERROR_ACK:
			if (msgsz < TERRORACKSZ) {
				u.u_error = EPROTO;
				retval = -1;
				break;
			}

                        if (pptr->error_ack.TLI_error == TSYSERR)
                                u.u_error = pptr->error_ack.UNIX_error;
                        else    u.u_error = tlitosyserr(pptr->error_ack.TLI_error);

			retval = -1;
			break;

		default:
			u.u_error = EPROTO;
			retval = -1;
			break;
	}
	return retval;
}

