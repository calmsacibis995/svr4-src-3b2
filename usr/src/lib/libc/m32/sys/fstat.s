#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- fstat
.ident	"@(#)libc-m32:sys/fstat.s	1.8"

# error = fstat(file, statbuf);

# char statbuf[34]

	.set	__fstat,28*8

	.globl	_cerror

_fwdef_(`fstat'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__fstat,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
