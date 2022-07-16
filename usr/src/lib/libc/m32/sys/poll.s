#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/poll.s	1.5"

# C library -- poll

	.set	__poll,87*8
	.set	ERESTART,91

	.globl	_cerror

_fwdef_(`poll'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__poll,%r1
	GATE
	jgeu	.noerror
	CMPB	&ERESTART,%r0
	BEB	poll
	jmp	_cerror
.noerror:
	RET
