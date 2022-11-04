	.file	"sin.s"
	.ident	"@(#)libm:m32mau/sinf.s	1.5"
###################################################
#	float sinf(float x)
#	float cosf(float x)
#	float tanf(float x)
# Single precision sin, cos  and tan routines 
# - algorithm from paper by Peter Teng, UCB
# argument is expressed as (n * pi/2 + f ), where
# -pi/4 <= f <= pi/4 and f is in double precision
# use table look-up for values less than 64*pi/2;
# Payne and Hanek range reduction for greater values
# sinf(x) = _sin(f)* -1**(s), for n even
#      and _cos(f)* -1**(s), for n odd (s == floor(n/2))
# for cosf(x), simply increment n by 1 after range reduction
# tanf(x) = _sin(f)/_cos(f), for n even
#      and -_cos(f)/_sin(f), for n odd
# If 32206 is attached, machine sin and cos are used.
#-----------------------------------------------------
# multiples of pi/2 expressed as sum of 2 doubles:
# leading, trailing
# extended precision multiples of pi/2 (168 bits) were generated
# using bc and dc
# leading = (double)(n *pi/2 in extended)
# trailing = (double)(n * pi/2 - leading in extended)
#######################################################
	.data
	.align	4
trailing:
	.word	0x0,0x0
	.word	0x3c91a626,0x33145c07
	.word	0x3ca1a626,0x33145c07
	.word	0x3caa7939,0x4c9e8a0a
	.word	0x3cb1a626,0x33145c07
	.word	0x3cb60faf,0xbfd97309
	.word	0x3cba7939,0x4c9e8a0a
	.word	0x3cbee2c2,0xd963a10c
	.word	0x3cc1a626,0x33145c07
	.word	0x3cc3daea,0xf976e788
	.word	0x3cc60faf,0xbfd97309
	.word	0xbcd3ddc5,0xbce200bb
	.word	0x3cca7939,0x4c9e8a0a
	.word	0xbcd1a900,0xf67f753a
	.word	0x3ccee2c2,0xd963a10c
	.word	0xbccee878,0x6039d373
	.word	0x3cd1a626,0x33145c07
	.word	0xbcca7eee,0xd374bc71
	.word	0x3cd3daea,0xf976e788
	.word	0xbcc61565,0x46afa570
	.word	0x3cd60faf,0xbfd97309
	.word	0xbcc1abdb,0xb9ea8e6e
	.word	0xbce3ddc5,0xbce200bb
	.word	0x3cecaf6b,0x74b6a225
	.word	0x3cda7939,0x4c9e8a0a
	.word	0xbcb1b191,0x40c0c0d5
	.word	0xbce1a900,0xf67f753a
	.word	0x3ceee430,0x3b192da6
	.word	0x3cdee2c2,0xd963a10c
	.word	0xbc26d61b,0x58c99c43
	.word	0xbcdee878,0x6039d373
	.word	0xbceee70a,0xfe8446d9
	.word	0x3ce1a626,0x33145c07
	.word	0x3cb19abb,0x2567f739
	.word	0xbcda7eee,0xd374bc71
	.word	0xbcecb246,0x3821bb58
	.word	0x3ce3daea,0xf976e788
	.word	0x3cc1a070,0xac3e29a0
	.word	0xbcd61565,0x46afa570
	.word	0xbcea7d81,0x71bf2fd8
	.word	0x3ce60faf,0xbfd97309
	.word	0xbcfcb18f,0x8746f50c
	.word	0xbcd1abdb,0xb9ea8e6e
	.word	0x3cf3dba1,0xaa51add5
	.word	0xbcf3ddc5,0xbce200bb
	.word	0x3cd1a34b,0x6fa942d3
	.word	0x3cfcaf6b,0x74b6a225
	.word	0xbce613f7,0xe4fa18d6
	.word	0x3cea7939,0x4c9e8a0a
	.word	0xbcfa7cca,0xc0e4698b
	.word	0xbcc1b191,0x40c0c0d5
	.word	0x3cf61066,0x70b43955
	.word	0xbcf1a900,0xf67f753a
	.word	0x3cda765e,0x893370d7
	.word	0x3cfee430,0x3b192da6
	.word	0xbce1aa6e,0x583501d4
	.word	0x3ceee2c2,0xd963a10c
	.word	0xbcf84805,0xfa81de0a
	.word	0xbc36d61b,0x58c99c43
	.word	0x3cf8452b,0x3716c4d6
	.word	0xbceee878,0x6039d373
	.word	0x3ce1a4b8,0xd15ecf6d
	.word	0xbcfee70a,0xfe8446d9
	.word	0xbcda81c9,0x96dfd5a5
	.word	0x3cf1a626,0x33145c07
	.align	4
