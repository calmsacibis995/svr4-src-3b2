#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library - alarm
.ident	"@(#)libc-m32:sys/alarm.s	1.7"

	.set	__alarm,27*8

	.align	1
_fwdef_(`alarm'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__alarm,%r1
	GATE
	RET
