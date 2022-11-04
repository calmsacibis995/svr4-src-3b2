# C library -- chmod
.ident	"@(#)libc-m32:sys/chmod.s	1.8"

# error = chmod(string, mode);

	.set	__chmod,15*8

	.globl	_cerror

_fwdef_(`chmod'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__chmod,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
