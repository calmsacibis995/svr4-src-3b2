	.file	"expf.s"
	.ident	"@(#)libm:m32_sfm/expf.m	1.3"
################################################################
#
#	float expf(x)
#	float x;
#	coefficients from Cody and Waite
##################################################################
	.data
	.align	4
.F_X_EPS:
	.word	0x323504f3
.FM_LN2:
	.word	0x3f317218
.2TO23:
	.word	0x4b000000
.p:
	.word	0x3b885308
	.word	0x3e800000
.q:
	.word	0x3d4cbf5b
	.word	0x3f000000
.f_zero:
	.word	0x0
.f_half:
	.word	0x3f000000
.f_one:
	.word	0x3f800000
.d_zero:
	.word	0x0,0x0
.HUGE:
	.word	0x7f7fffff
.HUGE_VAL:
	.word	0x7f800000
.MLOG2E:
	.word	0x3fb8aa3b
.C1:
	.word	0x3f318000	# 0.693359375
.C2:
	.word	0xb95e8083	# -2.1219444005469058277e-4
#----------------------------------------------------------
	.text
	.set 	stow,32		# temporary
	.globl	expf
	.align	4
#--------------------------------------------------------------
expf:
	save	&.R1
	addw2	&.F1,%sp
	MCOUNT
	mmovss	0(%ap),%s2	# register float x
	mfabss2	%s2,%s1		# |x|
	mfcmpts	%s1,.F_X_EPS	#if (|x| < F_X_EPS)
	jge	.L42
	mfadds3 %s2,.f_one,stow(%fp)	# return 1.0 + x
	movw	stow(%fp),%r0
	ret	&.R1
.L42:
	mfmuls3	.MLOG2E,%x2,%x0	# x * M_LOG2E
				# assign y in %s1
	mfrnds2	%x0,%x1		#y = (float)(int)(x*M_LOG2E)
	mmovsw	%s1,stow(%fp)
	movw	stow(%fp),%r8	# n=(int)(x * M_LOG2E + 0.5)
	mfmuls3	.C1,%x1,%x0	# y * C1
	mfsubs2 %x0,%x2		# x - y * C1
	mfmuls2	.C2,%x1		# y * C2
	mfsubs2	%x1,%x2		# x = (x -y * C1) - y *C2
.L63:
	mfmuls3	%x2,%x2,%x1	# y = x * x
	mfmuls3	.p,%x1,%x0	# p[0]*y
	mfadds2	.p+4,%x0	# +p[1]
	mfmuls2	%x0,%x2		# x *=
	mfmuls3	.q,%x1,%x0	# y*q[0]
	mfadds2	.q+4,%x0	#+q[1]
	mfsubs2	%x2,%x0		# -x
	mfdivs2	%x0,%x2		# x/(POLY1(y,q) -x)
	mfadds2	.f_half,%x2	# +0.5
				# inline expand ldexp(z,n+1)
	addw2	&1,%r8		# n++
	je	.L90		# if exp adjust == 0
	mmovss	%s2,0(%fp)
	je	.L90		# if value == 0 just return it
	extzv	&23,&8,0(%fp),%r2	# get exponent
	jz	.denormal	# If exponent is 0, input is denormalized
	cmpw	%r2,&0x7f8
	je	.done
#----------------------------------------------------------------------
.norm:
	addw2	%r8,%r2		# r2 := new biased exponent
	jnpos	.under		# If it's <= 0, we have an underflow
	cmpw	%r2,&0x7f8	# Otherwise check if it's too big
	jge	.over		# if ( expf >= 0x7f8) then overflow
#-----------------------------------------------------------------------
#	Construct the result by jamming the exponent and return

	insv	%r2,&23,&8,0(%fp)	# Put the exponent
							# back in the result
	jmp	.done
#-----------------------------------------------------------------------
.denormal:			# Multiply by 2 ** 23
				# to guarantee a normalized number
	mfmuls2	.2TO23,0(%fp)
	EXTFW	&8-1,&23,0(%fp),%r2	# biased exponent
	subw2	&23,%r2		# Reduce biased exponent
	jmp	.norm
#-------------------------------------------------------------------------
#	Underflow
.under:
	cmpw	%r2,&-23	# If it's too small for denormalized
	jl	.zero		# return zero
				# Reset the exponent in the result
	insv	&1,&23,&8,0(%fp)
	addw2	&126,%r2	# Prepare exponent for negative power of 2
	llsw2	&23,%r2		# Shift exponent into position
				# Multiply to produce denormalized result
	movw	%r2,28(%fp)
	mfmuls2	28(%fp),0(%fp)
	jmp	.done
#-------------------------------------------------------------------------
#	Return zero result
.zero:
	movw	&0,0(%fp)		# Result is zero
	jmp	.done
#-----------------------------------------------------------------------
#	Overflow
.over:
	movw	0(%fp),20(%fp)
	cmpw	_lib_version,&0 # if (_lib_version == c_issue_4)
	jne	.ansi
	movw	.HUGE,0(%fp)	# Largest possible floating magnitude
	jmp	.L91
.ansi:
	movw	.HUGE_VAL,0(%fp)	# Largest possible floating magnitude
.L91:
	jbc	&31,20(%fp),.done	# Jump if argument was positive
	orw2	&0x80000000,0(%ap)	# If arg < 0, make result negative
#-----------------------------------------------------------------------
.L90:
	mmovss	%s2,0(%fp)
.done:

#TYPE	SINGLE
	MOVW	0(%fp),%r0
#REGAL	0	NODBL
#REGAL	0	PARAM	0(%ap)	4	FP
	.set	.F1,40
	.set	.R1,1
	ret	&.R1#1
	.size	expf,.-expf
	.text
