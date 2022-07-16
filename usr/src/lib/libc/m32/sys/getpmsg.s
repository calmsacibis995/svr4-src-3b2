#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/getpmsg.s	1.2"

# C library -- getpmsg

	.set	__getpmsg,132*8
	.set	ERESTART,91

	.globl	_cerror

_fwdef_(`getpmsg'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getpmsg,%r1
	GATE
	jgeu	.noerror
	CMPB	&ERESTART,%r0
	BEB	getpmsg
	jmp	_cerror
.noerror:
	RET
