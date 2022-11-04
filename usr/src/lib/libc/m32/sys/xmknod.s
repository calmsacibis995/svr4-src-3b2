.ident	"@(#)libc-m32:sys/xmknod.s	1.1"

# error = _xmknod(version, string, mode, dev)

	.set	__xmknod,126*8

	.globl	_cerror
	.globl	_xmknod

_fgdef_(`_xmknod'):
	MOVW	&4,%r0
	MOVW	&__xmknod,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	CLRW	%r0
	RET
