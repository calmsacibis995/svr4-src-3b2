.ident	"@(#)libc-m32:sys/getpmsg.s	1.1"

# C library -- getpmsg

	.set	__getpmsg,132*8

	.globl	_cerror

_fwdef_(`getpmsg'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getpmsg,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
