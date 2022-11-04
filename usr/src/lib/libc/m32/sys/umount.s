# C library -- umount
.ident	"@(#)libc-m32:sys/umount.s	1.6"

	.set	__umount,22*8

	.globl	_cerror

_fwdef_(`umount'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__umount,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
