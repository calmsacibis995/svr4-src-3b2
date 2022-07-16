#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/putpmsg.s	1.2"

# C library -- putpmsg

	.set	__putpmsg,133*8
	.set	ERESTART,91

	.globl	_cerror

_fwdef_(`putpmsg'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__putpmsg,%r1
	GATE
	jgeu	.noerror
	CMPB	&ERESTART,%r0
	BEB	putpmsg
	jmp	_cerror
.noerror:
	RET
