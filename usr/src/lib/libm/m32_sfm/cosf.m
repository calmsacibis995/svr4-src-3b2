	.ident	"@(#)libm:m32_sfm/cosf.m	1.3"
	.file	"cosf.c"
	.data
	.align	4
.POS_ONE:
	.word	0x3f800000	# 1
.NEG_ONE:
	.word	0xbf800000	# -1
.L51:
	.word	0x3fc90fdb	# 1.5707963705062866
.COFF1:
	.word	0x362e9c5b	# 2.6019031338364584e-06
.COFF2:
	.word	0xb94fb222	# -0.00019807417993433773
.COFF3:
	.word	0x3c08873e	# 0.008333025500178337
.COFF4:
	.word	0xbe2aaaa4	# -0.16666656732559204
.trail:
	.word	0x3c91a626,0x33145c07
.lead:
	.word	0x3ff921fb,0x54442d18
	.text
	.align	4
	.globl	cosf
cosf:
	save	&.R1
	MCOUNT
	addw2	&.F1,%sp
	mmovsd	0(%ap),%f1
	cmpw	_fp_hw,&2	# do we have 32206 MAU?
	jne	.MAU106C
	mfcosd2	%f1,%f2
	jmp	.out
.MAU106C:
	mfabsd2	%f1,%f2

	mfsubd2	.lead,%x2
	mfsubd2	.trail,%x2
	mfnegd2	%x2,%x2
.L52:
	mfmuls3	%f2,%f2,%x1		# y = x*x
	mfmuls3	%f1,.COFF1,%x0
	mfadds2	.COFF2,%x0
	mfmuls2	%f1,%x0
	mfadds2	.COFF3,%x0
	mfmuls2	%f1,%x0
	mfadds2	.COFF4,%x0
	mfmuls2	%f2,%x1
	mfmuls2	%f0,%x1
	mfadds3	%f1,%f2,%x2

	mfcmpts	%f2,.POS_ONE
	jg	.P_ONE
	mfcmpts	%f2,.NEG_ONE
	jge	.out

	mmovss	.NEG_ONE,%f2
	jmp	.out
.P_ONE:
	mmovss	.POS_ONE,%f2
.out:
	mmovss	%f2,0(%fp)
#TYPE	SINGLE
	MOVW	0(%fp),%r0
#REGAL	0	NODBL
#REGAL	0	PARAM	0(%ap)	4	FP
	.set	.F1,8
	.set	.R1,0
	ret	&.R1#1
	.size	cosf,.-cosf
	.text
