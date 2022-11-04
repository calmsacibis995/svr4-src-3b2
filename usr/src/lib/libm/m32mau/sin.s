	.file	"sin.s"
	.ident	"@(#)libm:m32mau/sin.s	1.4"
###################################################
#	double sin(x)
#	double cos(x)
#	double tan(x)
#	double x;
# sin, cos and tan routines - algorithm from paper by Peter Teng, UCB
# argument is expressed as (n * pi/2 + f ), where
# -pi/4 <= f <= pi/4 and f is in extended precision
# use table look-up for values less than 64*pi/2;
# Payne and Hanek range reduction for greater values
# sin(x) = _sin(f)* -1**(s), for n even
#      and _cos(f)* -1**(s), for n odd (s == floor(n/2))
# for cos(x), simply increment n by 1 after range reduction
# tanx(x) = _sin(f)/_cos(f), for n even
#      and -_cos(f)/_sin(f), for n odd
# If 32206 is attached, machine sin and cos are used.
#-----------------------------------------------------
# multiples of pi/2 expressed as sum of 2 extenededs:
# leading, trailing
# extended precision multiples of pi/2 (168 bits) were generated
# using bc and dc
# leading = (double)(n *pi/2 in extended)
# trailing = (double)(n * pi/2 - leading in extended)
#######################################################
	.data
	.align	4
