.ident	"@(#)libc-m32:sys/poll.s	1.4"

# C library -- poll

	.set	__poll,87*8

	.globl	_cerror

_fwdef_(`poll'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__poll,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
