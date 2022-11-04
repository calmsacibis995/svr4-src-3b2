.ident	"@(#)libc-m32:sys/putmsg.s	1.4"

# C library -- putmsg

	.set	__putmsg,86*8

	.globl	_cerror

_fwdef_(`putmsg'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__putmsg,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
