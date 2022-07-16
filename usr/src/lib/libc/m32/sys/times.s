#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- times
.ident	"@(#)libc-m32:sys/times.s	1.8"

	.set	__times,43*8

	.globl	_cerror

_fwdef_(`times'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__times,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
