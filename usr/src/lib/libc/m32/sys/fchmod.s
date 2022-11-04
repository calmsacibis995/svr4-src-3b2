.ident	"@(#)libc-m32:sys/fchmod.s	1.1"

# error = fchmod(fd, mode)

	.set	__fchmod,93*8

	.globl	_cerror

_fwdef_(`fchmod'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__fchmod,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