leading:			# n *pi/2 in double extended
	.word	0x0,0x0,0x0			# 0 *pi/2
	.word	0x3fff,0xc90fdaa2,0x2168c234	# 1 *pi/2
	.word	0x4000,0xc90fdaa2,0x2168c234	# 2 *pi/2
	.word	0x4001,0x96cbe3f9,0x990e91a7	# 3 *pi/2
	.word	0x4001,0xc90fdaa2,0x2168c234	# 4 *pi/2
	.word	0x4001,0xfb53d14a,0xa9c2f2c1	# 5 *pi/2
	.word	0x4002,0x96cbe3f9,0x990e91a7	# 6 *pi/2
	.word	0x4002,0xafeddf4d,0xdd3ba9ee	# 7 *pi/2
	.word	0x4002,0xc90fdaa2,0x2168c234	# 8 *pi/2
	.word	0x4002,0xe231d5f6,0x6595da7b	# 9 *pi/2
	.word	0x4002,0xfb53d14a,0xa9c2f2c1	# 10 *pi/2
	.word	0x4003,0x8a3ae64f,0x76f80584	# 11 *pi/2
	.word	0x4003,0x96cbe3f9,0x990e91a7	# 12 *pi/2
	.word	0x4003,0xa35ce1a3,0xbb251dca	# 13 *pi/2
	.word	0x4003,0xafeddf4d,0xdd3ba9ee	# 14 *pi/2
	.word	0x4003,0xbc7edcf7,0xff523611	# 15 *pi/2
	.word	0x4003,0xc90fdaa2,0x2168c234	# 16 *pi/2
	.word	0x4003,0xd5a0d84c,0x437f4e58	# 17 *pi/2
	.word	0x4003,0xe231d5f6,0x6595da7b	# 18 *pi/2
	.word	0x4003,0xeec2d3a0,0x87ac669e	# 19 *pi/2
	.word	0x4003,0xfb53d14a,0xa9c2f2c1	# 20 *pi/2
	.word	0x4004,0x83f2677a,0x65ecbf72	# 21 *pi/2
	.word	0x4004,0x8a3ae64f,0x76f80584	# 22 *pi/2
	.word	0x4004,0x90836524,0x88034b95	# 23 *pi/2
	.word	0x4004,0x96cbe3f9,0x990e91a7	# 24 *pi/2
	.word	0x4004,0x9d1462ce,0xaa19d7b9	# 25 *pi/2
	.word	0x4004,0xa35ce1a3,0xbb251dca	# 26 *pi/2
	.word	0x4004,0xa9a56078,0xcc3063dc	# 27 *pi/2
	.word	0x4004,0xafeddf4d,0xdd3ba9ee	# 28 *pi/2
	.word	0x4004,0xb6365e22,0xee46efff	# 29 *pi/2
	.word	0x4004,0xbc7edcf7,0xff523611	# 30 *pi/2
	.word	0x4004,0xc2c75bcd,0x105d7c23	# 31 *pi/2
	.word	0x4004,0xc90fdaa2,0x2168c234	# 32 *pi/2
	.word	0x4004,0xcf585977,0x32740846	# 33 *pi/2
	.word	0x4004,0xd5a0d84c,0x437f4e58	# 34 *pi/2
	.word	0x4004,0xdbe95721,0x548a9469	# 35 *pi/2
	.word	0x4004,0xe231d5f6,0x6595da7b	# 36 *pi/2
	.word	0x4004,0xe87a54cb,0x76a1208d	# 37 *pi/2
	.word	0x4004,0xeec2d3a0,0x87ac669e	# 38 *pi/2
	.word	0x4004,0xf50b5275,0x98b7acb0	# 39 *pi/2
	.word	0x4004,0xfb53d14a,0xa9c2f2c1	# 40 *pi/2
	.word	0x4005,0x80ce280f,0xdd671c69	# 41 *pi/2
	.word	0x4005,0x83f2677a,0x65ecbf72	# 42 *pi/2
	.word	0x4005,0x8716a6e4,0xee72627b	# 43 *pi/2
	.word	0x4005,0x8a3ae64f,0x76f80584	# 44 *pi/2
	.word	0x4005,0x8d5f25b9,0xff7da88d	# 45 *pi/2
	.word	0x4005,0x90836524,0x88034b95	# 46 *pi/2
	.word	0x4005,0x93a7a48f,0x1088ee9e	# 47 *pi/2
	.word	0x4005,0x96cbe3f9,0x990e91a7	# 48 *pi/2
	.word	0x4005,0x99f02364,0x219434b0	# 49 *pi/2
	.word	0x4005,0x9d1462ce,0xaa19d7b9	# 50 *pi/2
	.word	0x4005,0xa038a239,0x329f7ac2	# 51 *pi/2
	.word	0x4005,0xa35ce1a3,0xbb251dca	# 52 *pi/2
	.word	0x4005,0xa681210e,0x43aac0d3	# 53 *pi/2
	.word	0x4005,0xa9a56078,0xcc3063dc	# 54 *pi/2
	.word	0x4005,0xacc99fe3,0x54b606e5	# 55 *pi/2
	.word	0x4005,0xafeddf4d,0xdd3ba9ee	# 56 *pi/2
	.word	0x4005,0xb3121eb8,0x65c14cf6	# 57 *pi/2
	.word	0x4005,0xb6365e22,0xee46efff	# 58 *pi/2
	.word	0x4005,0xb95a9d8d,0x76cc9308	# 59 *pi/2
	.word	0x4005,0xbc7edcf7,0xff523611	# 60 *pi/2
	.word	0x4005,0xbfa31c62,0x87d7d91a	# 61 *pi/2
	.word	0x4005,0xc2c75bcd,0x105d7c23	# 62 *pi/2
	.word	0x4005,0xc5eb9b37,0x98e31f2b	# 63 *pi/2
	.word	0x4005,0xc90fdaa2,0x2168c234	# 64 *pi/2
