#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/seteguid.s	1.2"

# C library -- setegid, seteuid

	.set	__setegid,136*8
	.set	__seteuid,141*8

	.globl	_cerror

_fwdef_(`setegid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__setegid,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
_fwdef_(`seteuid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__seteuid,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
