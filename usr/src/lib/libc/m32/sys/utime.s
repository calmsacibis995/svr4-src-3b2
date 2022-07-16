#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- utime
.ident	"@(#)libc-m32:sys/utime.s	1.6"
 
#  error = utime(string,timev);
 
	.set	__utime,30*8

	.globl	_cerror
 
_fwdef_(`utime'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__utime,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
