# OS library -- nfssys
.ident	"@(#)libc-m32:sys/_nfssys.s	1.1"

# error = nfssys(opcode, arg)

	.set	__nfssys,152*8

	.globl	_cerror
	.globl	_nfssys

_fgdef_(_nfssys):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__nfssys,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	CLRW	%r0
	RET
