/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktli:ktli/t_ksndudat.c	1.4"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)t_ksndudata.c 1.3 89/01/12 SMI"
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
 *	TLI-like function to send datagrams over a specified
 *	transport endpoint.
 *
 *	Returns:
 *		-1	on failure.
 *		0	on success, no data on receive queue.
 *		TLOOK	on success, and data pending on receive queue.
 *
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/stream.h>
#include <sys/strsubr.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include <sys/timod.h>
#include <sys/tiuser.h>
#include <sys/t_kuser.h>


int
t_ksndudata(tiptr, unitdata, frtn)
register TIUSER *tiptr;
register struct t_kunitdata *unitdata;
register frtn_t *frtn;
{
	register int msgsz, minsz, maxsz;
	register struct file *fp;
	register struct vnode *vp;
	register struct stdata *stp;
	register mblk_t *bp, *dbp;
	register struct T_unitdata_req *udreq;

	fp = tiptr->fp;
	vp = fp->f_vnode;
	stp = vp->v_stream;

	if (stp->sd_flag & (STWRERR|STRHUP|STPLEX))  {
		u.u_error = ((stp->sd_flag&STPLEX) ? EINVAL : stp->sd_werror);
		return -1;
	}

	/* check size constraints
	 */
	minsz = stp->sd_wrq->q_next->q_minpsz;
	maxsz = stp->sd_wrq->q_next->q_maxpsz;

	msgsz = unitdata->udata.len;
	if (msgsz > maxsz || msgsz < minsz) {
		u.u_error = ERANGE;
		return -1;
	}

	/* now check tsdu
	 */
	if (msgsz <= 0 || msgsz > tiptr->tp_info.tsdu) {
		u.u_error = ERANGE;
		return -1;
	}

	/* See if Class 0 is required
	 */
	if (frtn != NULL) {
		/* user has supplied their own buffer, all we have to
		 * do is allocate a class 0 streams buffer and set it
		 * up.
		 */
		if ((dbp = (mblk_t *)esballoc(unitdata->udata.buf,
					msgsz, BPRI_LO, frtn)) == NULL) {
			u.u_error = ENOSR;
			return -1;
		}
		dbp->b_datap->db_type = M_DATA;
#ifdef KTLIDEBUG
printf("t_ksndudata: bp %x, len %d, free func %x\n",dbp,msgsz,frtn->free_func);
#endif
	}
	else	
	if (unitdata->udata.buf) {
		while (!(dbp = allocb(msgsz, BPRI_LO)))
			if (strwaitbuf(msgsz, BPRI_LO)) {
				u.u_error = ENOSR;
				return -1;
			}

		bcopy(unitdata->udata.buf, dbp->b_wptr, unitdata->udata.len);
		dbp->b_datap->db_type = M_DATA;
	}
	else
	if (unitdata->udata.udata_mp) {
		/* user has done it all
		 */
		dbp = unitdata->udata.udata_mp;
		goto gotdp;
	}
	else	{
		/* zero length message.
		 */
		dbp = NULL;
	}

	if (dbp)
		dbp->b_wptr += msgsz;		/* on behalf of the user */

	/* Okay, put the control part in 
	 */
gotdp:
	msgsz = TUNITDATAREQSZ;
	while (!(bp = allocb(msgsz+unitdata->addr.len+unitdata->opt.len,
		 BPRI_LO)))
		if (strwaitbuf(msgsz, BPRI_LO)) {
			freeb(dbp);
			u.u_error = ENOSR;
			return -1;
		}

	/* LINTED pointer alignment */
	udreq = (struct T_unitdata_req *)bp->b_wptr;
	udreq->PRIM_type = T_UNITDATA_REQ;
	udreq->DEST_length = unitdata->addr.len;
	if (unitdata->addr.len) {
		bcopy(unitdata->addr.buf, (char *)(bp->b_wptr+msgsz), unitdata->addr.len);
	
		udreq->DEST_offset = msgsz;
		msgsz += unitdata->addr.len;
	}
	else	udreq->DEST_offset = 0;

	udreq->OPT_length = unitdata->opt.len;
	if (unitdata->opt.len) {
		bcopy(unitdata->opt.buf, (char *)(bp->b_wptr+msgsz), unitdata->opt.len);
		udreq->OPT_offset = msgsz;
		msgsz += unitdata->opt.len;
	}
	else	udreq->OPT_offset = 0;

	bp->b_datap->db_type = M_PROTO;
	bp->b_wptr += msgsz;

	/* link the two.
	 */
	linkb(bp, dbp);

	/* Put it to the transport provider
	 */
	 if (tli_send(tiptr, bp, fp->f_flag) < 0)
		return -1;

	unitdata->udata.udata_mp = 0;
	unitdata->udata.buf = 0;
	unitdata->udata.len = 0;

	/* before we return, see if there is anything waiting
	 * on our read queue.
	 */
	if (RD(stp->sd_wrq)->q_first)
		return TLOOK;

	return 0;
}

/******************************************************************************/

