.ident	"@(#)libc-m32:sys/readv.s	1.2"

# error = readv(fd, iovp, iovcnt)

	.set	__readv,121*8

	.globl	_cerror

_fwdef_(`readv'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__readv,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
