.ident	"@(#)libc-m32:sys/fmount.s	1.1"

	.set	__fmount,105*8

	.globl  _cerror

_fwdef_(`fmount'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__fmount,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
