	.file	"sigpending.s" 
	.ident	"@(#)libc-m32:sys/sigpending.s	1.4"

	.set	SYSGATE,1*4
	.set	SIGPENDING,99*8	

	.globl	__sigfillset

_fwdef_(`sigpending'):
	PUSHW	&1
	PUSHW	0(%ap)
	CALL	-8(%sp),_sref_(sys)
	RET

_fgdef_(__sigfillset):
	PUSHW	&2
	PUSHW	0(%ap)
	CALL	-8(%sp),_sref_(sys)
	RET

sys:
	MOVW	&4,%r0
	MOVW	&SIGPENDING,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror

.noerror:
	RET
