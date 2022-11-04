# C library -- open
.ident	"@(#)libc-m32:sys/open.s	1.8"

# file = open(string, mode)

# file == -1 means error

	.set	__open,5*8

	.globl  _cerror

_fwdef_(`open'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__open,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
