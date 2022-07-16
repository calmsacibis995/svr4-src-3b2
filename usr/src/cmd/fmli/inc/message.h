/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 *
 */

#ident	"@(#)vm.inc:src/inc/message.h	1.4"

extern	int Mess_lock;
#define mess_lock()	(Mess_lock++)
#define mess_unlock()	(Mess_lock = 0)
#define MESSIZ	(256)  /* that should be wider than any screen */
