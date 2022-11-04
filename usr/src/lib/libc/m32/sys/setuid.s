# C library -- setuid
.ident	"@(#)libc-m32:sys/setuid.s	1.8"

# error = setuid(uid);

	.set	__setuid,23*8

	.globl  _cerror

_fwdef_(`setuid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__setuid,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
