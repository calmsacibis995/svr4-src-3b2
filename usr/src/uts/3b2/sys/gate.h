/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_GATE_H
#define _SYS_GATE_H

#ident	"@(#)head.sys:sys/gate.h	11.2"
/*
 *  Gate table descriptions
 */

typedef struct gate_l2 * gate_l1;	/* level 1 gate table entry */

struct gate_l2				/* level 2 gate table entry */
	{
	psw_t psw;
	int (*pc)();
	};

#endif	/* _SYS_GATE_H */
