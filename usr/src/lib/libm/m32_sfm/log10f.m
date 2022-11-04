	.ident	"@(#)libm:m32_sfm/log10f.m	1.3"
	.file	"log10f.c"
	.data
	.align	4
.ACOFF1:
	.word	0x3c5ed68a	# 0.013600954785943031
.ACOFF2:
	.word	0xbeee0830	# -0.4649062156677246
.BCOFF2:
	.word	0xc0b28622	# -5.5788736343383789
.L49:
	.word	0x00000000	# 0
.L50:
	.word	0x3f800000	# 1
.L51:
	.word	0x3f000000	# 0.5
.L64:
	.word	0x3f3504f3	# 0.70710676908493041
.L65:
	.word	0xb95e8083	# -0.00021219444170128554
.L66:
	.word	0x3f318000	# 0.693359375
.L69:
	.word	0x3fdbcb7b,0x1526e50e	 # 0.43429449200630187
	.text
	.align	4
	.globl	log10f
log10f:
	save	&.R1
	MCOUNT
	addw2	&.F1,%sp
#TYPE	SINGLE
	movw	0(%ap),0(%fp)
	mmovss	.L50,%f0

	EXTFW	&8-1,&23,0(%fp),%r0	# get exponent
	jz	.denormal
	cmpw	%r0,&0x7f8
	je	.out
.norm:
	insv	&127-1,&23,&8,0(%fp)# Force the result
					# exponent to biased 0
	subw2	&127-1,%r0 #Bias the exponent appropriately

	mmovss	0(%fp),%f2
	mfcmpts	%f2,.L64
	jge	.L63
	mmovss	.L51,%f0
	DECW	%r0
.L63:
	mfadds3	%f2,%f0,%x3
	mfsubs3	%f0,%f2,%x0
	mfdivs3	%f3,%f0,%x2
	mfadds2	%f2,%x2
	mfmuls3	%f2,%f2,%x1
	mfmuls3	%f1,.ACOFF1,%x0
	mfadds3	.ACOFF2,%f0,0(%fp)
	mfadds3	.BCOFF2,%f1,%x3
	mfmuls3	%f2,%f1,%x0
	mfmuls2	0(%fp),%x0
	mfdivs2	%f3,%x0
	mfadds2	%f0,%x2
	MOVW	%r0,0(%fp)
	mmovws	0(%fp),%f1
	mfmuls3	.L65,%f1,%x0
	mfadds2	%f0,%x2
	mfmuls3	.L66,%f1,%x0
	mfadds2	%f2,%x0
	mfmuld2	.L69,%f0
	mmovss	%s0,0(%fp)
	jmp	.out

.denormal:
	mfcmps	0(%fp),.L49
	jle	.out
	addw2	&1,%r0
	andw3	&0x80000000,0(%fp),%r2	# remember sign

.norm_loop:				# normalization loop
	llsw2	&1,0(%fp)		# word << 1
	subw2	&1,%r0			# exp--
	bitw	&0x800000,0(%fp)	# is fraction normalized?
	je	.norm_loop		# 	no, left shift again
					# yes,  %r0 has the right exponent
	orw2	%r2,%r0			# put the sign back
	jmp	.norm			# and join main code


.out:
#TYPE	SINGLE
	MOVW	0(%fp),%r0
#REGAL	0	NODBL
#REGAL	0	PARAM	0(%ap)	4	FP
	.set	.F1,16
	.set	.R1,0
	ret	&.R1#1
	.size	log10f,.-log10f
	.text
