#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

	.file	"sigaltstk.s"
	.ident	"@(#)libc-m32:sys/sigaltstk.s	1.3"

	.set	SYSGATE,1*4
	.set	SIGALTSTACK,97*8	# second-level gate entry for signal

#
# sigaltstack(ss,oss) 
#
_fwdef_(`sigaltstack'):
	MCOUNT

	movw	&SYSGATE,%r0
	movw	&SIGALTSTACK,%r1
	GATE
	jlu	_cerror
	RET

