	.file	"atanf.s"
	.ident	"@(#)libm:m32mau/atanf.s	1.5"
#################################################################
#
#
#	float atanf(float x)
#	float atan2f(float y,float x) 
#	atan returns the arctangent of its single-precision argument,
#	in the range [-pi/2, pi/2]. There are no error returns
#
#	If a 32206 MAU is attached, the machine instruction is used.
#	The identity atan(x) = pi/2 -atan(1/x) is used for arguments
#	> 1.
#
#	atan2(y,x) returns the arctangent of y/x, in the range 
#	[-pi, pi]. atan2 discovers what quadrant the angle is in
#	and jumps to atan. atan2 returns EDOM error and value 0 if
#	both arguments are zero.
#
##################################################################
	.data
	.align	4
# double precision constants used to preserve accuracy
athfhi:
	.word	0x3fddac67,0x561bb4f
athflo:
	.word	0x3c55543b,0x8f253271
at1fhi:
	.word	0x3fef730b,0xd281f69b
at1flo:
	.word	0xbc7c23df,0xefeae6b5
.M_PI_4:
	.word	0x3fe921fb,0x54442d18
.M_PI_2:
	.word	0x3ff921fb,0x54442d18
.d_one:
	.word	0x3ff00000,0x0
a:
	.word	0xbd6f0930
	.word	0x3d886d49
	.word	0xbd9d87d3
	.word	0x3dba2e73
	.word	0xbde38e38
	.word	0x3e124925
	.word	0xbe4ccccd
	.word	0x3eaaaaab
.f_one:
	.word	0x3f800000
.f_zero:
	.word	0x0
.f_two:
	.word	0x40000000
.big:
	.word	0x4cbebc20		#1.e8
.L82:
	.word	0x401c0000 		#2.4375
.L83:
	.word	0x3d800000		# 0.0625
.f_four:
	.word	0x40800000
.small:
	.word	0x39800000		# FX_EPS
.L106:
	.word	.L86
	.word	.L87
	.word	.L93
	.word	.L96
	.word	.L97
	.align	4
.f_negone:
	.word	0xbf800000
.N_PI_2:
	.word	0xbfc90fdb
.FM_PI_2:
	.word	0x3fc90fdb
.M_PI:
	.word	0x40490fdb
.DM_PI:
	.word	0x400921fb,0x54442d18		
# special value of PI, 1 ulp less than M_PI rounded to float
# to satisfy range requirements - (float)M_PI is actually greater
# than true pi
.FM_PI:
	.word	0x40490fda
#-----------------------------------------------------------------
	.text
	.set	.F1,44
	.globl	atanf
	.align	4
	.set	.R1,5
#-----------------------------------------------------------------a
atanf:
	save	&.R1
	MCOUNT
	addw2	&.F1,%sp
	movw	&0,%r6		# indicates atan rather than atan2
	mfabss2	0(%ap),%d2	# register float x = |x|
	extzv	&31,&1,0(%ap),%r7 # register int signx = sign bit of x
#-----------------------------------------------------------------
	.text
	.align	4

#-----------------------------------------------------------------
# code common to atan and atan2
.common:
	cmpw	_fp_hw,&2	# determine hardware type
	jne	.MAU106
	movw	&0,%r2		# less than 1?
	mmovsd	%s2,%d2
	mfcmptd	%f2,.d_one	# if (|x| > 1.)
	jle	.less1
	mfdivd3	%d2,.d_one,%d2	# x = 1/x
	movw	&1,%r2
.less1:
	mfatnd2	%d2,%d1
	bitw	&1,%r2		# if > 1
	jz	.less2
	mfsubd3	%d1,.M_PI_2,%d1 # Pi/2 - atan(x)
.less2:
	cmpw	&1,%r7
	jne	.noneg
	mfnegd2	%d1,%d1
.noneg:
	jmp	.creturn
.MAU106:
	mfcmpts	%f2,.L82	#if (x < 2.4375)
	jge	.L80
	mfadds3	.L83,%d2,%d0	# tmp = x + 0.0625
	mfmuls2	.f_four,%d0	# tmp *= 4.0
	mmovsw	%f0,12(%fp)	# register int k = (int)tmp
	movw	12(%fp),%r8
	cmpw	%r8,&4		# switch(k)
	jg	.L102
	ALSW3	&2,%r8,%r0
	cmpw	%r8,&0
	jnneg	*.L106(%r0)
