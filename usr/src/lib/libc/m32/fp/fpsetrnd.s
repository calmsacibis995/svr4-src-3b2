	.file	"fpsetrnd.s"
.ident	"@(#)libc-m32:fp/fpsetrnd.s	1.5.1.9"
	.text
#----------------------------------
	.globl	_fp_hw		# to check hardware presence
	.globl	_asr		# simulated arithmetic status register
	.globl  __flt_rounds     # ANSI rounding mode value
	.set	CSC_BIT,0x20000	# context switch control bit in ASR
				# is manually set after a "mmovta"
				# so that the new ASR is not lost on switch
#----------------------------------
	.align	4
#----------------------------------
_fwdef_(`fpsetround'):
	MCOUNT
	movw	0(%ap),%r0		# new rounding mode
					# set ansi round value
	ANDW2	&3,%r0			# mask off all but last 2 bits
	TSTW	%r0			# is round-to-zero?
	jne	.t1
	movw	&1,_dref_(__flt_rounds)
	jmp	.ismau
.t1:
	cmpw	%r0,&1			# is round to +inf?
	jne	.t2
	movw	&2,_dref_(__flt_rounds)
	jmp	.ismau
.t2:
	cmpw	%r0,&2			# is round to -inf?
	jne	.t3
	movw	&3,_dref_(__flt_rounds)
	jmp	.ismau
.t3:
	CLRW	_dref_(__flt_rounds)	# must be round to 0
.ismau:
	TSTW	_dref_(_fp_hw)
	je	.soft
#----------------------------------
# MAU
	save	&0
	addw2	&4,%sp
	mmovfa	0(%fp)			# read ASR
	extzv	&22,&2,0(%fp),%r0	# read old rounding mode
	insv	0(%ap),&22,&2,0(%fp)	# put new rounding mode
	orw2	&CSC_BIT,0(%fp)		# make sure this ASR is saved
	mmovta	0(%fp)			# write ASR back
	ret	&0
#----------------------------------
	.align	4
.soft:					# running software
	extzv	&22,&2,_dref_(_asr),%r0	# extract rounding mode
	insv	0(%ap),&22,&2,_dref_(_asr)	# put new rounding mode
	RET

