	.ident	"@(#)libc-m32:fp/fpsetmask.s	1.4.1.9"
	.file	"fpsetmask.s"
# fp_except fpsetmask(mask)
# 	fp_except mask;
# set exception masks as defined by user and return
# previous setting
# any sticky bit set whose corresponding mask is dis-abled
# is cleared
#----------------------------------------------------------------
	.text
	.set	.F1,4
	.set	.R1,2
	.set	INV,16		# invalid op mask
	.set	OFL,8		# overflow mask
	.set 	UFL,4		# underflow mask
	.set	DZ,2		# divide by 0 mask
	.set	IMP,1		# imprecise mask
	.set	IS,9		# invalid sticky
	.set	OS,8		# overflow sticky
	.set	US,7		# underflow sticky
	.set	QS,6		# divide by 0 sticky
	.set	PS,18 		# imprecise sticky
	.set	PM,10 		# imprecise mask - _asr
	.set	CSC,17		# context switch bit

	.globl	_fp_hw		# to check hardware presence
	.globl	_asr		# simulated arithmetic status register

	.align	4
#----------------------------------------------------------------
_fwdef_(`fpsetmask'):
	MCOUNT
	TSTW	_dref_(_fp_hw)
	je	.soft
#----------------------------------
# MAU
	save	&.R1
	addw2	&.F1,%sp
	movw	0(%ap),%r8		# new mask setting
	mmovfa	0(%fp)			# get ASR
	movw	0(%fp),%r7
	andw3	&0x7c00,%r7,%r0		# get old masks from asr
	LRSW3	&10,%r0,%r0		# old mask for return
#
# for each mask bit set, clear corresponding sticky bit
#
	bitw	%r8,&INV		# invalid op
	jz	.L10
	insv	&0,&IS,&1,%r7
.L10:
	bitw	%r8,&OFL		# overflow
	jz	.L11
	insv	&0,&OS,&1,%r7
.L11:
	bitw	%r8,&UFL		# underflow
	jz	.L12
	insv	&0,&US,&1,%r7
.L12:
	bitw	%r8,&DZ			# divide by zero
	jz	.L13
	insv	&0,&QS,&1,%r7
.L13:
	bitw	%r8,&IMP		# imprecise
	jz	.L14
	insv	&0,&PS,&1,%r7
.L14:
	LLSW3	&10,%r8,%r8		# prepare to store new mask
	andw2	&0xffff83ff,%r7		# clear out old mask bits
	orw2	%r8,%r7			# or in new bits
	insv	&1,&CSC,&1,%r7		# set CSC bit
	movw	%r7,0(%fp)		# write in memory
	mmovta	0(%fp)			# write ASR
	ret	&.R1

#----------------------------------
	.align	4
.soft:					# running software
	extzv	&PM,&5,_dref_(_asr),%r0	# old mask bits
	insv	0(%ap),&PM,&5,_dref_(_asr)	# put in new mask bits
	RET

	.text
