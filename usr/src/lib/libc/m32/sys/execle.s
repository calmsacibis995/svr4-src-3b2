#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- execle
.ident	"@(#)libc-m32:sys/execle.s	1.7"

# execle(file, arg1, arg2, ... , env);

	.globl	execve

_fwdef_(`execle'):
	MCOUNT
	PUSHW	0(%ap)		#  file
	PUSHAW	4(%ap)		#  argv
	PUSHW	-20(%sp)	#  env
	CALL	-12(%sp),_fref_(execve)
	RET
