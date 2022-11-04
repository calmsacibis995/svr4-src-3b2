# C library-- nice
.ident	"@(#)libc-m32:sys/nice.s	1.6"

# error = nice(hownice)

	.set	__nice,34*8

	.globl  _cerror

_fwdef_(`nice'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__nice,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
