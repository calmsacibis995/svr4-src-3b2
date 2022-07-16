#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

#  C library -- sync
.ident	"@(#)libc-m32:sys/sync.s	1.5"

	.set	__sync,36*8

_fwdef_(`sync'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__sync,%r1
	GATE
	RET
