	.file	"atan.s"
	.ident	"@(#)libm:m32mau/atan.s	1.6"
#################################################################
#
#
#	double atan(x) double x;
#	double atan2(y,x) double x, y;
#	atan returns the arctangent of its double-precision argument,
#	in the range [-pi/2, pi/2]. There are no error returns.
#	If a 32206 MAU is attached, the machine instruction is used.
#	The identity atan(x) = pi/2 -atan(1/x) is used for arguments
#	> 1.
#
#	atan2(y,x) returns the arctangent of y/x, in the range 
#	[-pi, pi]. atan2 discovers what quadrant the angle is in
#	and calls atan. atan2 returns EDOM error and value 0 if
#	both arguments are zero.
#
##################################################################
	.data
	.align	4
# extended precision constants
atn0.5:
	.word	0x3ffd,0xed63382b,0x0dda7b45
XM_PI_2:
	.word	0x3fff,0xc90fdaa2,0x2168c235	
atn1_0:
	.word	0x3ffe,0xfb985e94,0x0fb4d900
XM_PI_4:				
	.word	0x3ffe,0xc90fdaa2,0x2168c235

.x_one:
	.word	0x3fff,0x80000000,0x0
# double precision constants
a:
	.word	0x3f90ad3a,0xe322da11
	.word	0xbfa2b444,0x2c6a6c2f
	.word	0x3fa97b4b,0x24760deb
	.word	0xbfadde2d,0x52defd9a
	.word	0x3fb10d66,0xa0d03d51
	.word	0xbfb3b0f2,0xaf749a6d
	.word	0x3fb745cd,0xc54c206e
	.word	0xbfbc71c6,0xfe231671
	.word	0x3fc24924,0x920083ff
	.word	0xbfc99999,0x9998ebc4
	.word	0x3fd55555,0x5555550d
.small:
	.word	0x3e112e0b,0xe826d695		# 1e-9
.big:
	.word	0x43abc16d,0x674ec800		# 1e18
.L81:
	.word	0x40038000,0x0			# 2.4375
.L82:
	.word	0x3fb00000,0x0			# 0.0625
.d_four:
	.word	0x40100000,0x0	
.XM_PI:
	.word	0x4000,0xc90fdaa2,0x2168c234	# pi in extended 
.d_zero:
	.word	0x0,0x0
.d_one:
	.word	0x3ff00000,0x0
.d_two:
	.word	0x40000000,0x0
.L106:
	.word	.L86
	.word	.L87
	.word	.L93
	.word	.L96
	.word	.L97
	.align	4
.d_negone:
	.word	0xbff00000,0x0
.M_PI_2:
	.word	0x3ff921fb,0x54442d18
.N_PI_2:
	.word	0xbff921fb,0x54442d18
#-----------------------------------------------------------------
	.text
	.set	.F1,40
	.globl	atan
	.align	4
	.set	.R1,5
#------------------------------------------------------------------
atan:
	save	&.R1
	MCOUNT   
	addw2	&.F1,%sp
	movw	&0,%r6		# indicates atan rather than atan2
	mfabsd2	0(%ap),%x2	# register double x = |x|
	extzv	&31,&1,0(%ap),%r7 # register int signx = sign bit of x
#-----------------------------------------------------------------
	.text
	.align	4

#-----------------------------------------------------------------
.common:

	cmpw	_fp_hw,&2	# determine hardware type
	jne	.MAU106
	CLRW	%r2		# less than 1?
	mmovdx	%d2,%x2
	mfcmptx	%f2,.x_one	# if (|x| > 1.)
	jle	.less1
	mfdivx3	%x2,.x_one,%x2	# x = 1/x
	movw	&1,%r2
.less1:
	mfatnx2	%x2,%x1 
	TSTW	%r2		# if > 1
	jz	.less2
	mfsubx3	%x1,XM_PI_2,%x1 # Pi/2 - atan(x)
.less2:
	cmpw	&1,%r7
	jne	.noneg
	mfnegx2	%x1,%x1
.noneg:
	jmp	.creturn
.MAU106:
	mfcmptd	%f2,.L81	#if (x < 2.4375)
	jge	.L80
	mfaddd3	.L82,%x2,%x0	# tmp = x + 0.0625
	mfmuld2	.d_four,%x0	# tmp *= 4.0
	mmovdw	%f0,24(%fp)	# register int k = (int)tmp
	movw	24(%fp),%r8
	cmpw	%r8,&4		# switch(k)
	jg	.L102
	ALSW3	&2,%r8,%r0
	cmpw	%r8,&0
	jnneg	*.L106(%r0)