leading:
	.word	0x0,0x0
	.word	0x3ff921fb,0x54442d18
	.word	0x400921fb,0x54442d18
	.word	0x4012d97c,0x7f3321d2
	.word	0x401921fb,0x54442d18
	.word	0x401f6a7a,0x2955385e
	.word	0x4022d97c,0x7f3321d2
	.word	0x4025fdbb,0xe9bba775
	.word	0x402921fb,0x54442d18
	.word	0x402c463a,0xbeccb2bb
	.word	0x402f6a7a,0x2955385e
	.word	0x4031475c,0xc9eedf01
	.word	0x4032d97c,0x7f3321d2
	.word	0x40346b9c,0x347764a4
	.word	0x4035fdbb,0xe9bba775
	.word	0x40378fdb,0x9effea47
	.word	0x403921fb,0x54442d18
	.word	0x403ab41b,0x9886fea
	.word	0x403c463a,0xbeccb2bb
	.word	0x403dd85a,0x7410f58d
	.word	0x403f6a7a,0x2955385e
	.word	0x40407e4c,0xef4cbd98
	.word	0x4041475c,0xc9eedf01
	.word	0x4042106c,0xa4910069
	.word	0x4042d97c,0x7f3321d2
	.word	0x4043a28c,0x59d5433b
	.word	0x40446b9c,0x347764a4
	.word	0x404534ac,0xf19860c
	.word	0x4045fdbb,0xe9bba775
	.word	0x4046c6cb,0xc45dc8de
	.word	0x40478fdb,0x9effea47
	.word	0x404858eb,0x79a20bb0
	.word	0x404921fb,0x54442d18
	.word	0x4049eb0b,0x2ee64e81
	.word	0x404ab41b,0x9886fea
	.word	0x404b7d2a,0xe42a9153
	.word	0x404c463a,0xbeccb2bb
	.word	0x404d0f4a,0x996ed424
	.word	0x404dd85a,0x7410f58d
	.word	0x404ea16a,0x4eb316f6
	.word	0x404f6a7a,0x2955385e
	.word	0x405019c5,0x1fbace4
	.word	0x40507e4c,0xef4cbd98
	.word	0x4050e2d4,0xdc9dce4c
	.word	0x4051475c,0xc9eedf01
	.word	0x4051abe4,0xb73fefb5
	.word	0x4052106c,0xa4910069
	.word	0x405274f4,0x91e2111e
	.word	0x4052d97c,0x7f3321d2
	.word	0x40533e04,0x6c843287
	.word	0x4053a28c,0x59d5433b
	.word	0x40540714,0x472653ef
	.word	0x40546b9c,0x347764a4
	.word	0x4054d024,0x21c87558
	.word	0x405534ac,0xf19860c
	.word	0x40559933,0xfc6a96c1
	.word	0x4055fdbb,0xe9bba775
	.word	0x40566243,0xd70cb82a
	.word	0x4056c6cb,0xc45dc8de
	.word	0x40572b53,0xb1aed992
	.word	0x40578fdb,0x9effea47
	.word	0x4057f463,0x8c50fafb
	.word	0x405858eb,0x79a20bb0
	.word	0x4058bd73,0x66f31c64
	.word	0x405921fb,0x54442d18
	.align	4
.maxlookup:
	.word	0x405921fb,0x54442d18
.M2_PI:
	.word	0x3fe45f30,0x6dc9c883
.M_PI_2:
	.word	0x3fc90fdb
.f_one:
	.word	0x3f800000
#----------------------------------------------------------------
	.text
	.set	.F1,20
	.globl	sinf
	.align	4
	.set	.R1,3
#----------------------------------------------------------------
sinf:
	save	&.R1
	MCOUNT
	addw2	&.F1,%sp
	mfabss2 0(%ap),%f0	# y =  |x|
	cmpw	_fp_hw,&2	# if 32206 atached
	jne	.ssoft
	mfcmpts	%f0,.M_PI_2	# if (|x| < Pi/2) use machine instr.
	jg	.ssoft
	mfsins2	%f0,0(%fp)
	movw	0(%fp),%r0
	bitw	&0x80000000,0(%ap) # if (x < 0.0)
	jz	.sret
	xorw2	&0x80000000,%r0	  # return -sin
.sret:
	ret	&.R1
.ssoft:
	movw	&0,%r6		# indicates sin
	mmovsd	%s0,%d1 	# register double x
#-----------------------------------------------------------------
	.align	4
	.text
