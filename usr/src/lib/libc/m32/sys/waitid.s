#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/waitid.s	1.1"


# error = waitid(idtype,id,&info,options)

	.set	WAITID,107*8
	.set	ERESTART,91

	.globl  _cerror

_fwdef_(`waitid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&WAITID,%r1
	GATE
	jgeu 	.noerror
	CMPB	&ERESTART,%r0
	je	_dref_(waitid)
	jmp 	_cerror

.noerror:
	RET
