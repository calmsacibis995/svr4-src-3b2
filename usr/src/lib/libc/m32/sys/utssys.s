#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# OS library -- utssys
.ident	"@(#)libc-m32:sys/utssys.s	1.1"

# error = utssys(cbuf, mv, type, outbufp)

	.set	__utssys,57*8

	.globl	_cerror

_fwdef_(`utssys'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__utssys,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
