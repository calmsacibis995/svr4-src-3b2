# C library -- creat
.ident	"@(#)libc-m32:sys/creat.s	1.8"

# file = creat(string, mode);
# file == -1 if error

	.set	__creat,8*8

	.globl	_cerror

_fwdef_(`creat'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__creat,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
