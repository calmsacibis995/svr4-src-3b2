# C library -- mount
.ident	"@(#)libc-m32:sys/mount.s	1.6"

# error = mount(dev, file, flag)

	.set	__mount,21*8

	.globl  _cerror

_fwdef_(`mount'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__mount,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
