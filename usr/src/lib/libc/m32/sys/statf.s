#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- statf
	.ident	"@(#)libc-m32:sys/statf.s	1.8"
	.file 	"statf.s"

# error = statf(string, statbuf,len);
# char statbuf[len]

	.set	__statf,82*8

	.globl  _cerror

_fwdef_(`statf'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__statf,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
