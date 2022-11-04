.ident	"@(#)libc-m32:sys/fstatvfs.s	1.1"

# error = fstatvfs(fd, statbuf)

	.set	__fstatvfs,104*8

	.globl	_cerror

_fwdef_(`fstatvfs'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__fstatvfs,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
