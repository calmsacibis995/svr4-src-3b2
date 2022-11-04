# OS library -- memcntl
.ident	"@(#)libc-m32:sys/memcntl.s	1.3"

# error = memcntl(addr, len, cmd, arg, attr, mask)

	.set	__memcntl,131*8

	.globl	_cerror

_fwdef_(`memcntl'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__memcntl,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
