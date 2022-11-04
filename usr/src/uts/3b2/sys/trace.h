/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_TRACE_H
#define _SYS_TRACE_H

#ident	"@(#)head.sys:sys/trace.h	11.2"
/*
 * Per trace structure
 */
struct trace {
	struct 	clist tr_outq;
	short	tr_state;
	short	tr_chbits;
	short	tr_rcnt;
	unsigned char	tr_chno;
	char	tr_ct;
};

#endif	/* _SYS_TRACE_H */
