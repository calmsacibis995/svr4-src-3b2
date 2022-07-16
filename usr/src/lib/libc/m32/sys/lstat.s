#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/lstat.s	1.1"

# error = lstat(string, statbuf)
# char statbuf[36]

	.set	__lstat,88*8

	.globl	_cerror

_fwdef_(`lstat'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__lstat,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	CLRW	%r0
	RET
