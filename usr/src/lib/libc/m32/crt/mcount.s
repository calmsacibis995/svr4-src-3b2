# count subroutine called during profiling
.ident	"@(#)libc-m32:crt/mcount.s	1.7"
#
# calling sequence:	MOVW	&a_word,%r0
#			JSB	_mcount
#			.data
#		a_word:	.word	0
#

	.globl	_mcount
	.globl	countbase		# Next pc,count cell available.
	.globl	_countlimit		# Cells End Here (points after last)


_fgdef_(_mcount):
	MOVW	0(%r0),%r1		# a_word to %r1
	jne	.add1			# skip initialization if not zero

	MOVW	_dref_(countbase),%r1	# get stack base in %r1
	REQL				# return if counting not turned on yet

	ADDW2	&8,_dref_(countbase)	# allocate a new pc,count pair
	MOVW	-4(%sp),0(%r1)		# remember pc
	ADDW2	&4,%r1
	MOVW	&0,0(%r1)		# init call count to zero
	MOVW	%r1,0(%r0)		# store &pair+4 in a_word

	CMPW	_dref_(_countlimit),_dref_(countbase)	# if yet more cells are available..
	jlu	.yetmore		# then stick with what we have.

	PUSHW	%r1			# No Mo Cells; preserve a_word,
	call	&0,_fref_(_mnewblock)	# get more cells (a block of 'em),
	POPW	%r1			# and restore a_word agin.

.yetmore:				# ok: countbase,limit set for block.

.add1:
	INCW	0(%r1)			# count function call
	RSB
