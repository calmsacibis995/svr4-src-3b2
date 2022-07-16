#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- fstatf
.ident	"@(#)libc-m32:sys/fstatf.s	1.6"

# error = fstatf(file, statbuf, len);

# char statbuf[len]

	.set	__fstatf,83*8

	.globl	_cerror

_fwdef_(`fstatf'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__fstatf,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
