#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# Determine the sign of a double-long number.

.ident	"@(#)kernel:io/lsign.s	1.1"

	.file	"lsign.s"
	.globl	lsign
	.text
	.align	4

lsign:
	SAVE	%fp
#	MCOUNT

	LRSW3	&31,0(%ap),%r0
	ANDW2	&1,%r0

	RESTORE	%fp
	RET
