#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

#ident	"@(#)libdl:m32/dlclose.s	1.1"
# dlclose calls _dlopen in ld.so

	.globl	dlclose
	.globl	_dlclose

dlclose:
	jmp	_dlclose@PLT
