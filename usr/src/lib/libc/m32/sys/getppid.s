#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# getppid -- get parent process ID
.ident	"@(#)libc-m32:sys/getppid.s	1.5"

	.set	__getpid,20*8

_fwdef_(`getppid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getpid,%r1
	GATE
	MOVW	%r1,%r0
	RET
