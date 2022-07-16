#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- sigsendset
.ident	"@(#)libc-m32:sys/sigsendset.s	1.1"

#	sigsendset(psp,sig)

	.set	__sigsendset,108*8

	.globl  _cerror

_fwdef_(`sigsendset'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__sigsendset,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror

.noerror:
	RET
