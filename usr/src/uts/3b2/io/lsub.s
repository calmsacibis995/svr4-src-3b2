#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# Double long subtraction routine.  Ported from pdp 11/70 version
# with considerable effort.  All supplied comments were ported.

.ident	"@(#)kernel:io/lsub.s	1.1"

	.file	"lsub.s"
	.globl	lsub
	.text
	.align	4

	.set	lop,0
	.set	rop,8
	.set	ans,0

lsub:
	SAVE	%fp
#	MCOUNT

	MCOMW	rop(%ap),%r0
	MCOMW	rop+4(%ap),%r1
	INCW	%r1
	BCCB	lsub1
	INCW	%r0

lsub1:
	ADDW2	lop(%ap),%r0
	ADDW2	lop+4(%ap),%r1
	BCCB	lsub2
	INCW	%r0

lsub2:
	MOVW	%r0,ans(%r2)
	MOVW	%r1,ans+4(%r2)
	MOVAW	0(%r2),%r0

	RESTORE	%fp
	RET
