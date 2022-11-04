	.ident	"@(#)libm:m32_sfm/sinf.m	1.3"
	.file	"sinf.c"
	.data
	.align	4
.POS_ONE:
	.word	0x3f800000	# 1
.NEG_ONE:
	.word	0xbf800000	# -1
.L47:
	.word	0x00000000	# 0
.COFF1:
	.word	0x362e9c5b	# 2.6019031338364584e-06
.COFF2:
	.word	0xb94fb222	# -0.00019807417993433773
.COFF3:
	.word	0x3c08873e	# 0.008333025500178337
.COFF4:
	.word	0xbe2aaaa4	# -0.16666656732559204
	.text
	.align	4
	.globl	sinf
sinf:
	save	&.R1
	MCOUNT
	addw2	&.F1,%sp
	mmovss	0(%ap),%f2
	cmpw	_fp_hw,&2	# do we have 32206 attached?
	jne	.MAU106S
	mfsins1	%f2
	jmp	.out
.MAU106S:
	mfmuls3	%f2,%f2,%x1
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
	.size	sinf,.-sinf
	.text
