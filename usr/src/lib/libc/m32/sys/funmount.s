.ident	"@(#)libc-m32:sys/funmount.s	1.1"

	.set	__funmount,106*8

	.globl	_cerror

_fwdef_(`funmount'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__funmount,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
