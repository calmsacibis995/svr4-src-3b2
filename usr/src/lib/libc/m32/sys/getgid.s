#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- getgid
.ident	"@(#)libc-m32:sys/getgid.s	1.7"

# gid = getgid();
# returns real gid

	.set	__getgid,47*8

_fwdef_(`getgid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getgid,%r1
	GATE
	RET
