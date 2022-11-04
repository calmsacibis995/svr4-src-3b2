# C library -- sigaction

	.file	"_sigaction.s"
	.ident	"@(#)libc-m32:sys/_sigaction.s	1.4"

	.set	SIGACTION,98*8
	.set	SYSGATE,1*4

	.globl  _cerror
	.globl	__sigaction
	.globl	_siguhandler

_fgdef_(__sigaction):
	MCOUNT
	MOVW	&SYSGATE,%r0
	MOVW	&SIGACTION,%r1
	GATE
	jlu	_cerror
	RET
