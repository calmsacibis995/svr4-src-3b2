#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

	.file	"abs.s"
.ident	"@(#)libc-m32:gen/abs.s	1.5"
#	int
#	abs(arg)
#	int arg;
#	{
#		return((arg < 0)? -arg : arg);
#	}
	.globl	abs
	.globl	labs
	.align	4
_fgdef_(abs):
_fgdef_(labs):
	save	&0
	MCOUNT
	movw	0(%ap),%r0
	jnneg	.L0
	mnegw	%r0,%r0
.L0:
	ret	&0
