.ident	"@(#)libc-m32:sys/priocntlset.s	1.1"

	.set	_priocntlset,112*8

	.globl	__priocntlset
	.globl	_cerror

_fgdef_(`__priocntlset'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&_priocntlset,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