#------------------------------------------------------------------
.trig:
	mfcmptd	%f1,.maxlookup	# if (y <= maxlookup)
	jle	.L49
	addw2	&4,%sp
	mmovds	%f1,-4(%sp)
	pushw	&1		# [-pi/4,pi/4] reduction
	pushaw	16(%fp)
	pushaw	0(%fp)		# q
	call	&4,_reducef
	movw	16(%fp),%r8	# n == quadrant
	mmovdd	0(%fp),%f1	#q
	jmp	.L51
.L49:
	mfmuld3	.M2_PI,%f1,%f0	# tmp = y * 2/pi
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
	mmovdw	%f0,16(%fp)	# n = (int) tmp - n is quadrant
	movw	16(%fp),%r8
	mmovfa	0(%fp)
	andb2	&0x3f,1(%fp)
	orb2	4(%fp),1(%fp)	# restore old rounding
	orb2	&0x2,1(%fp)	# set CSC
	mmovta	0(%fp)
	jmp	.sub
.round:
	mmovdw	%f0,16(%fp)	# n = (int) tmp - n is quadrant
	movw	16(%fp),%r8
.sub:
	alsw3	&3,%r8,%r0
	mfsubd2	leading(%r0),%f1	# y = y - leading[n]
	mfsubd2	trailing(%r0),%f1	# y = y-trailing[n]
.L51:
	bitw	&0x80000000,0(%ap)	# if (x < 0.0)
	jz	.L54
	mfnegd2	%f1,%f1		# y = -y
	subw3	%r8,&4,%r8	# n = 4 -n
.L54:
	cmpw	_fp_hw,&2	# determine machine version
	jne	.MAU106		# if 206, use machine instructions
	cmpw	&2,%r6
	je	.dotan2		# sin or cos
	addw2	%r6,%r8		# if (cosflag) n+= 1
	lrsw3	&1,%r8,%r7 	# register int sign = n/2
	bitw	&1,%r8		# if (n % 2)
	jz	.dosin2
	mfcosd2	%d1,%d2
	jmp	.comp2
.dosin2:
	mfsind2	%d1,%d2
.comp2:
	mmovds	%d2,4(%fp)
	movw	4(%fp),%r0
	bitw	&1,%r7		# if (sign % 2)
	jz	.return2
	xorw2	&0x80000000,%r0	# return - y
.return2:
	ret	&.R1
.dotan2:				# tan
	mfcosd2	%d1,%d2
	mfsind2	%d1,%d0
	bitw	&1,%r8		# if (n % 2)
	jz	.even2
	mfdivd2	%d0,%d2		# cos/sin
	mmovds	%d2,0(%fp)
	movw	0(%fp),%r0
	xorw2	&0x80000000,%r0	# return -cos/sin
	ret	&.R1
.even2:
	mfdivd2	%d2,%d0		# sin/cos
	mmovds	%d0,4(%fp)
	movw	4(%fp),%r0
	ret	&.R1
.MAU106:
	cmpw	&2,%r6		
	je	.dotan		# sin or cos
	addw2	%r6,%r8		# if cos, increment quadrant
	LRSW3	&1,%r8,%r7	# sign = n >> 1
	modw3	&2,%r8,%r1	# if (n% 2)
	jz	.L56
	addw2	&8,%sp		# y = _cos(q,p)
	mmovdd	%f1,-8(%sp)	# push y	
	call	&2,_cosf
	jmp	..0
.L56:
	addw2	&8,%sp		# y = _sin(q,p)
	mmovdd	%f1,-8(%sp)	# push y	
	call	&2,_sinf
..0:
	modw3	&2,%r7,%r2	# if (sign % 2)
	jz	.L58
	xorw2	&0x80000000,%r0	# return -y
.L58:
	ret	&.R1		# else return y
.dotan:
	mmovdd	%f1,8(%fp)	# save y
	pushw	8(%fp)
	pushw	12(%fp)
	call	&2,_cosf
	movw	%r0,0(%fp)	# tmp = _cosf(f)
	pushw	8(%fp)
	pushw	12(%fp)
	call	&2,_sinf
	movw	%r0,4(%fp)	# y = _sinf(f)
	bitw	&1,%r8		# if (n % 2)
	jz	.even
	mfdivs2	4(%fp),0(%fp)	# cos/sin
	movw	0(%fp),%r0	
	xorw2	&0x80000000,%r0	# return -cos/sin
	ret	&.R1		
.even:
	mfdivs2	0(%fp),4(%fp)	# sin/cos
	movw	4(%fp),%r0	
	ret	&.R1		# 
#------------------------------------------------------------------
	.align	4
	.text
	.set	.F2,20	
	.globl	cosf
	.align	4
	.set	.R2,3
