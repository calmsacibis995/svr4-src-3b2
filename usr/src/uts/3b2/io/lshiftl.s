#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Shift a double long value

.ident	"@(#)kernel:io/lshiftl.s	1.1"

	.file	"lshiftl.s"
	.globl	lshiftl
	.text
	.align	4

	.set	arg,0
	.set	cnt,8

lshiftl:
	SAVE	%r8
#	MCOUNT

	MOVW	arg(%ap),%r0
	MOVW	arg+4(%ap),%r1
	MOVW	cnt(%ap),%r8
	BLB	lshiftln
	BEB	lshiftld

# We are doing a positive (left) shift

lshiftlp:
	LLSW3	&1,%r0,%r0
	CMPW	&0,%r1
	BGEB	lshiftlp1
	ORW2	&1,%r0

lshiftlp1:
	LLSW3	&1,%r1,%r1
	DECW	%r8
	BGB	lshiftlp
	BRB	lshiftld

# We are doing a negative (right) shift.

lshiftln:
	MNEGW	%r8,%r8

lshiftln1:
	LRSW3	&1,%r1,%r1
	BITW	&1,%r0
	BEB	lshiftln2
	ORW2	&0x80000000,%r1

lshiftln2:
	LRSW3	&1,%r0,%r0
	DECW	%r8
	BGB	lshiftln1

# We are done.  Answer is in %r0,%r1.

lshiftld:
	MOVW	%r0,0(%r2)
	MOVW	%r1,4(%r2)
	MOVAW	0(%r2),%r0

	RESTORE	%r8
	RET
