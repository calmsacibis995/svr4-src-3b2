	.file	"fpgetsticky.s"
.ident	"@(#)libc-m32:fp/fpgetsticky.s	1.11"
#----------------------------------
	.data
	.set	PS,18		# inexact sticky
	.set	IS,9		# invalid sticky
	.set	OS,8		# overflow sticky
	.set	US,7		# underflow sticky
	.set	QS,6		# divide by zero sticky
#----------------------------------
	.text
	.globl	_fp_hw		# to check hardware presence
	.globl	_asr		# simulated arithmetic status register
	.globl	_fpstart	# to force floating point startup
#----------------------------------
	.align	4
_fwdef_(`fpgetsticky'):
	MCOUNT
	TSTW	_dref_(_fp_hw)
	je	.soft
#----------------------------------
# MAU
	save	&0
	addw2	&4,%sp
	mmovfa	0(%fp)
	movw	0(%fp),%r2
	extzv	&QS,&4,%r2,%r0		# get IS,OS,US,QS
	llsw2	&1,%r0			# << 1 for inexact sticky
	extzv	&PS,&1,%r2,%r1		# get PS
	addw2	%r1,%r0			# add to the others
	ret	&0
#----------------------------------
	.align	4
.soft:					# running software
	movw	_dref_(_asr),%r2
	extzv	&QS,&4,%r2,%r0		# get IS,OS,US,QS
	llsw2	&1,%r0			# << 1 for inexact sticky
	extzv	&PS,&1,%r2,%r1		# get PS
	addw2	%r1,%r0			# add to the others
	RET

