# C library - alarm
.ident	"@(#)libc-m32:sys/alarm.s	1.7"

	.set	__alarm,27*8

	.align	1
_fwdef_(`alarm'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__alarm,%r1
	GATE
	RET
