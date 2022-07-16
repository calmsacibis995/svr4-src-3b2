#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# C library -- exit
.ident	"@(#)libc-m32:gen/cuexit.s	1.7"

# exit(code)
# code is return in r0 to system

	.set	_exit,1*8

	.globl	exit

_fgdef_(exit):
	MCOUNT
	call	&0,_fref_(_exithandle)
	MOVW	&4,%r0
	MOVW	&_exit,%r1
	GATE
