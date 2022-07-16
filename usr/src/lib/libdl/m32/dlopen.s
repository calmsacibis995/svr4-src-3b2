#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

#ident	"@(#)libdl:m32/dlopen.s	1.1"
# dlopen calls _dlopen in ld.so

	.globl	dlopen
	.globl	_dlopen

dlopen:
	jmp	_dlopen@PLT
