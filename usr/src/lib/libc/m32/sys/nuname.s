#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/nuname.s	1.1"
# OS library -- nuname

# error or system version = nuname(buf)

	.set	__nuname,135*8

	.globl	_cerror

_fwdef_(`nuname'):
	#
	MOVW	&4,%r0
	MOVW	&__nuname,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
