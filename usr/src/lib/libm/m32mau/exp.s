	.file	"exp.s"
	.ident	"@(#)libm:m32mau/exp.s	1.6"
#####################################################################
#	double exp(x)
#	double x;
#	Coefficients from Cody and Waite (1980).
####################################################################
	.data
	.align	4
.exp:					# "exp"
	.byte	101,120,112,0
	.align	4
.d_zero:				# 0.0
	.word	0x0,0x0
.d_one:					# 1.0
	.word	0x3ff00000,0x0
.d_half:				# 0.5
	.word	0x3fe00000,0x0
.p:
	.word	0x3f008b44,0x2ae6921e	# 0.31555192765684646356e-4
	.word	0x3f7f074b,0xf22a12a6	# 0.75753180159422776666e-2
	.word	0x3fd00000,0x0		# 0.25
.q:
	.word	0x3ea93363,0xce50455	# 0.75104028399870046114e-6
	.word	0x3f44af0c,0x5c28d4df	# 0.63121894374398503557e-3
	.word	0x3fad1728,0x51dfd9ff	# 0.56817302698551221787e-1
	.word	0x3fe00000,0x0		# 0.5
.X_EPS:
	.word	0x3e46a09e,0x667f3bcc
.LN_MINDOUBLE:
	.word	0xc0874385,0x446d71c3
.MINDOUBLE:
	.word	0x0,0x1
.LN_MAXDOUBLE:
	.word	0x40862e42,0xfefa39ef
.MAXDOUBLE:
	.word	0x7fefffff,0xffffffff
.HUGE:
	.word	0x47efffff,0xe0000000
.HUGE_VAL:
	.word	0x7ff00000,0x0
.M_LOG2E:
	.word	0x3ff71547,0x652b82fe	# 1.4426950408889634074
.M_LN2:
	.word	0x3fe62e42,0xfefa39ef	# 0.69314718055994530942
.C1:
	.word	0x3fe63000,0x0		# 0.693359375
.C2:
	.word	0xbff2,0xde8082e3,0x08654362 # -2.1219444005469058277e-4

#----------------------------------------------------------------------------
	.text
	.align	4
	.globl	exp
#------------------------------------------------------------------------------
				# struct exception {
	.set	type,0		# 	int type;
	.set	name,4		# 	char *name;
	.set	arg1,8		# 	double arg1;
	.set	arg2,16		# 	double arg2;
	.set	retval,24	# 	double retval;
				# };

	.set	dtow,32		# memory for double to integer conversion

	.set	.R1,1		# register int n
	.set	.F1,36
#----------------------------------------------------------------------------
exp:
	save	&.R1
	addw2	&.F1,%sp
	MCOUNT

	mmovdd	0(%ap),%d2		# register double x
	mfabsd2	%d2,%d1			# if ( x < 0 )	x = -x
	mfcmptd	%d1,.X_EPS		# if ( |x| < X_EPS )
	jge	.L52
	mfaddd3	%d2,.d_one,retval(%fp) # return (1.0 + x)
	movw	retval(%fp),%r0
	movw	retval+4(%fp),%r1
	ret	&.R1
.L52:
	mfcmptd	%d2,.LN_MINDOUBLE	# if ( x <= LN_MINDOUBLE)
	jg	.L56
	jne	.L58			#     if (x == LN_MINDOUBLE)
	movw	.MINDOUBLE+4,%r1
	movw	.MINDOUBLE,%r0		#	return (MINDOUBLE)
	ret	&.R1
.L58:
	movw	&4,type(%fp)		#     exc.type = UNDERFLOW
	mmovdd	.d_zero,retval(%fp)	#     exc.retval = 0.0
	jmp	..0
.L56:
	mfcmptd	%d2,.LN_MAXDOUBLE	# if (x >= LN_MAXDOUBLE)
	jl	.L63
	jne	.L65			#    if (x == LN_MAXDOUBLE)
	movw	.MAXDOUBLE+4,%r1
	movw	.MAXDOUBLE,%r0		#		return (MAXDOUBLE)
	ret	&.R1
.L65:
	movw	&3,type(%fp)		# exc.type = OVERFLOW
	cmpw	_lib_version,&0		# if (_lib_version == c_issue_4)
	jne	.hval
	mmovdd	.HUGE,retval(%fp)	# exc.retval = HUGE
	jmp	..0
.hval:
	mmovdd	.HUGE_VAL,retval(%fp)	# else exc.retval = HUGE_VAL
..0:
	mmovdd	%d2,arg1(%fp)		# exc.arg1 = x
	movw	&.exp,name(%fp)		# exc.name = "exp"
	pushaw	type(%fp)		# (&exc)
	cmpw	_lib_version,&2		# if (_lib_version==strict_ansi)
	jne	.nstrict
	movw	&34,errno		#	errno = ERANGE
	jmp	.L69
.nstrict:
	call	&1,matherr
	cmpw	%r0,&0			# if (!matherr(&exc))
	jne	.L69
	movw	&34,errno		#	errno = ERANGE
.L69:
	movw	retval(%fp),%r0		# return (exc.retval)
	movw	retval+4(%fp),%r1
	ret	&.R1
.L63:
	mfmuld3	.M_LOG2E,%x2,%x0	# x * M_LOG2E
					# assign y in %d1
	mfrndd2	%x0,%x1			# y = (double)(long)(x * M_LOG2E + 0.5)
	mmovdw	%d1,dtow(%fp)
	movw	dtow(%fp),%r8		# n = (int)(x * M_LOG2E + 0.5)
	mfmuld3	.C1,%x1,%x0		# y * C1
	mfsubd2 %x0,%x2			# x - y * C1
	mfmulx2	.C2,%x1			# y * C2
	mfsubd2	%x1,%x2			# x = (x -y * C1) - y *C2

	mfmuld3	%x2,%x2,%x1		# y = x*x
	mfmuld3	%x1,.p,%x0		# p[0]*y
	mfaddd2	.p+8,%x0		# + p[1]
	mfmuld2	%x1,%x0			# *y
	mfaddd2	.p+16,%x0		# +p[2]
	mfmuld2	%x0,%x2			# x *= ..
	mfmuld3	%x1,.q,%x0		# q[0] * y
	mfaddd2	.q+8,%x0		# + q[1]
	mfmuld2	%x1,%x0			# *y
	mfaddd2	.q+16,%x0		# + q[2]
	mfmuld2	%x1,%x0			# * y
	mfaddd2	.q+24,%x0		# + q[3]
	mfsubd2	%x2,%x0			# - x
	mfdivd2	%x0,%x2			# x/(_POLY3(y, q) - x)
	mfaddd2	.d_half,%x2		# 0.5 + ..

	mmovxx	%x2,0(%fp)		#move to temporary memory location
	addw2	&1,%r8			# n + 1
	addw2	%r8,0(%fp)		# adjust exponent
	mmovxx	0(%fp),%x2		#move back to MAU register
	mmovxd	%x2,0(%ap)		#convert to double

	movw	4(%ap),%r1		# return in %r0,%r1
	movw	0(%ap),%r0
	ret	&.R1


#----------------------------------------------------------------------------

