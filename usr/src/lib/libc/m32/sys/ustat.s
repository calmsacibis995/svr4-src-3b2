# C library-- ustat
.ident	"@(#)libc-m32:sys/ustat.s	1.6"

	.set	__utssys,57*8
	.set	__ustat,2

	.globl	_cerror

_fwdef_(`ustat'):
	MCOUNT
	PUSHW	4(%ap)
	PUSHW	0(%ap)
	PUSHW	&__ustat
	CALL	-12(%sp),_sref_(sys)
	RET

sys:
	MOVW	&4,%r0
	MOVW	&__utssys,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	CLRW	%r0
	RET
