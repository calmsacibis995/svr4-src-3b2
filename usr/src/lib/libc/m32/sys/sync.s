#  C library -- sync
.ident	"@(#)libc-m32:sys/sync.s	1.5"

	.set	__sync,36*8

_fwdef_(`sync'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__sync,%r1
	GATE
	RET
