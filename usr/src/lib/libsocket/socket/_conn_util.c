/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:_conn_util.c	1.5"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stream.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include <sys/socket.h>
#include <sys/sockmod.h>
#include <sys/tiuser.h>
#include <sys/signal.h>

extern int	errno;

/*
 * Snd_conn_req - send connect request message to 
 * transport provider
 */
int
_s_snd_conn_req(siptr, call)
	register struct _si_user	*siptr;
	register struct t_call		*call;
{
	register struct T_conn_req	*creq;
	register char			*buf;
	register int			size;
	struct strbuf			ctlbuf;
	
	buf = siptr->ctlbuf;
	creq = (struct T_conn_req *)buf;
	creq->PRIM_type = T_CONN_REQ;
	creq->DEST_length = call->addr.len;
	creq->DEST_offset = 0;
	creq->OPT_length = call->opt.len;
	creq->OPT_offset = 0;
	size = sizeof(struct T_conn_req);

	if (call->addr.len) {
		_s_aligned_copy(buf, call->addr.len, size,
			     call->addr.buf, &creq->DEST_offset);
		size = creq->DEST_offset + creq->DEST_length;
	}
	if (call->opt.len && call->opt.len != -1) {
		_s_aligned_copy(buf, call->opt.len, size,
			     call->opt.buf, &creq->OPT_offset);
		size = creq->OPT_offset + creq->OPT_length;
	}

	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = size;
	ctlbuf.buf = buf;

	if (putmsg(siptr->fd, &ctlbuf, (call->udata.len? &call->udata: NULL),
						 0) < 0)
		return -1;

	if (!_s_is_ok(siptr, T_CONN_REQ))
		return -1;
	return 0;
}

/*
 * Rcv_conn_con - get connection confirmation off
 * of read queue
 */
int
_s_rcv_conn_con(siptr)
	register struct _si_user	*siptr;
{
	struct strbuf			ctlbuf;
	register union T_primitives	*pptr;
	register int			retval;
	int				flg;

	flg = 0;
	if (siptr->udata.servtype == T_CLTS) {
		errno = EOPNOTSUPP;
		return -1;
	}

	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;

	/* no data expected.
	 */
	if ((retval = getmsg(siptr->fd, &ctlbuf, NULL, &flg)) < 0) {
		if (errno == ENXIO)
			errno = ECONNREFUSED;
		return -1;
	}
	/*
	 * did we get entire message 
	 */
	if (retval) {
		errno = EIO;
		return -1;
	}

	/*
	 * is cntl part large enough to determine message type?
	 */
	if (ctlbuf.len < sizeof(long)) {
		errno = EPROTO;
		return -1;
	}

	pptr = (union T_primitives *)ctlbuf.buf;
	switch(pptr->type) {
		case T_CONN_CON:
			return 0;

		case T_DISCON_IND:
			if (ctlbuf.len < sizeof(struct T_discon_ind))
				errno = ECONNREFUSED;
			else	errno = pptr->discon_ind.DISCON_reason;
			return -1;

		default:
			break;
	}

	errno = EPROTO;
	return -1;
}
