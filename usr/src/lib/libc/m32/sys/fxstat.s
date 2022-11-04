.ident	"@(#)libc-m32:sys/fxstat.s	1.1"

# error = _fxstat(version, string, statbuf)

	.set	__fxstat,125*8

	.globl	_cerror
	.globl	_fxstat


_fgdef_(`_fxstat'):
	MOVW	&4,%r0
	MOVW	&__fxstat,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	CLRW	%r0
	RET
