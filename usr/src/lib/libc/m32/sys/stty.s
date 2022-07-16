#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- stty
.ident	"@(#)libc-m32:sys/stty.s	1.6"

	.set	__stty,31*8

	.globl	_cerror

_fwdef_(`stty'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__stty,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
