# C library -- unlink
.ident	"@(#)libc-m32:sys/unlink.s	1.8"

# error = unlink(string);

	.set	__unlink,10*8

	.globl  _cerror

_fwdef_(`unlink'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__unlink,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
