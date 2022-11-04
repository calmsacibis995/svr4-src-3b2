
.ident	"@(#)libc-m32:sys/uadmin.s	1.6"
# C library -- uadmin

# file = uadmin(cmd,fcn[,mdep])

# file == -1 means error

	.set	__uadmin,55*8

	.globl  _cerror

_fwdef_(`uadmin'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__uadmin,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
