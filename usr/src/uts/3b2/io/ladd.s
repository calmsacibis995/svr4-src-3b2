#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# Double long add routine.  Ported from pdp 11/70 version
# with considerable effort.  All supplied comments were ported.

.ident	"@(#)kernel:io/ladd.s	1.1"

	.file	"ladd.s"
	.globl	ladd
	.text
	.align	4

	.set	lop,0
	.set	rop,8
	.set	ans,0

ladd:
	SAVE	%fp
#	MCOUNT

	ADDW3	lop(%ap),rop(%ap),%r0
	ADDW3	lop+4(%ap),rop+4(%ap),%r1
	BCCB	ladd1
	INCW	%r0

ladd1:
	MOVW	%r0,ans(%r2)
	MOVW	%r1,ans+4(%r2)
	MOVAW	0(%r2),%r0

	RESTORE	%fp
	RET
