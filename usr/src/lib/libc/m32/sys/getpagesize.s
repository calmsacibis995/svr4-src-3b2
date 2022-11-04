# OS library -- getpagesize
.ident	"@(#)libc-m32:sys/getpagesize.s	1.1"

# error = getpagesize()

	.set	__getpagesize,118*8

	.globl	_cerror

_fwdef_(`getpagesize'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getpagesize,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
