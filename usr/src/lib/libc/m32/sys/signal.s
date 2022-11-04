	.file	"signal.s"
.ident	"@(#)libc-m32:sys/signal.s	1.19.1.12"

	.globl	signal
	.align	4
_fgdef_(signal):
	jmp	_dref_(__signal)	# Do the work.
