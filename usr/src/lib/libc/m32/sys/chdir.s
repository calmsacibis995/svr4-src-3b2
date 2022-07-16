#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- chdir
.ident	"@(#)libc-m32:sys/chdir.s	1.6"

# error = chdir(string);

	.set	__chdir,12*8

	.globl	_cerror

_fwdef_(`chdir'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__chdir,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
