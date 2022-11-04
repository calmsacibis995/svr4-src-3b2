#  C - library stime
.ident	"@(#)libc-m32:sys/stime.s	1.6"

	.set	__stime,25*8

	.globl  _cerror

_fwdef_(`stime'):
	MCOUNT
	MOVW	*0(%ap),0(%ap)	# copy time to set
	MOVW	&4,%r0
	MOVW	&__stime,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
