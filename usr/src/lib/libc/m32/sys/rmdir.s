#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- rmdir
	.ident	"@(#)libc-m32:sys/rmdir.s	1.9"
	.file	"rmdir.s"

	.set	__rmdir,79*8

	.globl	_cerror

_fwdef_(`rmdir'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__rmdir,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
