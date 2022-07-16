#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# ptrace -- C library
.ident	"@(#)libc-m32:sys/ptrace.s	1.6"

#	result = ptrace(req, pid, addr, data);

	.set	__ptrace,26*8

	.globl	_cerror
	.globl  errno

_fwdef_(`ptrace'):
	MCOUNT
	CLRW	_dref_(errno)
	MOVW	&4,%r0
	MOVW	&__ptrace,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