#------------------------------------------------------------------
cosf:
	save	&.R2
	MCOUNT
	addw2	&.F2,%sp
	mfabss2 0(%ap),%f0	# y =  |x|
	cmpw	_fp_hw,&2	# if 32206 atached
	jne	.csoft
	mfcmpts	%f0,.M_PI_2	# if (|x| < Pi/2) use machine instr.
	jg	.csoft
	mfcoss2	%f0,0(%fp)
	movw	0(%fp),%r0
	ret	&.R2
.csoft:
	movw	&1,%r6		# indicates cos
	mmovsd	%s0,%d1 	# register double x
	jmp	.trig
	.text
#----------------------------------------------------------------
	.text
	.set	.F3,20
	.globl	tanf
	.align	4
	.set	.R3,3
#----------------------------------------------------------------
tanf:
	save	&.R3
	MCOUNT
	addw2	&.F3,%sp
	mfabss2 0(%ap),%f0		# y =  |x|
	cmpw	_fp_hw,&2	# if 32206 atached
	jne	.tsoft
	mfcmpts	%f0,.M_PI_2	# if (|x| < Pi/2) use machine instr.
	jg	.tsoft
	mfsins2	%f0,%f2
	mfcoss2	%f0,%f1
	mfdivs2	%f1,%f2		# tan = sin/cos
	mmovss	%f2,0(%fp)
	movw	0(%fp),%r0
	bitw	&0x80000000,0(%ap) # if (x < 0.0)
	jz	.tret
	xorw2	&0x80000000,%r0	  # return -sin
.tret:
	ret	&.R3
.tsoft:
	movw	&2,%r6		# indicates tanf
	mmovsd	%s0,%d1 	# register double x
	jmp	.trig

#######################################################
#	float _sinf(double q)
#	float _cosf(double q)
#	-pi/4 <= q <= pi/4, 
#	Algorithm and coefficients from paper by
#	Peter Teng
#######################################################
	.data
	.align	4
p:
	.word	0xb94cab5b
	.word	0x3c0883bf
	.word	0xbe2aaaa3
q:
	.word	0x37ccf127
	.word	0xbab6060f
	.word	0x3d2aaaa5
#----------------------------------------------------------
	.text
	.set	.F1,4
	.align	4
	.set	.R1,0
#---------------------------------------------------------
_sinf:
	save	&.R1
	MCOUNT
	addw2	&.F1,%sp
	mmovdd	0(%ap),%f2	# register double x
	mfmuld3	%f2,%f2,%f1	# xsq
	mmovds	%d1,%s1		# Xsq = round(xsq)
	mfmuls3	%s1,p,%s0	# Xsq * p[0]
	mfadds2	p+4,%s0		#   + p[1]
	mfmuls2	%s1,%s0		#   * Xsq
	mfadds2	p+8,%s0		#   + p[2]
	mfmuls2	%s1,%s0		# qx = Xsq * _POLY2(Xsq,p)
	mmovsd	%s0,%d0
	mfmuld2	%d2,%d0		# qx *= x
	mfaddd2	%d0,%d2		# x += qx
	mmovds	%d2,0(%fp)	# ...+ x
	movw	0(%fp),%r0
	ret	&.R1
#---------------------------------------------------------
	.data
	.align	4
.d_two:
	.word	0x40000000,0x0
.d_one:
	.word	0x3ff00000,0x0
#----------------------------------------------------------------
	.text
	.set	.F2,4
	.align	4
	.set	.R2,0
#---------------------------------------------------------------
_cosf:
	save	&.R2
	MCOUNT
	addw2	&.F2,%sp
	mmovdd	0(%ap),%f1	# register double x
	mfmuld2	%f1,%f1		# xsq in double
	mmovds	%d1,%s2		# Xsq = round(xsq)
	mfmuls3	%s2,q,%s0	# Xsq * q[0]
	mfadds2	q+4,%s0		# + q[1]
	mfmuls2	%s2,%s0		# * Xsq
	mfadds2	q+8,%s0		# + q[2]
	mfmuls2	%s2,%s0		# *Xsq
	mfmuls2	%s2,%s0		# qx = Xsq * Xsq * _POLY2(Xsq,q)
	mmovsd	%s0,%d0
	mfdivd2	.d_two,%f1	# xsq /= 2.0
	mfsubd2	%f0,%f1		# xsq -= qx
	mfsubd3	%f1,.d_one,%f2	# 1 - xsq
	mmovds	%f2,0(%fp)	# round(1 - (xsq/2 - qx))
	movw	0(%fp),%r0
	ret	&.R2
	.text
