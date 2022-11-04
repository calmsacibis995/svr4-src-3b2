# C library -- fcntl
.ident	"@(#)libc-m32:sys/fcntl.s	1.9"

	.set	__fcntl,62*8
	.set	ERESTART,91

	.globl	_cerror

_fwdef_(`fcntl'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__fcntl,%r1
	GATE
	jgeu	.noerror
	CMPB	&ERESTART,%r0
	BEB	fcntl
	jmp	_cerror
.noerror:
	RET
