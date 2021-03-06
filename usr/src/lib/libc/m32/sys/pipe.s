#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# pipe -- C library
.ident	"@(#)libc-m32:sys/pipe.s	1.8"

#	pipe(f)
#	int f[2];

	.set	__pipe,42*8

	.globl  _cerror

_fwdef_(`pipe'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__pipe,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	MOVW	%r0,*0(%ap)
	MOVW	0(%ap),%r0
	MOVW	%r1,4(%r0)
	CLRW	%r0
	RET
