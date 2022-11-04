# C library -- close
.ident	"@(#)libc-m32:sys/close.s	1.8"

# error =  close(file);

	.set	__close,6*8

	.globl	_cerror

_fwdef_(`close'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__close,%r1
	GATE
	jgeu	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
