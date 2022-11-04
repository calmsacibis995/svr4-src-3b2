	.file	"fpgetrnd.s"
.ident	"@(#)libc-m32:fp/fpgetrnd.s	1.11"
	.text
#----------------------------------
	.globl	_fp_hw		# to check hardware present
	.globl	_asr		# simulated arithmetic status register
	.globl	_fpstart	# to force floating point startup
#----------------------------------
	.align	4
_fwdef_(`fpgetround'):
	MCOUNT
	TSTW	_dref_(_fp_hw)
	je	.soft
#----------------------------------
# MAU
	save	&0
	addw2	&4,%sp
	mmovfa	0(%fp)
	extzv	&22,&2,0(%fp),%r0
	ret	&0
#----------------------------------
.soft:					# running software
	extzv	&22,&2,_dref_(_asr),%r0
	RET
##############################################################################

