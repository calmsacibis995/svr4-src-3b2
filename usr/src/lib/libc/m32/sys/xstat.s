#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/xstat.s	1.1"

# error = _xstat(version, string, statbuf)

	.set	__xstat,123*8

	.globl	_cerror
	.globl	_xstat

_fgdef_(`_xstat'):
	MOVW	&4,%r0
	MOVW	&__xstat,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	CLRW	%r0
	RET
