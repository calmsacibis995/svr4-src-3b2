#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# OS library -- munmap
.ident	"@(#)libc-m32:sys/munmap.s	1.1"

# error = munmap(addr, len)

	.set	__munmap,117*8

	.globl	_cerror

_fwdef_(`munmap'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__munmap,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
