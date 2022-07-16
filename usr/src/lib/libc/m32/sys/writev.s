#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/writev.s	1.3"

# error = writev(fd, iovp, iovcnt)

	.set	__writev,122*8
	.set	ERESTART,91

	.globl	_cerror

_fwdef_(`writev'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__writev,%r1
	GATE
	jgeu 	.noerror
	CMPB	&ERESTART,%r0
	BEB	writev
	jmp 	_cerror
.noerror:
	RET
