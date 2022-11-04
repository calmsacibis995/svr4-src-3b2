# Double long add routine.  Ported from pdp 11/70 version
# with considerable effort.  All supplied comments were ported.

.ident	"@(#)libc-m32:gen/ladd.s	1.1"

	.file	"ladd.s"
	.text
	.align	4

	.set	lop,0
	.set	rop,8
	.set	ans,0

_fwdef_(`ladd'):
	SAVE	%fp
#	MCOUNT

	ADDW3	lop(%ap),rop(%ap),%r0
	ADDW3	lop+4(%ap),rop+4(%ap),%r1
	BCCB	ladd1
	INCW	%r0

ladd1:
	MOVW	%r0,ans(%r2)
	MOVW	%r1,ans+4(%r2)
	MOVAW	0(%r2),%r0

	RESTORE	%fp
	RET
