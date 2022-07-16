#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

	.file	"signal.s"
.ident	"@(#)libc-m32:sys/signal.s	1.19.1.12"

	.globl	signal
	.align	4
_fgdef_(signal):
	jmp	_dref_(__signal)	# Do the work.
