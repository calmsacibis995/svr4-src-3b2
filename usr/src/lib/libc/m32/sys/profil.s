#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# profil
.ident	"@(#)libc-m32:sys/profil.s	1.7"

	.set	_prof,44*8

_fwdef_(`profil'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&_prof,%r1
	GATE
	RET
