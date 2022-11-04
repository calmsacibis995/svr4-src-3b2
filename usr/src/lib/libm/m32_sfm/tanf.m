	.ident	"@(#)libm:m32_sfm/tanf.m	1.3"
	.file	"tanf.c"
	.data
	.align	4
.DZERO:
	.word	0x0,0x0		# 0
.trail:
	.word	0x3c91a626,0x33145c07
.lead:
	.word	0x3ff921fb,0x54442d18
.M_PI_4:
	.word	0x3fe921fb,0x54442d18
.PCOFF1:
	.word	0x3a8cec35	# 0.0010751547524705529
.PCOFF2:
	.word	0xbde41178	# -0.11136144399642944
.QCOFF1:
	.word	0x3c82daa2	# 0.015973392874002456
.QCOFF2:
	.word	0xbee3af08	# -0.44469475746154785
.QCOFF3:
	.word	0x3f800000	# 1
.L52:
	.word	0x3f490fdb	# 0.78539818525314331
.L53:
	.word	0x3fc90fdb	# 1.5707963705062866
	.text
	.align	4
	.globl	tanf
tanf:
	save	&.R1
	MCOUNT
	addw2	&.F1,%sp
	mmovsd	0(%ap),%f2
	cmpw	_fp_hw,&2	# do we have 32206?
	jne	.MAU106T
	mfsind2	%f2,%f0
	mfcosd2	%f2,%f1	
	mfdivd2	%f1,%f0		# tan = sin/cos
	mmovds	%f0,0(%fp)
	jmp	.out2
.MAU106T:
	CLRW	%r1
	mfcmptd	%f2,.DZERO
	jge	.L61
	mfnegd2	%f2,%f2
	INCW	%r1

.L61:
	CLRW	%r0
	mfcmptd	%f2,.M_PI_4
	jle	.L51
	mfsubd2	.lead,%f2
	mfsubd2	.trail,%f2
	INCW	%r0
.L51:

	mfmuls3	%f2,%f2,%x1
	mfmuls3	%f1,.PCOFF1,%x0
	mfadds2	.PCOFF2,%x0
	mmovdd	%f0,%f3

	mfmuls3	%f2,%f1,%x0
	mfmuld2	%f3,%x0
	mfadds2	%f0,%x2
	mfmuls3	%f1,.QCOFF1,%x0
	mfadds2	.QCOFF2,%x0
	mfmuls2	%f1,%x0
	mfadds3	.QCOFF3,%f0,%x1

	TSTW	%r1
	je	.L65
	mfnegd2	%f2,%f2

.L65:
	TSTW	%r0
	je	.L55
	mfnegd2	%f1,%f0
	mfdivd3	%f2,%f0,0(%fp)

	jmp	.out
.L55:
	mfdivd3	%f1,%f2,0(%fp)
.out:
	mmovds	0(%fp),0(%fp)
.out2:
#TYPE	SINGLE
	MOVW	0(%fp),%r0
#REGAL	0	NODBL
#REGAL	0	PARAM	0(%ap)	4	FP
	.set	.F1,12
	.set	.R1,0
	ret	&.R1#1
	.size	tanf,.-tanf
	.text
