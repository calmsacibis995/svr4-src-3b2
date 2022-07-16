#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# OS library -- nfssys
.ident	"@(#)libc-m32:sys/_nfssys.s	1.2"

# error = nfssys(opcode, arg)

	.set	__nfssys,106*8

	.globl	_cerror
	.globl	_nfssys

_fgdef_(_nfssys):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__nfssys,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	CLRW	%r0
	RET
