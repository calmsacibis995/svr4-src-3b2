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
