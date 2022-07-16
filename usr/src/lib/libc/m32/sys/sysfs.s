#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/sysfs.s	1.5"
# C library -- sysfs

	.set	__sysfs,84*8

	.globl	_cerror

_fwdef_(`sysfs'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__sysfs,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
