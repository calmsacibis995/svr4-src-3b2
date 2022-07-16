#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- read
.ident	"@(#)libc-m32:sys/read.s	1.9"

# nread = read(file, buffer, count);
# nread ==0 means eof; nread == -1 means error

	.set	__read,3*8
	.set	ERESTART,91

	.globl  _cerror

_fwdef_(`read'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__read,%r1
	GATE
	jgeu 	.noerror
	CMPB	&ERESTART,%r0
	BEB	read
	jmp 	_cerror
.noerror:
	RET
