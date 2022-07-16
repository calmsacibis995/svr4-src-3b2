#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# OS library -- mmap
.ident	"@(#)libc-m32:sys/mmap.s	1.1"

# error = mmap(addr, len, prot, flags, fd, off)

	.set	__mmap,115*8

	.globl	_cerror

_fwdef_(`mmap'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__mmap,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
