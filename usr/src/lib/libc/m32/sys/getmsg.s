.ident	"@(#)libc-m32:sys/getmsg.s	1.4"

# C library -- getmsg

	.set	__getmsg,85*8

	.globl	_cerror

_fwdef_(`getmsg'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getmsg,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
