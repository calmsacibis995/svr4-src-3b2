.ident	"@(#)libc-m32:sys/readlink.s	1.1"

# nchar = readlink(path, buffer, count)
# nchar == -1 means error

	.set	__readlink,90*8

	.globl	_cerror


_fwdef_(`readlink'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__readlink,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
