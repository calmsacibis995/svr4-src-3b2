/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _VM_KERNEL_H
#define _VM_KERNEL_H

#ident	"@(#)kernel:vm/kernel.h	1.3"

/*
 * Global variables for the kernel
 */

u_long	rmalloc();

/* 1.2 */
time_t boottime;
struct	timezone tz;			/* XXX */

long	avenrun[3];			/* FSCALED average run queue lengths */

#endif	/* _VM_KERNEL_H */