.L102:
	movw	at1fhi,0(%fp)	# default: x in [19/16,39/16]
	movw	at1fhi+4,4(%fp)
	movw	at1flo,8(%fp)	# lo = at1flo
	movw	at1flo+4,12(%fp)
	mfsubs3	.f_one,%d2,%d1	# z = x -1
	mfadds3	%d2,%d2,%d0	# x = x +x + x
	mfadds2	%d0,%d2		
	mfadds3	%d2,.f_two,%d0	# x + 2.0
	mfadds2	%d1,%d1		# z +=z
	mfsubs2	.f_one,%d1	# -1
	mfdivs3	%d0,%d1,%d2		# x = ((z+z)-1)/(2 + x)
	jmp	.L84
.L86:				# case 0:
.L87:				# case 1: x in [0,7/16]
	mfcmpts	%f2,.small	# if (x < small)
	jge	.L88
	cmpw	%r7,&1		# return(signx ? -x : x)
	jne	.L89
	mfnegs2	%f2,%f1
	jmp	.creturn
.L89:
	mmovss	%f2,%f1
	jmp	.creturn
.L88:				
	movw	&0,0(%fp)	# hi = 0.0
	movw	&0,4(%fp)	
	movw	&0,8(%fp)	# lo = 0.0
	movw	&0,12(%fp)
	jmp	.L84
.L93:				# case 2: x in [7/16,11/16]
	movw	athfhi,0(%fp)	# hi = athfhi
	movw	athfhi+4,4(%fp)
	movw	athflo,8(%fp)	# lo = athflo
	movw	athflo+4,12(%fp)
	mfadds3	%d2,.f_two,%d1	# 2  + x
	mfadds2	%d2,%d2		# x + x
	mfsubs2	.f_one,%d2	# -1
	mfdivs2	%d1,%d2		# x = ((x+x)-1)/(x + 2)
	jmp	.L84
.L96:				# case 3:
.L97:				# case 4: x in [11/16/19/16]
	movw	.M_PI_4,0(%fp) # hi = pi/4
	movw	.M_PI_4+4,4(%fp)
	movw	&0,8(%fp)	# lo = 0
	movw	&0,12(%fp)	
	mfadds3	.f_one,%d2,%d1	# x + 1
	mfsubs2	.f_one,%d2	# x -1
	mfdivs2	%d1,%d2		# x = (x-1)/(x+1)
	jmp	.L84
.L80:
	mfcmpts	%f2,.big	# (x > 2.4375) if (x <= big)
	jg	.L108
	mfdivs3	%d2,.f_negone,%d2	# x = -1/x
	movw	.M_PI_2,0(%fp)		# hi = pi/2
	movw	.M_PI_2+4,4(%fp)
	movw	&0,8(%fp)		# lo = 0.0
	movw	&0,12(%fp)
.L84:
	mfmuls3	%d2,%d2,%d1	# z = x * x
	mfmuls3	%d1,a,%d0	# z * a[0]
	mfadds2	a+4,%d0		#  + a[1]
	mfmuls2	%d1,%d0		#  * z
	mfadds2	a+8,%d0		# + a[2]
	mfmuls2	%d1,%d0		# * z
	mfadds2	a+12,%d0	# + a[3]
	mfmuls2	%d1,%d0		# ....
	mfadds2	a+16,%d0
	mfmuls2	%d1,%d0
	mfadds2	a+20,%d0
	mfmuls2	%d1,%d0
	mfadds2	a+24,%d0
	mfmuls2	%d1,%d0
	mfadds2	a+28,%d0	# + a[7]  _POLY7(z,a)
	mfmuls2	%d2,%d1		# z *= x
	mfmuls2	%d0,%d1		# z = x * z * _POLY7(z,a)
	mfsubd3	%d1,8(%fp),%d1	# z = lo -z - done in double precision
	mfaddd2	%d2,%d1		# z += x
	mfaddd2	0(%fp),%d1	# z += hi
	cmpw	%r7,&1		# atan - return(signx ? -z :z)
	jne	.creturn
	mfnegd2	%f1,%f1
	jmp	.creturn
.L108:				# x > big
	cmpw	%r7,&1		# return(signx ? -pi/2 : pi/2)
	jne	.L113
	mmovss	.N_PI_2,%f1
	jmp	.creturn
.L113:
	mmovss	.FM_PI_2,%f1
.creturn:
	cmpw	%r6,&0
	je	.done		# atan - or atan2?
	cmpw	%r4,&1		# atan2 - is x negative ?
	je	.x_isneg
.done:
	mmovss	%f1,16(%fp)
	movw	16(%fp),%r0
	ret	&.R1		# return at
