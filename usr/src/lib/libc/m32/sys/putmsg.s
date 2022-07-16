#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/putmsg.s	1.5"

# C library -- putmsg

	.set	__putmsg,86*8
	.set	ERESTART,91

	.globl	_cerror

_fwdef_(`putmsg'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__putmsg,%r1
	GATE
	jgeu	.noerror
	CMPB	&ERESTART,%r0
	BEB	putmsg
	jmp	_cerror
.noerror:
	RET
