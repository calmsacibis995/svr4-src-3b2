#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- dup
.ident	"@(#)libc-m32:sys/dup.s	1.6"

#	f = dup(of [ ,nf])
#	f == -1 for error

	.set	__dup,41*8

	.globl	_cerror

_fwdef_(`dup'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__dup,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
