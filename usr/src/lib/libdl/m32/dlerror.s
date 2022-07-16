#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

#ident	"@(#)libdl:m32/dlerror.s	1.1"
# dlerror calls _dlerror in ld.so

	.globl	dlerror
	.globl	_dlerror

dlerror:
	jmp	_dlerror@PLT
