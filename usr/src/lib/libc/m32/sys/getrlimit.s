.ident	"@(#)libc-m32:sys/getrlimit.s	1.1"
# OS library -- getrlimit

	.set	GETRLIMIT,129*8

	.globl	_cerror

_fwdef_(`getrlimit'):
	#
	MOVW	&4,%r0
	MOVW	&GETRLIMIT,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
