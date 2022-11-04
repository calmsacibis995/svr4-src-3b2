	.file	"sqrt.s"
	.ident	"@(#)libm:m32mau/sqrt.s	1.5"
##############################################################################
#	double sqrt(x)
#	double x;
##############################################################################
	.text
	.align	4
	.globl	sqrt
sqrt:
	MCOUNT
	mmovdd	0(%ap),%f0	# if ( x < 0.0 )
	jl	.exception	# 	exception
	mfsqrd2	%f0,0(%ap)	# else	sqare root
	movw	0(%ap),%r0
	movw	4(%ap),%r1
	RET			# return in %r0,%r1
#-----------------------------------------------------------------------------
	.set	.F1,40
	.text
.exception:
	save	&0
	addw2	&.F1,%sp
	mfsqrd2	%f0,0(%ap)	# raise exception
	movw	&1,4(%fp)	# exc.type = DOMAIN
	movw	&.sqrt,8(%fp)	# exc.name = "sqrt"
	mmovdd	%f0,12(%fp)	# exc.arg1 = x
	cmpw	_lib_version,&2 # if strict_ansi return NaN
	jne	.nonan
	movw	0(%ap),28(%fp)
	movw	4(%ap),32(%fp)
	jmp	.n2
.nonan:
	mmovdd	.d_zero,28(%fp)	# exc.retval = 0.0
.n2:
	cmpw	_lib_version,&2		# if (strict_ansi)
	jne	.nstrict
	movw	&33,errno
	jmp	.L55
.nstrict:
	pushaw	4(%fp)		# push &exc
	call	&1,matherr
	cmpw	%r0,&0		# if (!matherr(&exc) {
	jne	.L55
	cmpw	_lib_version,&0		# if (c_issue_4)
	jne	.nomess
	pushw	&2
	pushw	&.L57
	pushw	&19
	call	&3,_write	#	(void) write(2,"sqrt: DOMAIN error\n")
.nomess:
	movw	&33,errno		# errno = EDOM
.L55:				# }
	movw	28(%fp),%r0
	movw	32(%fp),%r1
	ret	&0		# return (exc.retval)

#-----------------------------------------------------------------------------
	.data
.sqrt:
	.byte	115,113,114,116,0	# "sqrt"
.L57:
	.byte	115,113,114,116,58,32,68,79,77,65
	.byte	73,78,32,101,114,114,111,114,10,0
	.align	4
.d_zero:
	.word 0,0

