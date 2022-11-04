.ident	"@(#)libc-m32:sys/mctl.s	1.1"

# error = mctl(addr, len, function, arg)

	.set	__mctl,113*8

	.globl	_cerror

_fwdef_(`mctl'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__mctl,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
