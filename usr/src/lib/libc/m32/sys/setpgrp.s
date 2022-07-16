#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- setpgrp, getpgrp
.ident	"@(#)libc-m32:sys/setpgrp.s	1.7"

	.set	__setpgrp,39*8

_fwdef_(`setpgrp'):
	MCOUNT
	PUSHW	&1
	CALL	-4(%sp),_fref_(pgrp)
	RET

_fwdef_(`getpgrp'):
	PUSHW	&0
	CALL	-4(%sp),_fref_(pgrp)
	RET

pgrp:
	MOVW	&4,%r0
	MOVW	&__setpgrp,%r1
	GATE
	RET
