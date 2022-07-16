#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

#  C library -- chroot
.ident	"@(#)libc-m32:sys/chroot.s	1.6"
 
#  error = chroot(string);
 
	.set	__chroot,61*8
 
	.globl	_cerror

_fwdef_(`chroot'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__chroot,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
