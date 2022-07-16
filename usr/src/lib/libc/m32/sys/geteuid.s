#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- geteuid
.ident	"@(#)libc-m32:sys/geteuid.s	1.7"

# uid = geteuid();
#  returns effective uid

	.set	__getuid,24*8

_fwdef_(`geteuid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getuid,%r1
	GATE
	MOVW	%r1,%r0
	RET
