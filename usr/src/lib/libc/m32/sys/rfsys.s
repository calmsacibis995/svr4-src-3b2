.ident	"@(#)libc-m32:sys/rfsys.s	1.6"

	.set	__rfsys,78*8

	.globl	_cerror

_fwdef_(`rfsys'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__rfsys,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
