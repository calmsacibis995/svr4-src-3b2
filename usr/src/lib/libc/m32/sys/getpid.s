# getpid -- get process ID
.ident	"@(#)libc-m32:sys/getpid.s	1.7"

	.set	__getpid,20*8

_fwdef_(`getpid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getpid,%r1
	GATE
	RET
