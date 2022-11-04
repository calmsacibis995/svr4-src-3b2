	.file	"reduce.s"
	.ident	"@(#)libm:m32mau/reduce.s	1.2"
#####################################################################
#	Range reduction for the trigonemetric functions
#
#       See Payne and Hanek, SIGNUM Jan '83 page 19
#	Reduction is quadrant.
#
#       This implementation assumes:
#       a) the range of double precision floating point
#          is no greater than IEEE double precision,
#          else one needs more bits of pi.
#       b) L is less than 32 
#       c) L > 3
#       d) longs are 32 bits
#       e) floating point add/subtract are done via round to nearest
#       f) probably doesn't work on non-binary machines without some
#          more work.
#------------------------------------------------------------------
#   Inputs:
#   x is the argument to be reduced.  x is assumed to be positive and
#       greater than M_PI/2.
#
#   i = 0     -> reduced argument between 0 and pi/2
#       1     -> reduced argument between -pi/4 and pi/4
#
#   Outputs:
# 
#   I      0 <= I <= 3    (integer) == quadrant number
#
#   *hr     reduced argument in double extended precision
#		h*r  (where 0 <= h < 1.0,  r = (pi/2) )
#			(or -0.5 <= h < 0.5, for i == 1)
#
#   relationship of variables:
#
#   x = hr + (I*pi/2)    (modulo 2*pi)
#
#####################################################################

	.data
	.align	4
pihex:			# bits of 1/2pi 
	.align	4
	.word	0
	.word	0
	.word	0
	.word	0
	.word	0
	.word	683565275
	.word	-1819212470
	.word	2131351028
	.word	2102212464
	.word	920167782
	.word	1326507024
	.word	2140428522
	.word	-139529896
	.word	1841896334
	.word	-1869384520
	.word	26364858
	.word	-2106301305
	.word	1065843399
	.word	743074255
	.word	-1172271747
	.word	1269748001
	.word	979835913
	.word	-1390944368
	.word	1315206542
	.word	1624559229
	.word	656480226
	.word	-276936178
	.word	-939645441
	.word	-142514685
	.word	-70531998
	.word	-696083641
	.word	-615669837
	.word	-906837395
	.word	-741240871
	.word	-1483212149
	.word	1565126321
	.word	-84312994
	.word	-817770883
	.word	-493574982
	.word	-1694574612
	.word	1206081346
	.word	360762385
	.word	-1088496918
	.word	-63705336
	.word	649923975
	.word	1786307672
	.word	1471776450
	.word	426139991
	.word	-987227632
	.word	595588640
	.word	324848076
	.word	1099007317
	.word	-1298339278
	.word	1480105712
	.word	588960241
	.word	108059123
	.word	2000290378
	.word	-1596559075
	.word	-1562015959
	.word	-1217670860
	.word	1439056207
	.word	-1451245119
	.word	1340050682
	.word	212907923
	.word	-435380370
	.word	-254850629
	.word	-624453698
	.word	667846605
	.word	1924964295
	.word	1643923556
	.word	522127642
	.word	-1097091747
	.word	-884207410
	.word	-982416083
	.word	1732669680
	.word	-1322561453
	.word	1259814915
	.word	295657867
	.word	1551427249
	.word	-1497360244
	.word	-550195931
	.word	-370330283
	.word	1308463029
	.word	-972805889
	.word	496192167
	.word	1495987676
	.word	1042225365
	.word	-333530683
	.word	1171624560
	.word	913673934
	.word	556397256
	.word	-920788299
	.word	-1999300946
	.word	1203766978
	.word	-1132989562
	.word	2144102362
	.word	-2019977131
	.word	-430833941
	.word	908058541
	.word	-649551238
	.align	4
.x_zero:
	.word	0x0,0x0,0x0
.x_one:
	.word	0x3fff,0x80000000,0x0
.x_four:
	.word	0x4001,0x80000000,0x0
.x_half:
	.word	0x3ffe,0x80000000,0x0
.M_PI_2:				# in double-extended
	.word	0x3fff,0xc90fdaa2,0x2168c235	# 1.57079632679489661923
.twom15:
	.word	0x3ff0,0x80000000,0x0
#---------------------------------------------------------------------
	.text
	.set	.F1,644
	.globl	_reduce
	.align	4
	.set	.R1,6
#---------------------------------------------------------------------
_reduce:
	save	&.R1
	MCOUNT
	addw2	&.F1,%sp
	movw	&0,4(%fp)		# sign
	movw	&0,628(%fp)		# onemhflag
	extzv	&20,&11,0(%ap),%r0
	cmpw	%r0,&2047		# k = unbiased exp of x
	je	.L75			# if (k == MAXEXP) don't unbias
	subw2	&1022,%r0
