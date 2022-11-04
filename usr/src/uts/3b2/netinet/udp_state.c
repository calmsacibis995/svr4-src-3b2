/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)netinet:netinet/udp_state.c	1.3"

/*
 * System V STREAMS TCP - Release 2.0
 * 
 * Copyright 1987, 1988 Lachman Associates, Incorporated (LAI) All Rights Reserved.
 * 
 * The copyright above and this notice must be preserved in all copies of this
 * source code.  The copyright above does not evidence any actual or intended
 * publication of this source code.
 * 
 * This is unpublished proprietary trade secret source code of Lachman
 * Associates.  This source code may not be copied, disclosed, distributed,
 * demonstrated or licensed except as expressly authorized by Lachman
 * Associates.
 * 
 * System V STREAMS TCP was jointly developed by Lachman Associates and
 * Convergent Technologies.
 */

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

#define STRNET

#ifdef INET
#include <netinet/symredef.h>
#endif INET

#include <sys/types.h>
#include <sys/param.h>

#include <sys/stream.h>
#include <sys/log.h>
#include <sys/strlog.h>
#ifdef SYSV
#include <sys/tihdr.h>
#include <sys/tiuser.h>
#else
#include <nettli/tihdr.h>
#include <nettli/tiuser.h>
#endif SYSV
#include <netinet/nihdr.h>

#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <net/route.h>
#include <netinet/in_pcb.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/ip_str.h>

mblk_t         *reallocb();

#define CHECKSIZE(bp,size) if (((bp) = reallocb((bp), (size),0)) == NULL) {\
			return;\
			}

/*
 * this is the subfunction of the upper put routine which handles data and
 * protocol packets for us.
 */

