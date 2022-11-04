	.file	"isnand.s"
.ident	"@(#)libc-m32:fp/isnand.s	1.7"
#	int isnand(srcD)
#	double srcD;
#
#	This routine returns 1 if the argument is a NaN
#		     returns 0 otherwise.
#
#	int isnan(srcD) 
#	double srcD;
#	-- functionality is same as isnand().

	.set	DMAX_EXP,0x7ff

	.text
	.align	4
_fwdef_(`isnand'):
_fwdef_(`isnan'):
	MCOUNT
	extzv	&20,&11,0(%ap),%r0
	cmpw	%r0,&DMAX_EXP		# if ( exp != 0x7ff )
	jne	.false			#	its not a NaN
	extzv	&0,&20,0(%ap),%r0
	orw2	4(%ap),%r0		# OR of all the fraction bits
	je	.false			# if ( fraction == 0 )
					#	its an infinity
	movw	&1,%r0
	RET
.false:
	movw	&0,%r0
	RET

