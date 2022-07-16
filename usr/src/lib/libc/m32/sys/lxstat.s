#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/lxstat.s	1.1"

# error = _lxstat(version, string, statbuf)

	.set	__lxstat,124*8

	.globl	_cerror
	.globl	_lxstat


_fgdef_(`_lxstat'):
	MOVW	&4,%r0
	MOVW	&__lxstat,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	CLRW	%r0
	RET
