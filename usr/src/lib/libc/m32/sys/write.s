#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- write
.ident	"@(#)libc-m32:sys/write.s	1.9"

# nwritten = write(file, buffer, count);
# nwritten == -1 means error

	.set	__write,4*8
	.set	ERESTART,91

	.globl  _cerror

_fwdef_(`write'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__write,%r1
	GATE
	jgeu 	.noerror
	CMPB	&ERESTART,%r0
	BEB	write
	jmp 	_cerror
.noerror:
	RET
