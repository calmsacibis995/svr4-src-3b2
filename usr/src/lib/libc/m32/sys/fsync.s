.ident	"@(#)libc-m32:sys/fsync.s	1.1"

# error = fsync(fd)

	.set	__fsync,58*8

	.globl	_cerror

_fwdef_(`fsync'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__fsync,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
