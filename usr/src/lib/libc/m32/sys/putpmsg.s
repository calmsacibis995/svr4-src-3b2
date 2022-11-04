.ident	"@(#)libc-m32:sys/putpmsg.s	1.1"

# C library -- putpmsg

	.set	__putpmsg,133*8

	.globl	_cerror

_fwdef_(`putpmsg'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__putpmsg,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
