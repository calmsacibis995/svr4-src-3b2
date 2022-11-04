	.ident	"@(#)libm:m32_sfm/atanf.m	1.3"
	.file	"atanf.c"
	.data
	.align	4
.PCOFF1:
	.word	0x3e8c6cad	# 0.27426663041114807
.PCOFF2:
	.word	0x4057d69b	# 3.3724734783172607
.PCOFF3:
	.word	0x408f70a6	# 4.4825010299682617
.QCOFF2:
	.word	0x409bbb84	# 4.8666400909423828
.QCOFF3:
	.word	0x408f70a6	# 4.4825010299682617
.L50:
	.word	0x3ff00000,0x0	# 1
.L53:
	.word	0x40012134,0xa0418937	# 2.4142136573791503
.L56:
	.word	0x3fda8279,0xa0000000	# 0.41421356797218322
.L58:
	.word	0x3ff921fb,0x54442d18	# pi/2
.L61:
	.word	0x3fe921fb,0x54442d18 	# pi/4
.ZRO:
	.word	0x0,0x0		# 0
	.text
	.align	4
	.globl	atanf
atanf:
	save	&.R1
	MCOUNT
	addw2	&.F1,%sp
	mmovsd	0(%ap),%f2

	CLRW	%r1
	mfcmptd	%f2,.ZRO
	jge	.POS
	INCW	%r1
	mfnegd1	%f2
.POS:
	cmpw	_fp_hw,&2	# determine hardware type
	jne	.MAU106
	CLRW	%r2
	mfcmptd	%f2,.L50	# if (|x| > 1.)
	jle	.less1
	mfdivd3	%d2,.L50,%d2	# x = 1/x
	INCW	%r2
.less1:
	mfatnd1	%d2
	TSTW	%r2		# if > 1
	jz	.L59
	mfsubd3	%d2,.L58,%d2 # Pi/2 - atan(x)
	jmp	.L59
.MAU106:
	CLRW	%r0
	mfcmptd	%f2,.L53
	jle	.L52
	mfdivd3	%f2,.L50,%f2
	INCW	%r0
	jmp	.L54
.L52:
	CLRW	%r2
	mfcmptd	%f2,.L56
	jle	.L54
	mfaddd3	%f2,.L50,%f1
	mfsubd3	.L50,%f2,%f0
	mfdivd3	%f1,%f0,%f2
	INCW	%r2
.L54:
	mfmuls3	%f2,%f2,%x1
	mfadds3	.QCOFF2,%f1,%x0
	mfmuls2	%f1,%x0
	mfadds3	.QCOFF3,%f0,%x3
	mfmuls3	%f1,.PCOFF1,%x0
	mfadds2	.PCOFF2,%x0
	mfmuls2	%f1,%x0
	mfadds2	.PCOFF3,%x0
	mfdivs2	%f3,%x0
	mfmuls2	%f0,%x2
	TSTW	%r0
	jz	.L57
	mfsubd3	%f2,.L58,%f2
	jmp	.L59
.L57:
	TSTW	%r2
	jz	.L59
	mfaddd2	.L61,%f2
.L59:
	TSTW	%r1
	jz	.OUT
	mfnegd1	%f2

.OUT:
	mmovds	%f2,0(%fp)
#TYPE	SINGLE
	MOVW	0(%fp),%r0
#REGAL	0	NODBL
#REGAL	0	PARAM	0(%ap)	4	FP
	.set	.F1,8
	.set	.R1,0
	ret	&.R1#1
	.size	atanf,.-atanf
	.text
