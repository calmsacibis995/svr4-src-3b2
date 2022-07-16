#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- setsid, setpgid, getsid, getpgid
.ident	"@(#)libc-m32:sys/setsid.s	1.1"

	.set	__setsid,39*8	# same as setpgrp entry point

	.globl  _cerror

_fwdef_(`getsid'):
	PUSHW	&2
	PUSHW	0(%ap)
	CALL	-8(%sp),_fref_(pgrp)
	RET


_fwdef_(`setsid'):
	PUSHW	&3
	CALL	-4(%sp),_fref_(pgrp)
	RET

_fwdef_(`getpgid'):
	PUSHW	&4
	PUSHW	0(%ap)
	CALL	-8(%sp),_fref_(pgrp)
	RET

_fwdef_(`setpgid'):
	PUSHW	&5
	PUSHW	0(%ap)
	PUSHW	4(%ap)
	CALL	-12(%sp),_fref_(pgrp)
	RET

pgrp:
	MOVW	&4,%r0
	MOVW	&__setsid,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