trailing:			# extra bits for n*pi/2
	.word	0x0,0x0,0x0
	.word	0x3fbf,0xC4C6628B,0x80DC1CD1
	.word	0x3fc0,0xC4C6628B,0x80DC1CD1
	.word	0x3fc1,0x9394C9E8,0xA0A5159C
	.word	0x3fc1,0xC4C6628B,0x80DC1CD1
	.word	0x3fc1,0xF5F7FB2E,0x61132405
	.word	0x3fc2,0x9394C9E8,0xA0A5159C
	.word	0x3fc2,0x2C2D963A,0x10C09937
	.word	0x3fc2,0xC4C6628B,0x80DC1CD1
	.word	0x3fc2,0x5D5F2EDC,0xF0F7A06B
	.word	0x3fc2,0xF5F7FB2E,0x61132405
	.word	0x3fc3,0x474863BF,0xE89753CF
	.word	0x3fc3,0x9394C9E8,0xA0A5159C
	.word	0x3fc3,0xDFE13011,0x58B2D769
	.word	0x3fc3,0x2C2D963A,0x10C09937
	.word	0x3fc3,0x7879FC62,0xC8CE5B04
	.word	0x3fc3,0xC4C6628B,0x80DC1CD1
	.word	0x3fc3,0x1112C8B4,0x38E9DE9E
	.word	0x3fc3,0x5D5F2EDC,0xF0F7A06B
	.word	0x3fc3,0xA9AB9505,0xA9056238
	.word	0x3fc3,0xF5F7FB2E,0x61132405
	.word	0x3fc4,0xA12230AB,0x8C9072E9
	.word	0x3fc4,0x474863BF,0xE89753CF
	.word	0x3fc4,0xED6E96D4,0x449E34B6
	.word	0x3fc4,0x9394C9E8,0xA0A5159C
	.word	0x3fc4,0x39BAFCFC,0xFCABF683
	.word	0x3fc4,0xDFE13011,0x58B2D769
	.word	0x3fc4,0x86076325,0xB4B9B850
	.word	0x3fc4,0x2C2D963A,0x10C09937
	.word	0x3fc4,0xD253C94E,0x6CC77A1D
	.word	0x3fc4,0x7879FC62,0xC8CE5B04
	.word	0x3fc4,0x1EA02F77,0x24D53BEA
	.word	0x3fc4,0xC4C6628B,0x80DC1CD1
	.word	0x3fc4,0x6AEC959F,0xDCE2FDB7
	.word	0x3fc4,0x1112C8B4,0x38E9DE9E
	.word	0x3fc4,0xB738FBC8,0x94F0BF84
	.word	0x3fc4,0x5D5F2EDC,0xF0F7A06B
	.word	0x3fc4,0x038561F1,0x4CFE8151
	.word	0x3fc4,0xA9AB9505,0xA9056238
	.word	0x3fc4,0x4FD1C81A,0x050C431E
	.word	0x3fc4,0xF5F7FB2E,0x61132405
	.word	0x3fc5,0xCE0F1721,0x5E8D0275
	.word	0x3fc5,0xA12230AB,0x8C9072E9
	.word	0x3fc5,0x74354A35,0xBA93E35C
	.word	0x3fc5,0x474863BF,0xE89753CF
	.word	0x3fc5,0x1A5B7D4A,0x169AC443
	.word	0x3fc5,0xED6E96D4,0x449E34B6
	.word	0x3fc5,0xC081B05E,0x72A1A529
	.word	0x3fc5,0x9394C9E8,0xA0A5159C
	.word	0x3fc5,0x66A7E372,0xCEA88610
	.word	0x3fc5,0x39BAFCFC,0xFCABF683
	.word	0x3fc5,0x0CCE1687,0x2AAF66F6
	.word	0x3fc5,0xDFE13011,0x58B2D769
	.word	0x3fc5,0xB2F4499B,0x86B647DD
	.word	0x3fc5,0x86076325,0xB4B9B850
	.word	0x3fc5,0x591A7CAF,0xE2BD28C3
	.word	0x3fc5,0x2C2D963A,0x10C09937
	.word	0x3fc5,0xFF40AFC4,0x3EC409AA
	.word	0x3fc5,0xD253C94E,0x6CC77A1D
	.word	0x3fc5,0xA566E2D8,0x9ACAEA90
	.word	0x3fc5,0x7879FC62,0xC8CE5B04
	.word	0x3fc5,0x4B8D15EC,0xF6D1CB77
	.word	0x3fc5,0x1EA02F77,0x24D53BEA
	.word	0x3fc5,0xF1B34901,0x52D8AC5D
	.word	0x3fc5,0xC4C6628B,0x80DC1CD1
.MAXLOOKUP:
	.word	0x4005,0xc90fdaa2,0x2168c235	# 64 *pi/2
.twoopi:
	.word	0x3ffe,0xa2f9836e,0x4e44152a	# 2/pi in extended
.x_half:
	.word	0x3ffe,0x80000000,0x0	# 0.5
.d_one:
	.word	0x3ff00000,0x0		# 1.0