.x_isneg:
	mfcmps	%f1,.f_zero
	je	.at0		# atan(y/x) == 0.0
	cmpw	%r5,&0		# else if (neg_y)
	je	.not_negy
	mfsubd2	.DM_PI,%f1	# return at - M_PI
	jmp	.done2
.not_negy:
	mfaddd2	.DM_PI,%f1	# else return at + M_PI
.done2:
	mmovds	%f1,16(%fp)
	movw	16(%fp),%r0
	ret	&.R1
.at0:					# atan(y/x) == 0 - return FM_PI
	movw	.FM_PI,%r0
	ret	&.R1
	.data
	.align	4
#------------------------------------------------------------
				# struct exception {
	.set	type,0		# 	int type;
	.set	name,4		#	char *name;
	.set	arg1,8		#	doube arg1;
	.set	arg2,16		#	double arg2;
	.set	retval,24	#	double retval;
				# };
	.set	DOMAIN,1
	.set	EDOM,33
#----------------------------------------------------------

	.text
	.align	4
	.globl	atan2f
	.align	4
	.set	.F2,44
	.set	.R2,5
#--------------------------------------------------------------------
atan2f:
	save	&.R2
	MCOUNT
	addw2	&.F2,%sp
	movw	&0,%r5		# register int neg_y = 0
	movw	&0,%r4		# register int neg_x = 0
	mmovss	0(%ap),%f2	# register float y
	jnz	.no_err1
	mmovss	4(%ap),%f1	# register float x
	jnz	.no_err2
				# x and y == 0
				# set up for error return
	movw	&1,0(%fp)	# exc.type = DOMAIN
	movw	&.tan2,4(%fp)	# exc.name = "atan2f"
	mmovsd	%f2,8(%fp)	# exc.arg1 = (double)y
	mmovsd	%f1,16(%fp)	# exc.arg2 = (double)x
	movw	&0,24(%fp)	 # exc.retval = 0.0
	movw	&0,28(%fp)
	pushaw	0(%fp)		# &exc
	cmpw	_lib_version,&2		# if (_lib_version==strict_ansi)
	jne	.nstrict
	movw	&EDOM,errno
	jmp	.L104
.nstrict:
	pushaw	type(%fp)		# &exc
	call	&1,matherr
	cmpw	%r0,&0		# if (!matherr(&exc))
	jne	.L104
	cmpw	_lib_version,&0		# if (_lib_version==c_issue_4)
	jne	.nomess
	pushw	&2		# _write(2,"atan2f: DOMAIN error\n",21)
	pushw	&.err_mess
	pushw	&21
	call	&3,_write
.nomess:
	movw	&33,errno	# errno = EDOM
.L104:
	mmovds	24(%fp),40(%fp)
	movw	40(%fp),%r0	# return (float) exc.retval
	ret	&.R2
.no_err1:
	mmovss	4(%ap),%f1		# register double x
	jge	.not_neg1		# if (x < 0)
	mfnegs2	%f1,%f1			# x = -x
	movw	&1,%r4			# neg_x = 1
.not_neg1:
	mfcmpts	%f2,.f_zero		# y < 0 ?
	jge	.not_neg2
	mfnegs2	%f2,%f2			# y = -y
	movw	&1,%r5			# neg_y = 1
	jmp	.not_neg2
.no_err2:
	mfcmpts %f1,.f_zero		# x < 0 ?
	jge	.not_neg2
	mfnegs2	%f1,%f1			# x = -x
	movw	&1,%r4			# neg_x = 1
.not_neg2:
	mfsubs3	%f1,%f2,%f0		# |y| - |x|
	mfcmps	%f2,%f0			# if (|y| - |x| == |y|)
	jne	.L110
	cmpw	%r5,&0			# neg_y?
	je	.L114
	movw	.N_PI_2,%r0		# return -PI/2
	ret	&.R2
.L114:
	movw	.FM_PI_2,%r0		# : return PI/2
	ret	&.R2
.L110:
	mfdivs2	%x1,%x2			# |y|/|x|
	movw	&1,%r6			# indicates atan2
	addw3	%r4,%r5,%r7		# r4 + r5 : 0,2 indicates y/ pos
					# 	1 indicates y/x neg
	jmp	.common
.tan2:
	.byte	97,116,97,110,50,102,0
	.align	4
.err_mess:
	.byte	97,116,97,110,50,102,58,32,68,79,77
	.byte	65,73,78,32,101,114,114,111,114,10
	.byte	0
	.text
