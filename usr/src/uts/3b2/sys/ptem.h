/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_PTEM_H
#define _SYS_PTEM_H

#ident	"@(#)head.sys:sys/ptem.h	11.5"

/*
 * The ptem data structure used to define the global data
 * for the psuedo terminal emulation streams module
 */
struct ptem
{
	long cflags;		/* copy of c_cflags */
	mblk_t *dack_ptr;	/* pointer to preallocated message block used to ACK disconnect */
	queue_t *q_ptr;		/* pointer to the ptem read queue */
	struct winsize wsz;	/* struct to hold the windowing info. */
	unsigned short state;	/* state of ptem entry, free or not */
};
/*
 * state flags
 * if state is zero then ptem entry is free to be allocated
 */
#define INUSE 	0x1
/*
 * Constants used to distinguish between a common function invoked
 * from the read or write side put procedures
 */
#define	RDSIDE	1
#define	WRSIDE	2

#endif	/* _SYS_PTEM_H */
