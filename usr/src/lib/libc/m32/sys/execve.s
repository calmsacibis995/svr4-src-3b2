#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- execve
.ident	"@(#)libc-m32:sys/execve.s	1.7"

# execve(file, argv, env);

# where argv is a vector argv[0] ... argv[x], 0
# last vector element must be 0

	.set	__exece,59*8

	.globl	_cerror

_fwdef_(`execve'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__exece,%r1
	GATE
	jmp 	_cerror
