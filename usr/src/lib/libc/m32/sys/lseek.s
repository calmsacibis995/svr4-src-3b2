#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- lseek
.ident	"@(#)libc-m32:sys/lseek.s	1.8"

# error = lseek(file, offset, ptr);


	.set	__lseek,19*8

	.globl  _cerror

_fwdef_(`lseek'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__lseek,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
