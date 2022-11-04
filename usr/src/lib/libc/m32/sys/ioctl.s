# C library -- ioctl
.ident	"@(#)libc-m32:sys/ioctl.s	1.9"

	.set	__ioctl,54*8
	.set	ERESTART,91

	.globl	_cerror

_fwdef_(`ioctl'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__ioctl,%r1
	GATE
	jgeu	.noerror
	CMPB	&ERESTART,%r0
	BEB	ioctl
	jmp	_cerror
.noerror:
	RET
