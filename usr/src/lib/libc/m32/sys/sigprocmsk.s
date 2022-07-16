#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

	.file	"sigprocmsk.s"
	.ident	"@(#)libc-m32:sys/sigprocmsk.s	1.3"

	.set	SYSGATE,1*4
	.set	SIGPROCMASK,95*8
#
# sigprocmask(how,set,oset)		/* hold signal mask */ 
#
_fwdef_(`sigprocmask'):
	MCOUNT

	movw	&SYSGATE,%r0
	movw	&SIGPROCMASK,%r1
	GATE
	jlu	_cerror
	RET


