#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- getuid
.ident	"@(#)libc-m32:sys/getuid.s	1.7"

# uid = getuid();
#  returns real uid

	.set	__getuid,24*8

_fwdef_(`getuid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getuid,%r1
	GATE
	RET
