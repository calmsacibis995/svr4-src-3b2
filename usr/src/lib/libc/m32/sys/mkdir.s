# C library -- mkdir
	.ident	"@(#)libc-m32:sys/mkdir.s	1.8"
	.file	"mkdir.s"

# file = mkdir(string, mode);
# file == -1 if error

	.set	__mkdir,80*8

	.globl	_cerror

_fwdef_(`mkdir'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__mkdir,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