.L102:
	movw	atn1_0,0(%fp)	# default: x in [19/16,39/16]
	movw	atn1_0+4,4(%fp)	
	movw	atn1_0+8,8(%fp)	
	mfsubd3	.d_one,%x2,%x1	# z = x -1
	mfaddd3	%x2,%x2,%x0	# x = x +x + x
	mfaddd2	%x0,%x2		
	mfaddd3	%x2,.d_two,%x0	# x + 2.0
	mfaddd2	%x1,%x1		# z +=z
	mfsubd2	.d_one,%x1	# -1
	mfdivd3	%x0,%x1,%x2		# x = ((z+z)-1)/(2 + x)
	jmp	.L84
.L86:				# case 0:
.L87:				# case 1: x in [0,7/16]
	mfcmptd	%f2,.small	# if (x < small)
	jge	.L88
	cmpw	%r7,&1		# return(signx ? -x : x)
	jne	.L89
	mfnegd2	%f2,%f1
	jmp	.creturn
.L89:
	mmovdd	%f2,%f1
	jmp	.creturn
.L88:				
	movw	&0,0(%fp)	# hi = 0.0
	movw	&0,4(%fp)
	movw	&0,8(%fp)
	jmp	.L84
.L93:				# case 2: x in [7/16,11/16]
	movw	atn0.5,0(%fp)	# hi = atn0.5
	movw	atn0.5+4,4(%fp)
	movw	atn0.5+8,8(%fp)
	mfaddd3	%x2,.d_two,%x1	# 2  + x
	mfaddd2	%x2,%x2		# x + x
	mfsubd2	.d_one,%x2	# -1
	mfdivd2	%x1,%x2		# x = ((x+x)-1)/(x + 2)
	jmp	.L84
.L96:				# case 3:
.L97:				# case 4: x in [11/16/19/16]
	movw	XM_PI_4,0(%fp) # hi = pi/4
	movw	XM_PI_4+4,4(%fp)
	movw	XM_PI_4+8,8(%fp)
	mfaddd3	.d_one,%x2,%x1	# x + 1
	mfsubd2	.d_one,%x2	# x -1
	mfdivd2	%x1,%x2		# x = (x-1)/(x+1)
	jmp	.L84
.L80:
	mfcmptd	%f2,.big	# (x > 2.4375) if (x <= big)
	jg	.L108
	mfdivd3	%x2,.d_negone,%x2	# x = -1/x
	movw	XM_PI_2,0(%fp)
	movw	XM_PI_2+4,4(%fp)	# hi = pi/2
	movw	XM_PI_2+8,8(%fp)	# hi = pi/2
.L84:
	mfmuld3	%x2,%x2,%x1		# z = x * x
	mfmuld3	%x1,a,%x0		# z * a[0]
	mfaddd2	a+8,%x0			#  + a[1]
	mfmuld2	%x1,%x0			#  * z
	mfaddd2	a+16,%x0		# + a[2]
	mfmuld2	%x1,%x0			# * z
	mfaddd2	a+24,%x0		# + a[3]
	mfmuld2	%x1,%x0			# ....
	mfaddd2	a+32,%x0
	mfmuld2	%x1,%x0
	mfaddd2	a+40,%x0
	mfmuld2	%x1,%x0
	mfaddd2	a+48,%x0
	mfmuld2	%x1,%x0
	mfaddd2	a+56,%x0
	mfmuld2	%x1,%x0
	mfaddd2	a+64,%x0
	mfmuld2	%x1,%x0
	mfaddd2	a+72,%x0
	mfmuld2	%x1,%x0
	mfaddd2	a+80,%x0		# + a[10]  _POLY10(z,a)
	mfmuld2	%x2,%x1		# z *= x
	mfmuld2	%x0,%x1		# z = x * z * _POLY10(z,a)
	mfsubx3	%x1,%x2,%x1	# z = x - z
	mfaddx2	0(%fp),%x1	# z += hi
	cmpw	%r7,&1		# return(signx ? -z :z)
	jne	.creturn
	mfnegx2	%f1,%f1
	jmp	.creturn
.L108:				# x > big
	cmpw	%r7,&1		# return(signx ? -pi/2 : pi/2)
	jne	.L113
	mmovdd	.N_PI_2,%f1
	jmp	.creturn
.L113:
	mmovdd	.M_PI_2,%f1
.creturn:
	cmpw	%r6,&0
	je	.done		# atan - or atan2?
	cmpw	%r4,&1		# atan2 - is x negative ?
	je	.x_isneg
