.ident	"@(#)libc-m32:sys/writev.s	1.2"

# error = writev(fd, iovp, iovcnt)

	.set	__writev,122*8

	.globl	_cerror

_fwdef_(`writev'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__writev,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
