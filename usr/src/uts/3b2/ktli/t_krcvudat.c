/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktli:ktli/t_krcvudat.c	1.3"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)t_krcvudata.c 1.1 88/12/12 SMI"
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
 *	Kernel TLI-like function to read a datagram off of a
 *	transport endpoints stream head.
 *
 *	Returns:
 *		T_DATA		If normal data has been received
 *		T_UDERR		If an error indication has been received.
 *		-1 		On failure
 *
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/errno.h>
#include <sys/stream.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include <sys/timod.h>
#include <sys/tiuser.h>
#include <sys/t_kuser.h>


int
t_krcvudata(tiptr, unitdata, errtype)
register TIUSER *tiptr;
register struct t_kunitdata *unitdata;
register long *errtype;

{
	register int len, retval, hdrsz;
	register union T_primitives *pptr;
	register struct file *fp;
	mblk_t *bp;
	register mblk_t *nbp, *mp, *tmp;

	fp = tiptr->fp;

	retval = 0;
	unitdata->udata.buf = (char *)NULL;

	if (unitdata->udata.udata_mp) {
#ifdef KTLIDEBUG
printf("t_krcvudata: freeing existing message block\n");
#endif
		freemsg(unitdata->udata.udata_mp);
		unitdata->udata.udata_mp = NULL;
	}

	if (tli_recv(tiptr, &bp, fp->f_flag) < 0)
		return -1;

	/* Got something
	 */
	switch (bp->b_datap->db_type) {
		case M_DATA:
#ifdef KTLIDEBUG
printf("t_krcvudata: tli_recv returned M_DATA\n");
#endif
			while (bp->b_cont && bp->b_rptr >= bp->b_wptr) {
				nbp = bp;
				bp = bp->b_cont;
				freeb(nbp);
			}
			if ((bp->b_wptr - bp->b_rptr) != 0) {
				unitdata->udata.buf = (char *)bp->b_rptr;
				unitdata->udata.len = bp->b_wptr-bp->b_rptr;
				unitdata->udata.udata_mp = bp;
			}
			retval = T_DATA;
			break;

		case M_PROTO:
			/* LINTED pointer alignment */
			pptr = (union T_primitives *)bp->b_rptr;
			switch (pptr->type) {
				case T_UNITDATA_IND:
#ifdef KTLIDEBUG
printf("t_krcvudata: Got T_UNITDATA_IND\n");
#endif
					hdrsz = bp->b_wptr - bp->b_rptr;

					/* check everything for consistency
					 */
					if (hdrsz < TUNITDATAINDSZ ||
				 	 hdrsz < (pptr->unitdata_ind.OPT_length+
					 pptr->unitdata_ind.OPT_offset) ||
					 hdrsz < (pptr->unitdata_ind.SRC_length+
					 pptr->unitdata_ind.SRC_offset) ) {
						u.u_error = EPROTO;
						retval = -1;
						break;
					}

					/* okay, so now we copy them
					 */
					len = min(pptr->unitdata_ind.SRC_length,
							unitdata->addr.maxlen);
					bcopy(bp->b_rptr+pptr->unitdata_ind.SRC_offset,
						unitdata->addr.buf, len);
					unitdata->addr.len = len;

					len = min(pptr->unitdata_ind.OPT_length,
							unitdata->opt.maxlen);
					bcopy(bp->b_rptr+pptr->unitdata_ind.OPT_offset,
						unitdata->opt.buf, len);
					unitdata->opt.len = len;

					bp->b_rptr += hdrsz;

					/* we assume that the client knows
					 * how to deal with a set of linked
					 * mblks, so all we do is make a pass
					 * and remove any that are zero length.
					 */
					nbp = NULL;
					mp = bp;
					while (mp) {
						if (!(bp->b_wptr-bp->b_rptr)){
#ifdef KTLIDEBUG
printf("t_krcvudata: zero length block\n");
#endif
							tmp = mp->b_cont;
							if (nbp)
								nbp->b_cont = tmp;
							else	bp = tmp;

							freeb(mp);
							mp = tmp;
						}
						else	{
							nbp = mp;
							mp = mp->b_cont;
						}
					}
#ifdef KTLIDEBUG
{
mblk_t *tp;
tp = bp;
while (tp) {
	struct datab *dmp;

	dmp = tp->b_datap;
/*
	printf("t_krcvudata: bp %x, b_cont %x, rptr %x, wptr %x\n", tp, tp->b_cont, tp->b_rptr, tp->b_wptr);
	printf("t_krcvudata: db_base %x, db_lim %x, db_size %x, db_class %d, db_ref %x, db_type %x\n", dmp->db_base, dmp->db_lim, dmp->db_size, dmp->db_class, dmp->db_ref, dmp->db_type);
*/
	if (tp->b_datap->db_size < 0)
		printf("t_krcvudata: bp %x, func: %x, arg %x\n", tp, dmp->db_frtnp->free_func, dmp->db_frtnp->free_arg);
	tp = tp->b_cont;
}
}
#endif

					/* now just point the users mblk
					 * pointer to what we received.
					 */
					if (bp == NULL) {
						printf("t_krcvudata: No data\n");
						u.u_error = EPROTO; 
						retval = -1;
						break;
					}
					if ((bp->b_wptr - bp->b_rptr) != 0) {
						if (!str_aligned(bp->b_rptr))
							if (!pullupmsg(bp, bp->b_wptr - bp->b_rptr)) {
								printf("t_krcvudata:  pullupmsg failed\n");
								retval = -1;
								break;
							}
						unitdata->udata.buf = (char *)bp->b_rptr;
						unitdata->udata.len = bp->b_wptr-bp->b_rptr;

#ifdef KTLIDEBUG
printf("t_krcvudata: got %d bytes\n", unitdata->udata.len);
#endif
						unitdata->udata.udata_mp = bp;
					}
					else	printf("t_krcvudata: 0 length data message\n");
					retval = T_DATA;
					break;

				/* only other thing that can be there with
				 * CLTS is a datagram error indication.
				 */
				case T_UDERROR_IND:
#ifdef KTLIDEBUG
printf("t_krcvudata: Got T_UDERROR_IND\n");
#endif
					hdrsz = bp->b_wptr - bp->b_rptr;

					/* check everything for consistency
					 */
					if (hdrsz < TUDERRORINDSZ ||
				 	 hdrsz < (pptr->uderror_ind.OPT_length+
					 pptr->uderror_ind.OPT_offset) ||
					 hdrsz < (pptr->uderror_ind.DEST_length+
					 pptr->uderror_ind.DEST_offset) ) {
						u.u_error = EPROTO;
						retval = -1;
						break;
					}

					if (pptr->uderror_ind.DEST_length >
						(int)unitdata->addr.maxlen ||
					    pptr->uderror_ind.OPT_length >
						(int)unitdata->opt.maxlen) {
						u.u_error = TBUFOVFLW;
						retval = -1;
						break;
					}

					/* okay, so now we copy them
					 */
					bcopy(bp->b_rptr+pptr->uderror_ind.DEST_offset,
					unitdata->addr.buf,
					(int)pptr->uderror_ind.DEST_length);
					unitdata->addr.len = pptr->uderror_ind.DEST_length;

					bcopy(bp->b_rptr+pptr->uderror_ind.OPT_offset,
					unitdata->opt.buf,
					(int)pptr->uderror_ind.OPT_length);
					unitdata->addr.len = pptr->uderror_ind.OPT_length;

					*errtype =  pptr->uderror_ind.ERROR_type;
					unitdata->udata.buf = NULL;
					unitdata->udata.udata_mp = NULL;

					freemsg(bp);

					retval = T_UDERR;
					break;
				default:
					printf("t_krcvudata: Unknown transport primitive type %d\n", pptr->type);
					break;
			}
			break;

		default:
#ifdef KTLIDEBUG
printf("t_krcvudata: tli_recv returned unknown message type\n");
#endif
			u.u_error = EPROTO;
			retval = -1;
			break;
	}
	return retval;
}

/******************************************************************************/
