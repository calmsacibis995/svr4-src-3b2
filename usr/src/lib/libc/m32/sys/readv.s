#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/readv.s	1.3"

# error = readv(fd, iovp, iovcnt)

	.set	__readv,121*8
	.set	ERESTART,91

	.globl	_cerror

_fwdef_(`readv'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__readv,%r1
	GATE
	jgeu 	.noerror
	CMPB	&ERESTART,%r0
	BEB	readv
	jmp 	_cerror
.noerror:
	RET
