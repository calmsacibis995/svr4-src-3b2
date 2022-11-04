	.file	"isnanf.s"
	.ident	"@(#)libc-m32:fp/isnanf.s	1.4"
#	int isnanf(srcF)
#	float srcF;
#
#	This routine returns 1 if the argument is a NaN
#		     returns 0 otherwise.

	.set	FMAX_EXP,0xff

	.text
	.align	4
_fwdef_(`isnanf'):
	MCOUNT
	EXTFW	&7,&23,0(%ap),%r0	# exponent
	cmpw	%r0,&FMAX_EXP		# if ( exp != 0xff )
	jne	.false			#	its not a NaN
	EXTFW	&22,&0,0(%ap),%r0	# get fraction
	je	.false			# if ( fraction == 0 )
					#	its an infinity
	movw	&1,%r0
	RET
.false:
	movw	&0,%r0
	RET