.M_PI_2:				
	.word	0x3ff921fb,0x54442d18
#-----------------------------------------------------------------
	.text
	.align	4
	.set	.F1,28
	.globl	sin
#-----------------------------------------------------------------
sin:
	save	&3		# we use %r6, %r7, %r8
	MCOUNT
	addw2	&.F1,%sp
	mfabsd2	0(%ap),%f0	# y = |x|
	cmpw	_fp_hw,&2	# if 32206 atached
	jne	.ssoft
	mfcmptd	%f0,.M_PI_2	# if (|x| < Pi/2) use machine instr.
	jg	.ssoft
	mfsind2	%f0,0(%fp)
	movw	0(%fp),%r0
	movw	4(%fp),%r1
	bitw	&0x80000000,0(%ap) # if (x < 0.0)
	jz	.sret
	xorw2	&0x80000000,%r0	  # return -sin
.sret:
	ret	&3
.ssoft:
	movw	&0,%r6		# indicates sin
	mmovdx	%d0,%x1		# y = (long double)y
	
#-----------------------------------------------------------------
	.text
	.align	4

#-----------------------------------------------------------------
.trig:
	mfcmptx %x1,.MAXLOOKUP	# if (y > MAXLOOKUP)
	jle	.lookup		# use Payne-Hanek reduction
	addw2	&8,%sp
	mmovxd	%f1,-8(%sp)
	pushw	&1		# [-pi/4,pi/4] reduction
	pushaw	12(%fp)		# &n
	pushaw	0(%fp)		# &q
	call	&5,_reduce
	movw	12(%fp),%r7	# n == quadrant
	mmovxx	0(%fp),%x1	# q
	jmp	.cmpx
.lookup:
	mfmulx3	.twoopi,%x1,%x2	# tmp = y * 2/pi
	mmovfa	0(%fp)		# the following sets the rounding
				# mode to round toward nearest
				# before converting the double to int,
				# then restores the rounding mode
	bitb	&0xc0,1(%fp)
	jz	.round		# already round to zero
	andb3	&0xc0,1(%fp),4(%fp)	# save old rounding mode
	andb2	&0x3f,1(%fp)	# set round to nearest
	orb2	&0x02,1(%fp)	# set CSC
	mmovta	0(%fp)
	mmovxw	%f2,12(%fp)	# n = (int) tmp - n is quadrant
	movw	12(%fp),%r7
	mmovfa	0(%fp)
	andb2	&0x3f,1(%fp)
	orb2	4(%fp),1(%fp)	# restore old rounding
	orb2	&0x2,1(%fp)	# set CSC
	mmovta	0(%fp)
	jmp	.sub
.round:
	mmovxw	%f2,12(%fp)	# n = (int) tmp - n is quadrant
	movw	12(%fp),%r7
.sub:
	mulw3	&12,%r7,%r0
	mfsubx2	leading(%r0),%x1	# y = y- leading[n]
	mfsubx2	trailing(%r0),%x1	# y = y - trailing[n]
.cmpx:
	bitw	&0x80000000,0(%ap)	# if (x < 0.0)
	jz	.pos
	mfnegx2	%x1,%x1		# y = -y
	subw3	%r7,&4,%r7	# n = 4 -n
.pos:
	cmpw	_fp_hw,&2	# determine machine version
	jne	.MAU106		# if 206, use machine instructions
	cmpw	&2,%r6
	je	.dotan2		# sin or cos
	addw2	%r6,%r7		# if (cosflag) n+= 1
	lrsw3	&1,%r7,%r8 	# register int sign = n/2
	bitw	&1,%r7		# if (n % 2)
	jz	.dosin2
	mfcosx2	%x1,%x2
	jmp	.comp2
.dosin2:
	mfsinx2	%x1,%x2
.comp2:
	mmovxd	%x2,8(%fp)
	movw	8(%fp),%r0
	movw	12(%fp),%r1
	bitw	&1,%r8		# if (sign % 2)
	jz	.return2
	xorw2	&0x80000000,%r0	# return - y
.return2:
	ret	&3
