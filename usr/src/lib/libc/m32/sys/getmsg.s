#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/getmsg.s	1.5"

# C library -- getmsg

	.set	__getmsg,85*8
	.set	ERESTART,91

	.globl	_cerror

_fwdef_(`getmsg'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getmsg,%r1
	GATE
	jgeu	.noerror
	CMPB	&ERESTART,%r0
	BEB	getmsg
	jmp	_cerror
.noerror:
	RET
