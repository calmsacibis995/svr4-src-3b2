.ident	"@(#)libc-m32:sys/seteguid.s	1.1"

# C library -- setegid, seteuid

	.set	__setegid,140*8
	.set	__seteuid,141*8

	.globl	_cerror

_fwdef_(`setegid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__setegid,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
_fwdef_(`seteuid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__seteuid,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
