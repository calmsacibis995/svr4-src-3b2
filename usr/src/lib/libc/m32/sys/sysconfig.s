# OS library -- _sysconfig
.ident	"@(#)libc-m32:sys/sysconfig.s	1.1"

# error = _sysconfig(name)

	.set	__sysconfig,137*8

	.globl	_sysconfig
	.globl	_cerror


_fgdef_(_sysconfig):
	MOVW	&4,%r0
	MOVW	&__sysconfig,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
