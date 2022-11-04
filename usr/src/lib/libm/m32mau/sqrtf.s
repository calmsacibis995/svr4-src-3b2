	.file	"sqrtf.s"
	.ident	"@(#)libm:m32mau/sqrtf.s	1.5"
############################################################
#	float sqrtf(float x)
#
##########################################################
	.text
	.align	4
	.globl	sqrtf
sqrtf:
	MCOUNT
	mmovss	0(%ap),%s0	#if (x < 0.0)
	jl	.exception	#	exception
	mfsqrs2	%s0,0(%ap)	#else square root	
	movw	0(%ap),%r0	# return in %r0
	RET
#--------------------------------------------------------
	.set	.F1,40
	.text
.exception:
	save	&0
	addw2	&.F1,%sp
	mfsqrs2	%s0,0(%ap)	# raise exception
	movw	&1,4(%fp)	# exc.type = DOMAIN
	mmovsd	0(%ap),12(%fp)	# exc.arg1 = x
	movw	&.name,8(%fp)	#exc.name = "sqrtf"
	cmpw	_lib_version,&2	# if strict_ansi, return NaN
	jne	.nonan
	movw	&0x7ff80000,28(%fp)
	movw	&0,32(%fp)
	jmp	.n2
.nonan:
	movw	&0,28(%fp)	# exc.retval = 0.0
	movw	&0,32(%fp)
.n2:
	cmpw	_lib_version,&2	# if strict_ansi
	jne	.nstrict
	movw	&33,errno	# errno = EDOM
	jmp	.L43
.nstrict:
	pushaw	4(%fp)		# push &exc
	call	&1,matherr
	cmpw	%r0,&0
	jne	.L43		# if (!matherrr(&exc))
	cmpw	_lib_version,&0	# if (c_issue_4)
	jne	.nomess
	pushw	&2		# (void)write(2,"sqrtf: DOMAIN error\n",20);
	pushw	&.errmess
	pushw	&20
	call	&3,_write
.nomess:
	movw	&33,errno
.L43:
	mmovds	28(%fp),36(%fp)
	movw	36(%fp),%r0
	ret	&0
	.data
	.align	4
.name:
	.byte	115,113,114,116,102,0
	.align	4
.errmess:
	.byte	115,113,114,116,102,58,32,68,79,77,65
	.byte	73,78,32,101,114,114,111,114,10,0
	.text
