	.file	"expf.s"
	.ident	"@(#)libm:m32mau/expf.s	1.5"
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
.LN_MINFLOAT:
	.word	0xc2ce8ed0
.MINFLOAT:
	.word	0x1
.d_zero:
	.word	0x0,0x0
.LN_MAXFLOAT:
	.word	0x42b17218
.MAXFLOAT:
	.word	0x7f7fffff
.HUGE:
	.word	0x47efffff,0xe0000000
.HUGE_VAL:
	.word	0x7ff00000,0x0
.MLOG2E:
	.word	0x3fb8aa3b
.C1:
	.word	0x3f318000	# 0.693359375
.C2:
	.word	0xb95e8083	# -2.1219444005469058277e-4
#----------------------------------------------------------
	.text
	.set	.F1,40
				# struct exception {
	.set	type,0		# 	int type;
	.set	name,4		#	char *name;
	.set	arg1,8		#	double arg1;
	.set	arg2,16		#	double arg2;
	.set 	retval,24	# 	double retval;
				#};
	.set 	stow,32		# temporary
	.globl	expf
	.align	4
	.set	.R1,1
#--------------------------------------------------------------
expf:
	save	&.R1
	addw2	&.F1,%sp
	MCOUNT
	mmovss	0(%ap),%s2	# register float x
	mfabss2	%s2,%s1		# |x|
	mfcmpts	%s1,.F_X_EPS	#if (|x| < F_X_EPS)
	jge	.L42
	mfadds3 %d2,.f_one,stow(%fp)	# return 1.0 + x
	movw	stow(%fp),%r0
	ret	&.R1
.L42:
	mfcmpts	%s2,.LN_MINFLOAT	# if (x <= LN_MINFLOAT)
	jg	.L46
	jne	.L48		
	movw	.MINFLOAT,%r0	# return MINFLOAT
	ret	&.R1
.L48:
	movw	&4,type(%fp)		#exc.type = UNDERFLOW
	mmovdd	.d_zero,retval(%fp) 	#exc.retval = 0.0
	jmp	..0
.L46:
	mfcmpts	%s2,.LN_MAXFLOAT	# if (x >=LN_MAXFLOAT)
	jl	.L53
	jne	.L55
	movw	.MAXFLOAT,%r0	#return MAXFLOAT
	ret	&.R1
.L55:
	movw	&3,type(%fp)	#exc.type = OVERFLOW
	cmpw	_lib_version,&0		# if (_lib_version == c_issue_4)
	jne	.hval
	mmovdd	.HUGE,retval(%fp)	# exc.retval = HUGE
	jmp	..0
.hval:
	mmovdd	.HUGE_VAL,retval(%fp)	# else exc.retval = HUGE_VAL
..0:
	mmovsd	%s2,arg1(%fp)	 	# exc.arg1 = x;
	movw	&.expf,name(%fp)	# exc.name = "expf"
	pushaw	type(%fp)
	cmpw	_lib_version,&2		# if (_lib_version==strict_ansi)
	jne	.nstrict
	movw	&34,errno
	jmp	.errret
.nstrict:
	call	&1,matherr
	cmpw	%r0,&0
	jne	.errret		# if (!matherr(&exc)) errno = ERANGE
	movw	&34,errno
.errret:
	mmovds	retval(%fp),stow(%fp)	#return exc.retval
	movw	stow(%fp),%r0
	ret	&.R1
.L53:
	mfmuls3	.MLOG2E,%d2,%d0	# x * M_LOG2E
				# assign y in %s1
	mfrnds2	%d0,%d1		#y = (float)(int)(x*M_LOG2E)
	mmovsw	%s1,stow(%fp)
	movw	stow(%fp),%r8	# n=(int)(x * M_LOG2E + 0.5)
	mfmuls3	.C1,%d1,%d0	# y * C1
	mfsubs2 %d0,%d2		# x - y * C1
	mfmuls2	.C2,%d1		# y * C2
	mfsubs2	%d1,%d2		# x = (x -y * C1) - y *C2
.L63:
	mfmuls3	%d2,%d2,%d1	# y = x * x
	mfmuls3	%d1,.p,%d0	# p[0]*y
	mfadds2	.p+4,%d0	# +p[1]
	mfmuls2	%d0,%d2		# x *=
	mfmuls3	%d1,.q,%d0	# y*q[0]
	mfadds2	.q+4,%d0	#+q[1]
	mfsubs2	%d2,%d0		# -x
	mfdivs2	%d0,%d2		# x/(POLY1(y,q) -x)
	mfadds2	.f_half,%d2	# +0.5
				# inline expand ldexp(z,n+1)
	addw2	&1,%r8		# n++
	je	.L90		# if exp adjust == 0
	mmovss	%s2,0(%ap)
	je	.L91		# if value == 0 just return it
	extzv	&23,&8,0(%ap),%r0	# get exponent
	addw2	%r8,%r0			# add exp adjust
	jle	.call_ldexp	# if exp <=0 call ldexp
	cmpw	%r0,&255	# if exp > max exp call ldexp
	jg	.call_ldexp
	insv	%r0,&23,&8,0(%ap)  	#otherwise, jam in exponent
.L91:	movw	0(%ap),%r0	# return in %r0
	ret	&.R1
#--------------------------------------------------------------
.call_ldexp:
	mmovsd	%s2,stow(%fp)
	pushw	stow(%fp)
	pushw	stow+4(%fp)
	pushw	%r8
	call	&3,ldexp
	movw	%r0,stow(%ap)
	movw	%r1,stow+4(%ap)
	mmovds	stow(%ap),%s2
.L90:
	mmovss	%s2,0(%ap)
	movw	0(%ap),%r0	# return x
	ret	&.R1
	.data
	.align	4
.expf:
	.byte	101,120,112,102,0
	.align	4
	.text
