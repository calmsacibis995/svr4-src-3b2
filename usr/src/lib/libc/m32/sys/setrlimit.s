.ident	"@(#)libc-m32:sys/setrlimit.s	1.1"
# OS library -- setrlimit

	.set	SETRLIMIT,128*8

	.globl	_cerror

_fwdef_(`setrlimit'):
	#
	MOVW	&4,%r0
	MOVW	&SETRLIMIT,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
