# C library -- uname
.ident	"@(#)libc-m32:sys/uname.s	1.8"

	.set	_utssys,57*8
	.set	__uname,0

	.globl	_cerror

_fwdef_(`uname'):
	MCOUNT
	MCOUNT
	PUSHW	0(%ap)
	PUSHW	&0
	PUSHW	&__uname
	CALL	-12(%sp),_sref_(sys)
	RET

sys:
	MOVW	&4,%r0
	MOVW	&_utssys,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	RET
