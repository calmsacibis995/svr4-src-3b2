# C library -- kill
.ident	"@(#)libc-m32:sys/kill.s	1.8"

	.set	__kill,37*8

	.globl	_cerror

_fwdef_(`kill'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__kill,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	CLRW	%r0
	RET
