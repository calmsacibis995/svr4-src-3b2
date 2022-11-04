# C library -- stat
.ident	"@(#)libc-m32:sys/stat.s	1.8"

# error = stat(string, statbuf);
# char statbuf[36]

	.set	__stat,18*8

	.globl  _cerror

_fwdef_(`stat'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__stat,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
