	.file	"log.s"
	.ident	"@(#)libm:m32mau/log.s	1.7"
########################################################################
#	double log(x)
#	double x;
#	Coefficients from Cody and Waite (1980).
########################################################################
	.data
	.align	4
.log:					# "log"
	.byte	108,111,103,0

	.align	4
.p:
	.word	0xbfe94415,0xb356bd29	# -0.78956112887491257267e0
	.word	0x4030624a,0x2016afed	#  0.16383943563021534222e2
	.word	0xc05007ff,0x12b3b59a	# -0.64124943423745581147e2
.q:
	.word	0x3ff00000,0x0		# 1.0
	.word	0xc041d580,0x4b67ce0f	# -0.35667977739034646171e2
	.word	0x40738083,0xfa15267e	#  0.31203222091924532844e3
	.word	0xc0880bfe,0x9c0d9077	# -0.76949932108494879777e3
.d_zero:
	.word	0x0,0x0
.d_one:
	.word	0x3ff00000,0x0
.M_SQRT1_2:
	.word	0x3fe6a09e,0x667f3bcd	#  0.70710678118654752440
.d_half:
	.word	0x3fe00000,0x0		# 0.5

.C1:		# define C1	0.693359375
	.word	0x3fe63000,0x0

.C2:		# define C2	-2.121944400546905827679e-4
	.word	0xbf2bd010,0x5c610ca8
.M_LOG10E:
	.word	0x3ffd,0xde5bd8a9,0x37287195	# in double extended

	.set	dexp_offset,20		# offset for extracting exponent
	.set	dexp_width,11		# width of double exponent
########################################################################
	.text
	.align	4
	.globl	log

	# the auto variables n and ret_val are allocated on stack
	.set	n,0			# integer n
	.set	ret_val,4		# double ret_val
	.set	local_var,12		# total size of locals = 4 + 8
#-----------------------------------------------------------------------
log:
	save	&1
	MCOUNT
	addw2	&local_var,%sp		# n and ret_val are allocated on stack
	movw	&0,%r8			# indicates log
	mmovdd	0(%ap),%d2		# register double x (in %d2)
	jg	.common			# if ( x <= 0 )
	pushw	0(%ap)
	pushw	4(%ap)
	pushw	&.log
	pushw	&3
	call	&4,log_error		#	log_error(x,"log",3)
	ret	&1			#	return (..)

#-----------------------------------------------------------------
	.text
	.align	4

#-----------------------------------------------------------------
.common:
	mmovdd	.d_one,%d1		# y = 1.0;

					# inline expansion of frexp
	extzv	&dexp_offset,&dexp_width,0(%ap),%r0	# value.parts.exp
	jg	.inline
	pushw	0(%ap)			# if (exp==0) use frexp instead
	pushw	4(%ap)			# which deals with denormal numbers
	pushaw	n(%fp)
	call	&3,frexp		# x = frexp(x, &n);
	movw	%r0,0(%ap)
	movw	%r1,4(%ap)
	jmp	.end_inline

	.align	4
.inline:
	subw3	&1022,%r0,n(%fp)			# n = exponent - 1022;
	insv	&1022,&dexp_offset,&dexp_width,0(%ap)	# exponent = 1022;
.end_inline:
	mmovdd	0(%ap),%d2		# x = value;

	mfcmptd	%d2,.M_SQRT1_2		# if (x < M_SQRT1_2)
	jge	.L55
	subw2	&1,n(%fp)		#	n--
	mmovdd	.d_half,%d1		#	y = 0.5
.L55:
	mfsubd3	%x1,%x2,%x0
	mfaddd3	%x2,%x1,%x3
	mfdivd3	%x3,%x0,%x2		# x = (x-y) / (x+y)
	mfaddd2	%x2,%x2			# x = x + x
	mfmuld3	%x2,%x2,%x1		# y = x * x
	mfmuld3	%x1,%x2,%x0		# .. x * y
	mfmuld3	%x1,.p,%x3		#	p[0]*y
	mfaddd2	.p+8,%x3		#	+ p[1]
	mfmuld2	%x1,%x3			#	* y
	mfaddd2	.p+16,%x3		#	+ p[2]
	mfmuld2	%x3,%x0			# x * y * _POLY2(y,p)
	mfmuld3	%x1,.q,%x3		#	q[0]*y
	mfaddd2	.q+8,%x3		#	+ q[1]
	mfmuld2	%x1,%x3			#	* y
	mfaddd2	.q+16,%x3		#	+ q[2]
	mfmuld2	%x1,%x3			#	* y
	mfaddd2	.q+24,%x3		#	+ y[3]
	mfdivd2	%x3,%x0			# x * y *_POLY2(y,p) / _POLY3(y,q)
	mfaddd2	%x0,%x2			# x += ...
	mmovwd	n(%fp),%d1		# y = (double)n
	mfmuld3	.C2,%x1,%x0		# y * C2
	mfaddd2	%x0,%x2			# x += y * C2
	mfmuld2	.C1,%x1			# y * C1
	mfaddd2	%x2,%x1			# x + y * C1
	TSTW	%r8			# do we want log or log10?
	jz	.no10
	mfmulx2	.M_LOG10E,%x1		# if log10, mult by log10(e)
