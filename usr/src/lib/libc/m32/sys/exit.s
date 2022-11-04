# C library -- _exit
.ident	"@(#)libc-m32:sys/exit.s	1.4"

# _exit(code)
# code is return in r0 to system
# Same as plain exit, for user who want to define their own exit.

	.set	exit,1*8

	.globl	_exit

_fgdef_(_exit):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&exit,%r1
	GATE
