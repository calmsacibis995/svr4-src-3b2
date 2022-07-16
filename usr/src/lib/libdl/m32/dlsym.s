#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

#ident	"@(#)libdl:m32/dlsym.s	1.1"
# dlsym calls _dlsym in ld.so

	.globl	dlsym
	.globl	_dlsym

dlsym:
	jmp	_dlsym@PLT
