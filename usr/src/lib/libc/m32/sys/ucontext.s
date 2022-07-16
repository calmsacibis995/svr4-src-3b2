#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- setcontext and getcontext 

	.file	"ucontext.s"
	.ident	"@(#)libc-m32:sys/ucontext.s	1.4"

	.set	SYSGATE,1*4
	.set	UCONTEXT,100*8
	.globl	__getcontext

_fgdef_(__getcontext):
	PUSHW	&0
	PUSHW	0(%ap)
	CALL	-8(%sp),_sref_(sys)
	RET

_fwdef_(`setcontext'):
	PUSHW	&1
	PUSHW	0(%ap)
	CALL	-8(%sp),_sref_(sys)
	RET

sys:
	MCOUNT
	MOVW	&SYSGATE,%r0
	MOVW	&UCONTEXT,%r1
	GATE
	jlu 	_cerror
	RET
