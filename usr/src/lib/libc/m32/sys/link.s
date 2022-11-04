# C library -- link
.ident	"@(#)libc-m32:sys/link.s	1.8"

# error = link(old-file, new-file);

	.set	__link,9*8

	.globl	_cerror

_fwdef_(`link'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__link,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
