#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

	.ident	"@(#)libm:m32_sfm/sqrtf.m	1.3"
	.file	"sqrtf.c"
	.text
	.align	4
	.globl	sqrtf
sqrtf:
	save	&.R1
	MCOUNT
	addw2	&.F1,%sp

	mfsqrs2	0(%ap),0(%fp)

#TYPE	SINGLE
	MOVW	0(%fp),%r0
#REGAL	0	NODBL
#REGAL	0	PARAM	0(%ap)	4	FP
	.set	.F1,8
	.set	.R1,0
	ret	&.R1#1
	.size	sqrtf,.-sqrtf
	.text
