# getppid -- get parent process ID
.ident	"@(#)libc-m32:sys/getppid.s	1.5"

	.set	__getpid,20*8

_fwdef_(`getppid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getpid,%r1
	GATE
	MOVW	%r1,%r0
	RET