.no10:
	mmovdd	%d1,ret_val(%fp)
	movw	ret_val+4(%fp),%r1
	movw	ret_val(%fp),%r0
	ret	&1

########################################################################

	.text
	.set	local_var2,12		# total size of locals = 4 + 8
	.align	4
	.globl	log10
log10:
	save	&1
	MCOUNT
	addw2	&local_var2,%sp
	movw	&1,%r8		# indicates log10
	mmovdd	0(%ap),%d2
	jg	.common
	pushw	0(%ap)
	pushw	4(%ap)
	pushw	&.log10
	pushw	&5
	call	&4,log_error		# log_error(x,"log10",5)
	ret	&1
#############################################################################
	.data
	.align	4
.MHUGE:
	.word	0xc7efffff,0xe0000000
.MHUGE_VAL:
	.word	0xfff00000,0x0
	.text
	.set	.F3,32

	.align	4
	.set	.R3,1
log_error:
	save	&.R3
	addw2	&.F3,%sp
	movw	&0,%r8		# zflag = 0
	mfcmpd	0(%ap),&0.0	# if (!z)
	jne	.L100
	movw	&1,%r8		# zflag = 1
.L100:
	movw	8(%ap),4(%fp)	# exc.name = f_name
	cmpw	_lib_version,&0	# if c_issue_4
	jne	.L101
	movw	.MHUGE,24(%fp)	# exc.retval = -HUGE
	movw	.MHUGE+4,28(%fp)
	jmp	.L103
.L101:
	movw	.MHUGE_VAL,24(%fp)	# else exc.retval = -HUGE_VAL
	movw	.MHUGE_VAL+4,28(%fp)
.L103:
	movw	0(%ap),8(%fp)	# exc.arg1 = x
	movw	4(%ap),12(%fp)
	cmpw	_lib_version,&2	# if strict_ansi
	jne	.L104
	cmpw	%r8,&0		# if (!zflag)
	jne	.L105
	jmp	.L112
..0:
	cmpw	_lib_version,&0	# if c_issue_4
	jne	.L112
	pushw	&2		# write(2,f_name,namelength)
	pushw	8(%ap)
	pushw	12(%ap)
	call	&3,_write
	cmpw	%r8,&0		# if (!zflag)
	je	.nozero
	pushw	&2		# write(2,": SING error\n",13);
	pushw	&.X113
	pushw	&13
	call	&3,_write
	jmp	.L112
.nozero:
	pushw	&2		# write(2,": DOMAIN error\n",15);
	pushw	&.X114
	pushw	&15
	call	&3,_write
.L112:
	movw	&33,errno	# errno = EDOM
.L107:
	movw	28(%fp),%r1
	movw	24(%fp),%r0
	ret	&.R3
.L105:
	movw	&34,errno	# errno = ERANGE
	jmp	.L107
.L104:
	cmpw	%r8,&0		# if (!zflag)
	je	.nozero2
	movw	&2,0(%fp)	# exc.type = SING
	jmp	.nz3
.nozero2:
	movw	&1,0(%fp)	# exc.type = DOMAIN
.nz3:
	movaw	0(%fp),%r0
	pushw	%r0
	call	&1,matherr	# if (!matherr(&exc))
	cmpw	%r0,&0
	jne	.L107
	jmp	..0
	.data
.X114:
	.byte	58,32,68,79,77,65,73,78,32,101
	.byte	114,114,111,114,10,0
.X113:
	.byte	58,32,83,73,78,71,32,101
	.byte	114,114,111,114,10,0
.log10:
	.byte	108,111,103,49,48,0
