	.file	"logf.s"
	.ident	"@(#)libm:m32mau/logf.s	1.7"
############################################################
#	float logf(float x)
#
#	coefficients from Cody and Waite (1980)
############################################################
	.data
	.align	4
.p:
	.word	0x3c5ed68a
	.word	0xbeee0830
.q:
	.word	0x3f800000
	.word	0xc0b28622
.one:
	.word	0x3f800000
.half:
	.word	0x3f000000
.M_SQRT_1_2:
	.word	0x3f3504f3
.C1:
	.word	0x3f318000
.C2:
	.word	0xb95e8083
.M_LOG10E:
	.word	0x3fdbcb7b,0x1526e50e
	.text
	.set	.F1,20
	.globl	logf
	.align	4
	.set	.R1,1
logf:
	save	&.R1
	MCOUNT
	addw2	&.F1,%sp
	movw	&0,%r8		# indicates log
	mmovss	0(%ap),%f0	# x > 0 ?
	jg	.common
	pushw	0(%ap)
	pushw	&.logf
	pushw	&4
	call	&3,log_error
	ret	&.R1
#-----------------------------------------------------------------
	.text
	.align	4

#-----------------------------------------------------------------
.common:
	mmovss	.one,%s1	# y = (float)1.0
	extzv	&23,&8,0(%ap),%r0	#inline expansion of frexp
	jg	.inline			#if exp==0 use frexp instead
	mmovsd	0(%ap),%f0
	addw2	&8,%sp
	mmovdd	%f0,-8(%sp)
	pushaw	8(%fp)
	call	&3,frexp
	movw	%r0,12(%fp)
	movw	%r1,16(%fp)
	mmovdd	12(%fp),%f0
	mmovds	%f0,0(%ap)
	jmp	.end_inline
	.align	4
.inline:
	subw3	&126,%r0,8(%fp)		# un-bias exponent
	insv	&126,&23,&8,0(%ap)	# force to biased 0
.end_inline:
	mmovss	0(%ap),%s2
	mfcmpts	%s2,.M_SQRT_1_2	# if (x < M_SQRT1_2)
	jge	.L46
	subw2	&1,8(%fp)	# n--
	mmovss	.half,%s1	#  y = half;
.L46:
	mfadds3	%d1,%d2,%d3	# x + y
	mfsubs2	%d1,%d2		# x -y
	mfdivs2 %d3,%d2		# x = (x-y)/(x+y)
	mfadds2	%d2,%d2		# x += x
	mfmuls3	%d2,%d2,%d1	# y = x * x
	mfmuls3	%d1,%d2,%d0	# .. x * y
	mfmuls3	%d1,.p,%d3	#   p[0] * y
	mfadds2	.p+4,%d3	#   +p[1]
	mfmuls2 %d3,%d0		# x * y * _POLY1(y, p)
	mfmuls3	%d1,.q,%d3	#  q[0] * y
	mfadds2	.q+4,%d3	#  +q[1]
	mfdivs2	%d3,%d0		# x * y * _POLY1(y,p)/_POLY1(y,q)
	mfadds2	%d0,%d2		# x += ...
	mmovws	8(%fp),%s1	# y = (float)n
	mfmuls3	.C2,%d1,%d0	# y * C2
	mfadds2 %d0,%d2		# x+= y * C2	
	mfmuls2	.C1,%d1		# y * C1
	mfadds2	%d2,%d1		# x + y * C1
	TSTW	%r8		# log or log10?
	jz	.no10
	mfmuld2	.M_LOG10E,%d1	# if log10 mult by log10(e)
.no10:
	mmovss	%f1,20(%fp)
	movw	20(%fp),%r0
	ret	&.R1
	.text
	.set	.F2,20
	.globl	log10f
	.align	4
	.set	.R2,1
log10f:
	save	&.R2
	MCOUNT
	addw2	&.F2,%sp
	movw	&1,%r8		# indicates log10
	mmovss	0(%ap),%f0
	jg	.common
	pushw	0(%ap)
	pushw	&.log10f
	pushw	&6
	call	&3,log_error
	ret	&.R2
	.data
	.align	4
.MHUGE:
	.word	0xc7efffff,0xe0000000
.MHUGE_VAL:
	.word	0xfff00000,0x0
	.text
	.set	.F3,36
	.align	4
	.set	.R3,1
log_error:
	save	&.R3
	addw2	&.F3,%sp
	movw	&0,%r8		# zflag = 0
	mfcmps	0(%ap),&0.0	# if (!z)
	jne	.L100
	movw	&1,%r8		# zflag = 1
.L100:
	movw	4(%ap),4(%fp)	# exc.name = f_name
	cmpw	_lib_version,&0	# if c_issue_4
	jne	.L101
	movw	.MHUGE,24(%fp)	# exc.retval = -HUGE
	movw	.MHUGE+4,28(%fp)
	jmp	.L103
.L101:
	movw	.MHUGE_VAL,24(%fp)	# else exc.retval = -HUGE_VAL
	movw	.MHUGE_VAL+4,28(%fp)
.L103:
	mmovsd	0(%ap),8(%fp)	# exc.arg1 = (double)x
	cmpw	_lib_version,&2	# if strict_ansi
	jne	.L104
	cmpw	%r8,&0		# if (!zflag)
	jne	.L105
	jmp	.L112
..0:
	cmpw	_lib_version,&0	# if c_issue_4
	jne	.L112
	pushw	&2		# write(2,f_name,namelength)
	pushw	4(%ap)
	pushw	8(%ap)
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
	mmovds	24(%fp),32(%fp)	# return (float)exc.retval
	movw	32(%fp),%r0
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
.logf:
	.byte	108,111,103,102,0
.log10f:
	.byte	108,111,103,49,48,102,0
	.text
