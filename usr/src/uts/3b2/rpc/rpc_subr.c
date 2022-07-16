/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)krpc:rpc/rpc_subr.c	1.1"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)rpc_subr.c 1.1 89/08/21 SMI"
#endif
/* 
 * Miscellaneous support routines for kernel implementation of RPC.
 *
 */

#include	<sys/param.h>
#include	<sys/types.h>

#ifdef _KERNEL
/*
 * Kernel level debugging aid. The global variable "rpclog" is a bit
 * mask which allows various types of debugging messages to be printed
 * out.
 * 
 *	rpclog & 1 	will cause actual failures to be printed.
 *	rpclog & 2	will cause informational messages to be
 *			printed on the client side of rpc.
 *	rpclog & 4	will cause informational messages to be
 *			printed on the server side of rpc.
 */

int rpclog = 0;

int
rpc_log(level, str, a1)
	ulong		level;
	register char	*str;
	register int	a1;

{
	if (level & rpclog)
		printf(str, a1);
}

#endif _KERNEL
		
