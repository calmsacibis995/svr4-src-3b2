#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- chown
.ident	"@(#)libc-m32:sys/chown.s	1.8"

# error = chown(string, owner);

	.set	__chown,16*8

	.globl	_cerror

_fwdef_(`chown'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__chown,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
