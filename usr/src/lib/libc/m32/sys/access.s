#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# access(file, request)
.ident	"@(#)libc-m32:sys/access.s	1.8"
#  test ability to access file in all indicated ways
#  1 - read
#  2 - write
#  4 - execute

	.set	__access,33*8

	.globl	_cerror
_fwdef_(`access'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__access,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
