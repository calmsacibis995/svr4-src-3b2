# C library - pause
.ident	"@(#)libc-m32:sys/pause.s	1.8"

	.set	__pause,29*8

	.globl	_cerror

_fwdef_(`pause'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__pause,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
