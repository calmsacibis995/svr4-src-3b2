/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:inc/m32/Machine.h	1.6"

// Machine dependent constants and macros

#define ELFMAGIC	0x7f454c46
#define COFFMAGIC	0560
#define EXECCNT		2

#define SIZEOF_XFLOAT	12	/* size (in bytes) of extended float */

#define BKPTSIZE	1
#define BKPTSIG		4
#define BKPTTEXT	"\57"

#define SYSCALL_FAILED()	(getreg(REG_PS) & 0x40000)
#define PROCKLUDGE	0x80000200