udp_state(q, bp)
	queue_t        *q;
	register mblk_t *bp;
{
	register union T_primitives *t_prim;
	register struct inpcb *inp = qtoinp(q);
	int             error = 0;
	mblk_t         *head;
	struct taddr_in *sin;
	struct in_addr  laddr;

	/*
	 * check for pending error, or a broken state machine
	 */

	STRLOG(UDPM_ID, 0, 9, SL_TRACE, "got to udp_state");
	if (inp->inp_error != 0) {
		T_errorack(q, bp, TSYSERR, inp->inp_error);
		return;
	}
	/* just send pure data, if we're ready */
	if (bp->b_datap->db_type == M_DATA) {
		if ((inp->inp_state & SS_ISCONNECTED) != 0) {
			udp_output(inp, bp);
		} else {
			CHECKSIZE(bp, sizeof(struct T_error_ack));
			bp->b_datap->db_type = M_PROTO;
			t_prim = (union T_primitives *) bp->b_rptr;
			bp->b_wptr = bp->b_rptr + sizeof(struct T_error_ack);
			t_prim->type = T_ERROR_ACK;
			t_prim->error_ack.ERROR_prim = T_DATA_REQ;
			t_prim->error_ack.TLI_error = TOUTSTATE;
			qreply(q, bp);
		}
		return;
	}
	/* if it's not data, it's proto or pcproto */

	t_prim = (union T_primitives *) bp->b_rptr;
	STRLOG(UDPM_ID, 0, 9, SL_TRACE, "Proto msg, type is %d", t_prim->type);

	switch (t_prim->type) {
	case T_INFO_REQ:
		/* our state doesn't matter here */
		CHECKSIZE(bp, sizeof(struct T_info_ack));
		bp->b_rptr = bp->b_datap->db_base;
		bp->b_wptr = bp->b_rptr + sizeof(struct T_info_ack);
		t_prim = (union T_primitives *) bp->b_rptr;
		t_prim->type = T_INFO_ACK;
		t_prim->info_ack.TSDU_size = 16 * 1024;
		t_prim->info_ack.ETSDU_size = -2;
		t_prim->info_ack.CDATA_size = -2;	/* ==> not supported */
		t_prim->info_ack.DDATA_size = -2;
		t_prim->info_ack.ADDR_size = sizeof(struct taddr_in);
		t_prim->info_ack.OPT_size = -1;
		t_prim->info_ack.TIDU_size = 16 * 1024;
		t_prim->info_ack.SERV_type = T_CLTS;
		t_prim->info_ack.CURRENT_state = inp->inp_tstate;
		qreply(q, bp);
		break;

	case T_BIND_REQ:
		if (inp->inp_tstate != TS_UNBND) {
			T_errorack(q, bp, TOUTSTATE, 0);
			break;
		}
		if (t_prim->bind_req.ADDR_length == 0) {
			error = in_pcbbind(inp, (mblk_t *) NULL);
		} else {
			if (t_prim->bind_req.ADDR_length
			    != sizeof(struct taddr_in)) {
				T_errorack(q, bp, TBADADDR, 0);
				break;
			}
			bp->b_rptr += t_prim->bind_req.ADDR_offset;
			error = in_pcbbind(inp, bp);
			bp->b_rptr -= t_prim->bind_req.ADDR_offset;
		}
		if (error == EACCES) {
			T_errorack(q, bp, TACCES, 0);
			error = 0;
			break;
		}
		else if (error)
			break;
		inp->inp_tstate = TS_IDLE;
		if ((bp = reallocb(bp, sizeof(struct T_bind_ack)
				   + sizeof(struct taddr_in), 1))
		    == NULL) {
			return;
		}
		t_prim = (union T_primitives *) bp->b_rptr;
		t_prim->bind_ack.PRIM_type = T_BIND_ACK;
		t_prim->bind_ack.ADDR_length = sizeof(struct taddr_in);
		t_prim->bind_ack.ADDR_offset = sizeof(struct T_bind_req);
		sin = (struct taddr_in *)
			(bp->b_rptr + sizeof(struct T_bind_ack));
		bp->b_wptr = (unsigned char *)
			(((caddr_t) sin) + sizeof(struct taddr_in));
		bzero((caddr_t) sin, sizeof(struct taddr_in));
		sin->sin_family = AF_INET;
		sin->sin_addr = inp->inp_laddr;
		sin->sin_port = inp->inp_lport;
		qreply(q, bp);
		break;

	case T_UNBIND_REQ:
		if (inp->inp_tstate != TS_IDLE) {
			T_errorack(q, bp, TOUTSTATE, 0);
			break;
		}
		in_pcbdisconnect(inp);
		inp->inp_laddr.s_addr = INADDR_ANY;
		inp->inp_lport = 0;
		inp->inp_tstate = TS_UNBND;
		T_okack(q, bp);
		break;

		/*
		 * Initiate connection to peer. For udp this is simply faked
		 * by asigning a pseudo-connection, and sending up a
		 * conection confirmation.
		 */
	case T_CONN_REQ:{
			if (inp->inp_tstate != TS_IDLE) {
				T_errorack(q, bp, TOUTSTATE, 0);
				break;
			}
			bp->b_rptr += t_prim->conn_req.DEST_offset;
			error = in_pcbconnect(inp, bp);
			bp->b_rptr -= t_prim->conn_req.DEST_offset;
			if (error)
				break;
			T_okack(q, bp);
			T_conn_con(inp);
			break;
		}

	case T_DISCON_REQ:
		if (inp->inp_faddr.s_addr == INADDR_ANY) {
			error = ENOTCONN;
			break;
		}
		in_pcbdisconnect(inp);
		inp->inp_state &= ~SS_ISCONNECTED;	/* XXX */
		T_okack(q, bp);
		break;

	case T_OPTMGMT_REQ:
		udp_ctloutput(q, bp);
		break;

	case T_DATA_REQ:
		if ((inp->inp_state & SS_ISCONNECTED) == 0) {
			freemsg(bp);	/* TLI doesn't want errors here */
			break;
		}
		head = bp;
		bp = bp->b_cont;
		head->b_cont = NULL;
		freeb(head);
		if (bp == NULL) {
			break;
		}
		udp_output(inp, bp);
		break;

	case T_UNITDATA_REQ:
		laddr = inp->inp_laddr;
		if (inp->inp_faddr.s_addr != INADDR_ANY) {
			error = EISCONN;
			break;
		}
		if (bp->b_cont == NULL) {
			freeb(bp);
			break;
		}
		bp->b_rptr += t_prim->unitdata_req.DEST_offset;
		error = in_pcbconnect(inp, bp);
		bp->b_rptr -= t_prim->unitdata_req.DEST_offset;
		if (error)
			break;
		head = bp;
		bp = bp->b_cont;
		head->b_cont = NULL;
		freeb(head);
		udp_output(inp, bp);
		in_pcbdisconnect(inp);
		inp->inp_laddr = laddr;
		break;

	case T_CONN_RES:
	case T_ORDREL_REQ:
	case T_EXDATA_REQ:
	default:
		T_errorack(q, bp, TNOTSUPPORT, 0);
		return;
	}
	if (error)
		T_errorack(q, bp, TSYSERR, error);
}

udp_ctloutput(q, bp)
	queue_t        *q;
	mblk_t         *bp;
{
	int             in_pcboptmgmt(), ip_options();
	static struct opproc funclist[] = {
		SOL_SOCKET, in_pcboptmgmt,
		IPPROTO_IP, ip_options,
		0, 0
	};

	dooptions(q, bp, funclist);
}
