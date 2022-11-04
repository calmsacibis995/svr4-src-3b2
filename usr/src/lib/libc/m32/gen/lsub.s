# Double long subtraction routine.  Ported from pdp 11/70 version
# with considerable effort.  All supplied comments were ported.

.ident	"@(#)libc-m32:gen/lsub.s	1.1"

	.file	"lsub.s"
	.text
	.align	4

	.set	lop,0
	.set	rop,8
	.set	ans,0

_fwdef_(`lsub'):
	SAVE	%fp
#	MCOUNT

	MCOMW	rop(%ap),%r0
	MCOMW	rop+4(%ap),%r1
	INCW	%r1
	BCCB	lsub1
	INCW	%r0

lsub1:
	ADDW2	lop(%ap),%r0
	ADDW2	lop+4(%ap),%r1
	BCCB	lsub2
	INCW	%r0

lsub2:
	MOVW	%r0,ans(%r2)
	MOVW	%r1,ans+4(%r2)
	MOVAW	0(%r2),%r0

	RESTORE	%fp
	RET
