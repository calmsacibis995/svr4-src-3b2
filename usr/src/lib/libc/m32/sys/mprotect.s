#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# OS library -- mprotect
.ident	"@(#)libc-m32:sys/mprotect.s	1.1"

# error = mprotect(addr, len, prot)

	.set	__mprotect,116*8

	.globl	_cerror

_fwdef_(`mprotect'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__mprotect,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
