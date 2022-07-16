#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# getpid -- get process ID
.ident	"@(#)libc-m32:sys/getpid.s	1.7"

	.set	__getpid,20*8

_fwdef_(`getpid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getpid,%r1
	GATE
	RET