.L75:
	movw	%r0,0(%fp)	     # k 
	extzv	&13,&7,0(%ap),%r0    # break mantissa into 15 bit chunks
	orw2	&128,%r0	 	# or in implied 1 bit
	movw	%r0,52(%fp)		# F[3]
	extzv	&0,&13,0(%ap),%r0
	LLSW3	&2,%r0,%r0
	extzv	&30,&2,4(%ap),%r1
	orw2	%r1,%r0
	movw	%r0,48(%fp)		# F[2]
	extzv	&15,&15,4(%ap),44(%fp)	# F[1]
	extzv	&0,&15,4(%ap),40(%fp)	# F[0]
	movw	&8,%r5			# M = M1
	movw	&0,%r6			# for(l=0;l<NUMPARTIAL;l++)
	movaw	56(%fp),%r2		#   partial[l] = 0
.L76:
	alsw3	&2,%r6,%r1
	addw3	%r1,%r2,%r0
	movw	&0,0(%r0)
	addw2	&1,%r6
	cmpw	%r6,&143
	jl	.L76
.L78:
	movw	&1,%r7			# for (j =1;j<=M; j++)
					# find right 15 bits of 1/2pi
.L79:
	addw3	&92,0(%fp),%r0		# k - PRECISION - L + PADDING*CHARSIZE
	mulw3	&15,%r7,%r1		# L * j
	addw2	%r1,%r0			# m = k - PRESISION - L + L *j
	modw3	&32,%r0,%r8		# mmod32 = m % 32
	divw3	&32,%r0,%r6		# mover32 = m /32
	alsw3	&2,%r6,%r0
	LLSW3	%r8,pihex(%r0),%r0	#val1= phihex[mover32]<<mmod32
	LRSW3	&17,%r0,%r0		# val1 >>= 32-L
	andw2	&32767,%r0		# val1 &= LMASK
	cmpw	%r8,&17		# if mmod32 <= 32-L
	jg	.L82
	movw	%r0,36(%fp)		# gval = val1
	jmp	.L83
.L82:
	addw3	&1,%r6,%r1		# mover32 + 1
	alsw2	&2,%r1
	subw3	%r8,&49,%r2		# 64 - L - mmod32
	LRSW3	%r2,pihex(%r1),%r1   # val2= pihex[mover32+1]>>64-L-mmod32
	andw2	&32767,%r1		# val2 &= LMASK
	addw3	%r0,%r1,36(%fp)		# gval = val1 + val2
.L83:
	movw	&0,%r8		# for (n=0;(n<4)&&(n<=j);n++)
.L84:
	movaw	40(%fp),%r0
	alsw3	&2,%r8,%r1
	addw2	%r1,%r0
	mulw3	36(%fp),0(%r0),%r4	# prod = F[n] * gval
	movaw	56(%fp),%r2
	subw3	%r8,%r7,%r6	# for(l=j-n;(l>=0)&&(prod!=0);l--)
	jl	.L89
	cmpw	%r4,&0
	je	.L89
.L87:
	alsw3	&2,%r6,%r1
	addw3	%r1,%r2,%r0
	addw3	0(%r0),%r4,%r3		# tot = partial[l] + prod
	modw3	&32768,%r3,0(%r0)	# partial[l] = tot%TWOL
	divw3	&32768,%r3,%r4		# prod = tot /TWOL
	subw2	&1,%r6
	jl	.L89
	cmpw	%r4,&0
	jne	.L87
.L89:
	addw2	&1,%r8
	cmpw	%r8,&4
	jge	.L86
	cmpw	%r8,%r7
	jle	.L84
.L86:
	addw2	&1,%r7
	cmpw	%r7,%r5
	jle	.L79
					# calculate I (quadrant)
	alsw3	&2,60(%fp),%r0		# tmp = partial[1] * 4
	divw2	&32768,%r0		# tmp /= TWOL
	alsw3	&2,56(%fp),%r1
	addw2	%r1,%r0			# tmp += partial[0] * 4
	modw3	&4,%r0,*12(%ap)		# *I = tmp %4
					# determine whether to return
					# h or 1-h
	cmpw	&0,8(%ap)		# if (i)
	je	.noti
	andw3	&0x1fff,60(%fp),%r0	# partial[1] & 0x1fff
	cmpw	&0x1000,%r0		# if r0 > 0x1000, h > 1/2
	jg	.loop
	movw	&1,4(%fp)		# sign = 1
	movw 	&1,628(%fp)		# onemhflag =1 (return 1-h)
	addw2	&1,*12(%ap)		# *I += 1
	jmp	.loop
.noti:
	modw3	&2,*12(%ap),%r0		# if ((*I) % 2)
	jz	.loop
	movw	&1,628(%fp)		# onemhflag =1
.loop:				# loop begin - test for loss of 
				# significance and generate more bits
				# if necessary
					# calculate h or 1-h
	cmpw	&0,628(%fp)		#if (!onemhflag)
	jne	.onemh
	mmovxx	.x_zero,%x2		# h = 0.0
	jmp	.h
.onemh:
	mmovxx	.x_one,%x2		# onemh = 1.0
