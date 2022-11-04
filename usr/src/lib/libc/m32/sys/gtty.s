# C library -- gtty
.ident	"@(#)libc-m32:sys/gtty.s	1.6"

	.set	__gtty,32*8

	.globl	_cerror

_fwdef_(`gtty'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__gtty,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
