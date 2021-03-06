#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- setgid
.ident	"@(#)libc-m32:sys/setgid.s	1.8"

# error = setgid(uid);

	.set	__setgid,46*8

	.globl  _cerror

_fwdef_(`setgid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__setgid,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
