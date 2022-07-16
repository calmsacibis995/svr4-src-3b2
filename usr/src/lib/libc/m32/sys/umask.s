#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

#  C library -- umask
.ident	"@(#)libc-m32:sys/umask.s	1.6"
 
#  omask = umask(mode);
 
	.set	__umask,60*8

	.globl	_cerror

_fwdef_(`umask'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__umask,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