.dotan2:				# tan
	mfcosx2	%x1,%x2
	mfsinx2	%x1,%x0
	bitw	&1,%r7		# if (n % 2)
	jz	.even2
	mfdivx2	%x0,%x2		# cos/sin
	mmovxd	%x2,0(%fp)
	movw	0(%fp),%r0
	movw	4(%fp),%r1
	xorw2	&0x80000000,%r0	# return -cos/sin
	ret	&3
.even2:
	mfdivx2	%x2,%x0		# sin/cos
	mmovxd	%x0,8(%fp)
	movw	8(%fp),%r0
	movw	12(%fp),%r1
	ret	&3
.MAU106:
	cmpw	&2,%r6
	je	.dotan		# sin or cos
	addw2	%r6,%r7		# if (cosflag) n+= 1
	lrsw3	&1,%r7,%r8 	# register int sign = n/2
	addw2	&12,%sp
	mmovxx	%x1,-12(%sp)
	bitw	&1,%r7		# if (n % 2)
	jz	.dosin
	call	&3,_cos
	jmp	.comp
.dosin:
	call	&3,_sin
.comp:
	bitw	&1,%r8		# if (sign % 2)
	jz	.return
	xorw2	&0x80000000,%r0	# return - y
.return:
	ret	&3
.dotan:				# tan
	mmovxx	%x1,16(%fp)	# save y
	pushw	16(%fp)
	pushw	20(%fp)
	pushw	24(%fp)
	call	&3,_cos
	movw	%r0,0(%fp)	# tmp =_cos(y)
	movw	%r1,4(%fp)
	pushw	16(%fp)
	pushw	20(%fp)
	pushw	24(%fp)
	call	&3,_sin
	movw	%r0,8(%fp)	# y =_sin(y)
	movw	%r1,12(%fp)
	bitw	&1,%r7		# if (n % 2)
	jz	.even
	mfdivd2	8(%fp),0(%fp)	# cos/sin
	movw	0(%fp),%r0
	movw	4(%fp),%r1
	xorw2	&0x80000000,%r0	# return -cos/sin
	ret	&3
.even:
	mfdivd2	0(%fp),8(%fp)	# sin/cos
	movw	8(%fp),%r0
	movw	12(%fp),%r1
	ret	&3
#-----------------------------------------------------------------

	.text
	.align	4
	.set	.F2,28
	.globl	cos
#-----------------------------------------------------------------
cos:
	save	&3
	MCOUNT
	addw2	&.F2,%sp
	mfabsd2 0(%ap),%f0	# y =  |x|
	cmpw	_fp_hw,&2	# if 32206 atached
	jne	.csoft
	mfcmptd	%f0,.M_PI_2	# if (|x| < Pi/2) use machine instr.
	jg	.csoft
	mfcosd2	%f0,0(%fp)
	movw	0(%fp),%r0
	movw	4(%fp),%r1
	ret	&3
.csoft:
	movw	&1,%r6		# indicates cos
	mmovdx	%d0,%x1		# y = (long double)y
	jmp	.trig
#-----------------------------------------------------------------
	.text
	.align	4
	.set	.F3,28
	.globl	tan
#-----------------------------------------------------------------
tan:
	save	&3		# we use %r6, %r7, %r8
	MCOUNT
	addw2	&.F3,%sp
	mfabsd2 0(%ap),%f0		# y =  |x|
	cmpw	_fp_hw,&2	# if 32206 atached
	jne	.tsoft
	mfcmptd	%f0,.M_PI_2	# if (|x| < Pi/2) use machine instr.
	jg	.tsoft
	mfsind2	%f0,%f2
	mfcosd2	%f0,%f1
	mfdivd2	%f1,%f2		# tan = sin/cos
	mmovdd	%f2,0(%fp)
	movw	0(%fp),%r0
	movw	4(%fp),%r1
	bitw	&0x80000000,0(%ap) # if (x < 0.0)
	jz	.tret
	xorw2	&0x80000000,%r0	  # return -sin
.tret:
	ret	&3
.tsoft:
	movw	&2,%r6		# indicates tan
	mmovdx	%d0,%x1		# y = (long double)y
	jmp	.trig
