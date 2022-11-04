/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktli:ktli/t_kgtstate.c	1.2"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)t_kgetstate.c 1.2 89/01/11 SMI"
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
 *
 *	Kernel TLI-like function to get the state of an
 *	endpoint. 
 *
 *	Returns:
 *		The state or -1 on failure.		
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


int
t_kgetstate(tiptr)
register TIUSER *tiptr;
{
	struct   T_info_ack inforeq;
	struct	 strioctl strioc;
	int	 retval;
	register struct vnode *vp;
	register struct file *fp;

	retval = 0;
	fp = tiptr->fp;
	vp = fp->f_vnode;

	inforeq.PRIM_type = T_INFO_REQ;
	strioc.ic_cmd = TI_GETINFO;
	strioc.ic_timout = 0;
	strioc.ic_dp = (char *)&inforeq;
	strioc.ic_len = sizeof(struct T_info_req);

	u.u_error = strdoioctl(vp->v_stream, &strioc, NULL, K_TO_K,
					 (char *)NULL, u.u_cred, &retval);
	if (u.u_error) 
		return -1;

	if (retval) {
		if ((retval & 0xff) == TSYSERR)
			u.u_error = (retval >> 8) & 0xff;
		else    u.u_error = tlitosyserr(retval & 0xff);
		return -1;
	}

	if (strioc.ic_len != sizeof(struct T_info_ack)) {
		u.u_error = EPROTO;
		return -1;
	}

	switch (inforeq.CURRENT_state) {
		case TS_UNBND:
			return(T_UNBND);
		case TS_IDLE:
			return(T_IDLE);
		case TS_WRES_CIND:
			return(T_INCON);
		case TS_WCON_CREQ:
			return(T_OUTCON);
		case TS_DATA_XFER:
			return(T_DATAXFER);
		case TS_WIND_ORDREL:
			return(T_OUTREL);
		case TS_WREQ_ORDREL:
			return(T_INREL);
		default:
			u.u_error = EPROTO;
			return(-1);
	}
}

/******************************************************************************/

