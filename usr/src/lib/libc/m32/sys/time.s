#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- time
.ident	"@(#)libc-m32:sys/time.s	1.5"

# tvec = time(tvec);

	.set	__time,13*8

_fwdef_(`time'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__time,%r1
	GATE
	TSTW	0(%ap)
	je	nostore
	MOVW	%r0,*0(%ap)
nostore:
	RET