##################################################################
#	double _sin(x)
#	double _cos(x)
#	long double x;
#	-pi/4 <= x <= pi/4
#	Algorithm and coefficients from paper by Peter Teng
####################################################################
	.data
	.align	4
.p:

	.word	0x3de5de23,0xad2495f2
	.word	0xbe5ae5f9,0x33569b98
	.word	0x3ec71de3,0x7262ecf8
	.word	0xbf2a01a0,0x19e4a9ac
	.word	0x3f811111,0x11110d97
	.word	0xbfc55555,0x5555555a
	.align	4
.q:
	.word	0xbda8fb12,0xbaf59d4b
	.word	0x3e21ee9f,0x14cdc590
	.word	0xbe927e4f,0x812b495b
	.word	0x3efa01a0,0x19cbf671
	.word	0xbf56c16c,0x16c1521f
	.word	0x3fa55555,0x5555554c
	.align	4
.x_one:
	.word	0x3fff,0x80000000,0x0
	.align	4
.x_two:
	.word	0x4000,0x80000000,0x0
#-------------------------------------------------------------------
	.text
	.align	4
#--------------------------------------------------------------------
_sin:	
	save	&0
	MCOUNT
	addw2	&8,%sp
	mmovxx	0(%ap),%x2	# x in double extended
	mfmulx3	%x2,%x2,%x1	# xsq (x * x)
	mmovxd	%x1,%d1		# Xsq (xsq rounded to double)
	mfmuld3	%d1,.p,%d0	# Xsq * p[0]
	mfaddd2	.p+8,%d0	#     + p[1]
	mfmuld2 %d1,%d0		#     * Xsq
	mfaddd2	.p+16,%d0	#     + p[2]
	mfmuld2 %d1,%d0		#     * Xsq
	mfaddd2	.p+24,%d0	#     + p[3]
	mfmuld2 %d1,%d0		#     * Xsq
	mfaddd2	.p+32,%d0	#     + p[4]
	mfmuld2 %d1,%d0		#     * Xsq
	mfaddd2	.p+40,%d0	#     + p[5]
	mfmuld2 %d1,%d0		# Q(x) = Xsq * _POLY5(xsq, p)
	mmovdx	%d0,%x0		# Q(x) in extended
	mfmulx2 %x2,%x0		# x * Q(x)
	mfaddx2 %x2,%x0		# + x
	mmovxd	%x0,0(%fp)	# return round(x + Q(x) * x)
	movw	0(%fp),%r0
	movw	4(%fp),%r1
	ret	&0

#--------------------------------------------------------------------
	.text
	.align	4
#--------------------------------------------------------------------
_cos:	
	save	&0
	MCOUNT
	addw2	&8,%sp
	mmovxx	0(%ap),%x2	#x in double extended
	mfmulx2	%x2,%x2		# x * x in extended
	mmovxd	%x2,%d1		# Xsq rounded to double
	mfmuld3	%d1,.q,%d0	# Xsq * q[0]
	mfaddd2	.q+8,%d0	#     + q[1]
	mfmuld2 %d1,%d0		#     * Xsq
	mfaddd2	.q+16,%d0	#     + q[2]
	mfmuld2 %d1,%d0		#     * Xsq
	mfaddd2	.q+24,%d0	#     + q[3]
	mfmuld2 %d1,%d0		#     * Xsq
	mfaddd2	.q+32,%d0	#     + q[4]
	mfmuld2 %d1,%d0		#     * Xsq
	mfaddd2	.q+40,%d0	#     + q[5]
	mfmuld2 %d1,%d0		# Xsq * _POLY5(xsq, q)
	mfmuld2 %d1,%d0		# Q(x) = Xsq *Xsq * _POLY5(xsq, q)
	mmovdx	%d0,%x0		# Q(x) in extended
	mfdivx2 .x_two,%x2	# xsq /2.0 in extended
	mfsubx2 %x0,%x2		# xsq /2.0 - Q(x)
	mfsubx3	%x2,.x_one,%x0	# 1.0 - (xsq/2.0 - Q(x))
	mmovxd	%x0,0(%fp)	# return rounded to double
	movw	0(%fp),%r0
	movw	4(%fp),%r1
	ret	&0

	.text
