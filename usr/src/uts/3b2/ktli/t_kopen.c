/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktli:ktli/t_kopen.c	1.5"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)t_kopen.c 1.2 89/03/19 SMI"
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
 *	Kernel TLI-like function to initialize a transport
 *	endpoint using the protocol specified.
 *
 *	Returns:
 *		A valid file pointer on success or -1 on failure.
 *		On success, if info is non-NULL it will be filled with
 *		transport provider values.
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
#include <sys/kmem.h>

static _t_setsize();

TIUSER *
t_kopen(fp, rdev, flags, info)
struct file *fp;
register int flags;
register dev_t rdev;
register struct	t_info *info;

{
	extern	 struct vnode *makespecvp();

	register TIUSER *tiptr;
	register int madefp = 0;
	struct   T_info_ack inforeq;
	int	 retval;
	struct   vnode *vp;
	struct	 strioctl strioc;

#ifdef KTLIDEBUG
printf("t_kopen: fp %x, rdev %x, flags %x, info %x\n", fp, rdev, flags, info);
#endif

	u.u_error = 0;
	retval = 0;
	if (fp == NULL) {
		int fd;

		if (rdev == 0) {
			u.u_error = EINVAL;
			printf("t_kopen: null device\n");
			return NULL;
		}
		/* make a vnode.
		 */
		vp = makespecvp(rdev, VCHR);

		/* this will call the streams open
		 * for us.
		 */
		if (u.u_error = VOP_OPEN(&vp, flags, u.u_cred)) {
			printf("t_kopen: VOP_OPEN: %d\n", u.u_error);
			return NULL;
		}
		/* allocate a file pointer, but
		 * no file descripter.
		 */
		if ((u.u_error = falloc(vp, flags, &fp, &fd)) != 0) {
			(void)VOP_CLOSE(vp, flags, 0, (off_t)0, u.u_cred);
			printf("t_kopen: falloc: %d\n", u.u_error);
			return NULL;
		}
		setf(fd, NULLFP);

		madefp = 1;
	}
	else	vp = (struct vnode *)fp->f_vnode;

	/* allocate a new transport structure
	 */
	tiptr = (TIUSER *)kmem_alloc((u_int)TIUSERSZ, KM_SLEEP);
	tiptr->fp = fp;

#ifdef KTLIDEBUG
printf("t_kopen: fp %x, vp %x, stp %x\n", fp, vp, vp->v_stream);
#endif

	/* see if TIMOD is already pushed
	 */
	u.u_error = strioctl(vp, I_FIND, "timod", 0, K_TO_K, u.u_cred, &retval);
	if (u.u_error) {
		kmem_free((caddr_t)tiptr, (u_int)TIUSERSZ);
		if (madefp)
			closef(fp);
		printf("t_kopen: strioctl(I_FIND, timod): %d\n", u.u_error);
		return NULL;
	}

	if (retval == 0) {
		u.u_error = strioctl(vp, I_PUSH, "timod", 0, K_TO_K, u.u_cred,
								 &retval);
		if (u.u_error) {
			kmem_free((caddr_t)tiptr, (u_int)TIUSERSZ);
			if (madefp)
				closef(fp);
			printf("t_kopen: I_PUSH (timod): %d", u.u_error);
			u.u_error = TSYSERR;
			printf(", vp %x, fp %x, stp %x\n", vp, fp, vp->v_stream);
			return NULL;
		}
	}

	inforeq.PRIM_type = T_INFO_REQ;
	strioc.ic_cmd = TI_GETINFO;
	strioc.ic_timout = 0;
	strioc.ic_dp = (char *)&inforeq;
	strioc.ic_len = sizeof(struct T_info_req);

	u.u_error = strdoioctl(vp->v_stream, &strioc, NULL, K_TO_K,
					 (char *)NULL, u.u_cred, &retval);
	if (u.u_error) {
		kmem_free((caddr_t)tiptr, (u_int)TIUSERSZ);
		if (madefp)
			closef(fp);
		printf("t_kopen: strdoioctl(T_INFO_REQ): %d\n", u.u_error);
		return NULL;
	}

	if (retval) {
		if ((retval & 0xff) == TSYSERR)
			u.u_error = (retval >> 8) & 0xff;
		else    u.u_error = tlitosyserr(retval & 0xff);
		kmem_free((caddr_t)tiptr, (u_int)TIUSERSZ);
		if (madefp)
			closef(fp);
		printf("t_kopen: strdoioctl(T_INFO_REQ): retval: 0x%x, %d\n", retval, u.u_error);
		return NULL;
	}

	if (strioc.ic_len != sizeof(struct T_info_ack)) {
		kmem_free((caddr_t)tiptr, (u_int)TIUSERSZ);
		if (madefp)
			closef(fp);
		u.u_error = EPROTO;
		printf("t_kopen: strioc.ic_len != sizeof (struct T_info_ack): %d\n", u.u_error);
		return NULL;
	}

	if (info != NULL) {
		info->addr = _t_setsize(inforeq.ADDR_size);
		info->options = _t_setsize(inforeq.OPT_size);
		info->tsdu = _t_setsize(inforeq.TSDU_size);
		info->etsdu = _t_setsize(inforeq.ETSDU_size);
		info->connect = _t_setsize(inforeq.CDATA_size);
		info->discon = _t_setsize(inforeq.DDATA_size);
		info->servtype = inforeq.SERV_type;
	}

	tiptr->tp_info.addr = inforeq.ADDR_size;
	tiptr->tp_info.options = inforeq.OPT_size;
	tiptr->tp_info.tsdu = inforeq.TSDU_size;
	tiptr->tp_info.etsdu = inforeq.ETSDU_size;
	tiptr->tp_info.connect = inforeq.CDATA_size;
	tiptr->tp_info.discon = inforeq.DDATA_size;
	tiptr->tp_info.servtype = inforeq.SERV_type;

	return (tiptr);
}

#define	DEFSIZE	128
static int
_t_setsize(infosize)
long infosize;
{
        switch(infosize)
        {
                case -1: return(DEFSIZE);
                case -2: return(0);
                default: return(infosize);
        }
}

/******************************************************************************/