.done:
	mmovdd	%f1,16(%fp)
	movw	16(%fp),%r0
	movw	20(%fp),%r1
	ret	&.R1		# return at
.x_isneg:
	cmpw	%r5,&0		# else if (neg_y)
	je	.not_negy
	mfsubx2	.XM_PI,%f1	# return at - XM_PI
	jmp	.done2
.not_negy:
	mfaddx2	.XM_PI,%f1	# else return at + XM_PI
.done2:
	mmovxd	%f1,16(%fp)
	movw	16(%fp),%r0
	movw	20(%fp),%r1
	ret	&.R1
	.data
	.align	4
#------------------------------------------------------------
				# struct exception {
	.set	type,0		# 	int type;
	.set	name,4		#	char *name;
	.set	arg1,8		#	double arg1;
	.set	arg2,16		#	double arg2;
	.set	retval,24	#	double retval;
				# };
	.set	DOMAIN,1
	.set	EDOM,33
#----------------------------------------------------------

	.text
	.set	.F2,40
	.align	4
	.globl	atan2
#--------------------------------------------------------------------
atan2:
	save	&5
	MCOUNT 
	addw2	&.F2,%sp
	movw	&0,%r5			# register int neg_y = 0
	movw	&0,%r4			# register int neg_x = 0
	mmovdd	0(%ap),%f2		# register double y
	jnz	.no_err1
	mmovdd	8(%ap),%f1		# register double x
	jnz	.no_err2
					# y == 0 and x == 0
					# set up for error return
	movw	&DOMAIN,type(%fp)	# exc.type = DOMAIN
	movw	&.tan2,name(%fp)	# exc.name = "atan2"
	mmovdd	%d2,arg1(%fp)		# exc.arg1 = y;
	mmovdd	%d1,arg2(%fp)		# exc.arg2 = x;
	movw	&0,retval(%fp)	# exc.retval = 0.0
	movw	&0,retval+4(%fp)
	cmpw	_lib_version,&2		# if (_lib_version==strict_ansi)
	jne	.nstrict
	movw	&EDOM,errno
	jmp	.err_ret
.nstrict:
	pushaw	type(%fp)		# &exc
	call	&1,matherr		# matherr(&exc)
	cmpw	%r0,&0			# if (!matherr(&exc))
	jne	.err_ret
	cmpw	_lib_version,&0		# if (_lib_version==c_issue_4)
	jne	.nomess
	pushw	&2		
	pushw	&.err_mess
	pushw	&20
	call	&3,_write		#_write(2,"atan2: DOMAIN error\n",20)
.nomess:
	movw	&EDOM,errno		# errno = EDOM
.err_ret:
	movw	retval+4(%fp),%r1
	movw	retval(%fp),%r0
	ret	&5
.no_err1:
	mmovdd	8(%ap),%f1		# register double x
	jge	.not_neg1		# if (x < 0)
	mfnegd2	%f1,%f1			# x = -x
	movw	&1,%r4			# neg_x = 1
.not_neg1:
	mfcmptd	%f2,.d_zero		# y < 0 ?
	jge	.not_neg2
	mfnegd2	%f2,%f2			# y = -y
	movw	&1,%r5			# neg_y = 1
	jmp	.not_neg2
.no_err2:
	mfcmptd	%f1,.d_zero		# x < 0 ?
	jge	.not_neg2
	mfnegd2	%f1,%f1			# x = -x
	movw	&1,%r4			# neg_x = 1
.not_neg2:
	mfsubd3	%f1,%f2,%f0		# if (|y| -|x| == |y|)
	mfcmpd	%f0,%f2
	jne	.not_eq
	cmpw	%r5,&0			# if (neg_y)
	je	.not_neg
	movw	.N_PI_2+4,%r1		# return -M_PI_2
	movw	.N_PI_2,%r0
	ret	&5
.not_neg:
	movw	.M_PI_2+4,%r1		# else return M_PI_2
	movw	.M_PI_2,%r0
	ret	&5
.not_eq:
	mfdivd2	%x1,%x2			# y/x
	movw	&1,%r6			# indicates atan2
	addw3	%r4,%r5,%r7		# r4 + r5 : 0,2 indicates y/ pos
					# 	1 indicates y/x neg
	jmp	.common
.tan2:
	.byte	97,116,97,110,50,0
.err_mess:
	.byte	97,116,97,110,50,58,32,68,79,77
	.byte	65,73,78,32,101,114,114,111,114,10
	.byte	0
	.text
