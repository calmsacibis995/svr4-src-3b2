#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/symlink.s	1.1"

# error = symlink(linkname, target)

	.set	__symlink,89*8

	.globl	_cerror


_fwdef_(`symlink'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__symlink,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	CLRW	%r0
	RET