.h:
	mmovxx	.x_four,%x1		# factor = 4.
	movaw	56(%fp),%r2
	movw	&1,%r6			# for(l=1;l <=M;l++)
.L98:
	mfmulx2	.twom15,%x1		# factor *= 2^-15
	alsw3	&2,%r6,%r1
	addw3	%r1,%r2,%r0
	mmovwx	0(%r0),%x3		# (long double)partial[l]
	mfmulx3	%f1,%f3,%f0		# tmp = factor * partial[l]
	mfcmptx	%x0,.x_one		# if (temp >= 1.0)
	jl	.L102
	mmovfa	636(%fp)		# c truncates
	andb3	&0xc0,637(%fp),640(%fp)
	orb2	&0xc2,637(%fp)
	mmovta	636(%fp)
	mfrndx2	%f0,%f3			# (int)temp
	mmovfa	636(%fp)
	andb2	&0x3f,637(%fp)
	orb2	640(%fp),637(%fp)
	orb2	&0x2,637(%fp)
	mmovta	636(%fp)
	mfsubx2	%f3,%f0			#temp = temp - (int)temp;
.L102:
	cmpw	&0,628(%fp)		#if (!onemhflag)
	jne	.onemh2
	mfaddx2	%f0,%f2			# h += temp
	jmp	.h2
.onemh2:
	mfsubx2	%f0,%f2			# onemh -= temp
.h2:
	addw2	&1,%r6
	cmpw	%r6,%r5
	jle	.L98
.L100:
.L104:
	mmovxx	%f2,632(%fp)		# frexp(h,&nbits)
	andw3	&0x7fff,632(%fp),%r0	# extract exponent
					# (in-line expansion of frexp)
					# don't worry about de-normals
					# or infinities
	subw2	&16382,%r0		# nbits = unbiased exp
	mnegw	%r0,%r0			# nbits = - nbits
	mulw3	&15,%r5,%r1		# if(nbits <= M*L-2*PRECISION-K)
	subw2	&110,%r1
	cmpw	%r0,%r1
	jle	.L94			# jump out of loop
					# no loss of signiicance
					# else calculate more bits
					# first find correct 15 bits of
					# 1/2pi
	addw3	&107,0(%fp),%r0		# m = k - PRECISION+PADDING*CHARSIZE
	mulw3	&15,%r5,%r1		
	addw2	%r1,%r0			# m += L * M
	modw3	&32,%r0,%r8		# mmod32 = m % 32
	divw3	&32,%r0,%r6		# mover32 = m /32
	alsw3	&2,%r6,%r0
	LLSW3	%r8,pihex(%r0),%r0	#val1= phihex[mover32]<<mmod32
	LRSW3	&17,%r0,%r0		# val1 >>= 32-L
	andw2	&32767,%r0		# val1 &= LMASK
	cmpw	%r8,&17			# if mmod32 <= 32-L
	jg	.L110
	movw	%r0,%r7			# gval = val1
	jmp	.L111
.L110:
	addw3	&1,%r6,%r1		# mover32 + 1
	alsw2	&2,%r1
	subw3	%r8,&49,%r2		# 64 - L - mmod32
	LRSW3	%r2,pihex(%r1),%r1   # val2= pihex[mov32+1]>>64-L-mmod32
	andw2	&32767,%r1		# val2 &= LMASK
	addw3	%r0,%r1,%r7		# gval = val1 + val2
.L111:
	addw2	&1,%r5			# M += 1
	movw	&0,%r8			# for(n=0;n<N;n++)
.L112:
	movaw	40(%fp),%r0
	alsw3	&2,%r8,%r1
	addw2	%r1,%r0
	mulw3	%r7,0(%r0),%r4	# prod = F[n] * gval
	movaw	56(%fp),%r2
	subw3	%r8,%r5,%r6	# for(l=M-n;(l>=0)&&(prod!=0);l--)
	jl	.L117
	cmpw	%r4,&0
	je	.L117
.L115:
	alsw3	&2,%r6,%r1
	addw3	%r1,%r2,%r0
	addw3	0(%r0),%r4,%r3		# tot = prod + partial[l]
	modw3	&32768,%r3,0(%r0)	# partial[l] = tot % TWOL
	divw3	&32768,%r3,%r4		# prod = tot/TWOL
	subw2	&1,%r6
	jl	.L117
	cmpw	%r4,&0
	jne	.L115
.L117:
	addw2	&1,%r8
	cmpw	%r8,&4
	jl	.L112
	jmp	.loop			# loop again through
					# test for loss of significance
					#
.L94:					# end of calculating h, onemeh
					# no further loss of significance
					# at this point %f2
					# is a double extended value
					# representing h or onemh
	mfmulx2	.M_PI_2,%f2	# multiply h or 1-h by pi/2
	cmpw	&0,4(%fp)		# if (sign) negate product
	je	.done
	mfnegx2	%f2,*16(%ap)
	ret	&.R1
.done:
	mmovxx	%f2,*16(%ap)
	ret	&.R1
	.text
