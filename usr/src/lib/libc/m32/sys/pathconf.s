#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- pathconf
.ident	"@(#)libc-m32:sys/pathconf.s	1.3"

# error = pathconf(path, name)

	.set	__pathconf,113*8

	.globl	_cerror


_fwdef_(`pathconf'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__pathconf,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
