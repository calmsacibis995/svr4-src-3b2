# C library -- rmdir
	.ident	"@(#)libc-m32:sys/rmdir.s	1.9"
	.file	"rmdir.s"

	.set	__rmdir,79*8

	.globl	_cerror

_fwdef_(`rmdir'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__rmdir,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
