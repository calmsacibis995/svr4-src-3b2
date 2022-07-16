#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- fpathconf
.ident	"@(#)libc-m32:sys/fpathconf.s	1.3"

# error = fpathconf(fd, name)

	.set	__fpathconf,118*8

	.globl	_cerror


_fwdef_(`fpathconf'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__fpathconf,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
