# C library -- mknod
.ident	"@(#)libc-m32:sys/mknod.s	1.6"

# error = mknod(string, mode, major.minor);

	.set	__mknod,14*8

	.globl  _cerror

_fwdef_(`mknod'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__mknod,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
