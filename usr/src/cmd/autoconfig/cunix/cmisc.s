#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

.ident	"@(#)autoconfig:cunix/cmisc.s	1.2"

#
#	min() and max() routines for signed and unsigned arithmetic
#

#	signed min()
#
	.globl	min
min:
	MOVW	0(%ap),%r0
	CMPW	%r0,4(%ap)
	BGEB	minxit
	MOVW	4(%ap),%r0
minxit:
	RET


#	signed max()
#
	.globl	max
max:
	MOVW	0(%ap),%r0
	CMPW	%r0,4(%ap)
	BLEB	maxit
	MOVW	4(%ap),%r0
maxit:
	RET

#	unsigned min()
#
	.globl	umin
umin:
	MOVW	0(%ap),%r0
	CMPW	%r0,4(%ap)
	BGEUB	uminxit
	MOVW	4(%ap),%r0
uminxit:
	RET


#	unsigned max()
#
	.globl	umax
umax:
	MOVW	0(%ap),%r0
	CMPW	%r0,4(%ap)
	BLEUB	umaxit
	MOVW	4(%ap),%r0
umaxit:
	RET




#
#	This is the block copy routine.
#
#		bcopy(from, to, count)
#		caddr_t from, to;
#		{
#			while( count-- > 0 )
#				*to++ = *from++;
#		}
#

	.globl	bcopy

bcopy:
	SAVE	%r6

	MOVW	0(%ap),%r0		# from-address to %r0
	MOVW	4(%ap),%r1		# to-address to %r1
	MOVW	8(%ap),%r6		# byte count to %r6
 	je	bcret

	ANDW3	%r0,&0x03,%r7		# get alignment of addresses
	ANDW3	%r1,&0x03,%r8

	SUBW3	%r7,%r8,%r2
	je	bcalw			# word aligned addresses
	BITW	&0x01,%r2
	je	bcalh			# half-word aligned addresses

#
#	byte-aligned addresses; just copy a byte at a time
#
bcalb:
	MOVB	0(%r0),0(%r1)		# copy bytes
	INCW	%r0
	INCW	%r1
	DECW	%r6
	jne	bcalb
	jmp	bcret

#
#	half-word aligned addresses; copy a half-word at a time
#
bcalh:
	BITW	&0x01,%r0		# copy any initial odd-aligned byte
	je	bcalh1

	MOVB	0(%r0),0(%r1)
	INCW	%r0
	INCW	%r1
	DECW	%r6
	je	bcret

bcalh1:
	LRSW3	&1,%r6,%r2		# number of half-words
	je	bcalh2

bcalhmv:
	MOVH	0(%r0),0(%r1)		# copy half-words
	ADDW2	&2,%r0
	ADDW2	&2,%r1
	DECW	%r2
	jne	bcalhmv

	BITW	&0x01,%r6		# copy any trailing odd-aligned byte
	je	bcret

bcalh2:
	MOVB	0(%r0),0(%r1)
	jmp	bcret

#
#	word aligned addresses; use block move instruction
#
bcalw:
	BITW	&0x03,%r0		# copy any initial unaligned bytes
	je	bcalw1

	MOVB	0(%r0),0(%r1)
	INCW	%r0
	INCW	%r1
	DECW	%r6
	jne	bcalw
	jmp	bcret

bcalw1:
	LRSW3	&2,%r6,%r2		# number of words
	je	bcalw2

	MOVBLW				# copy words

	ANDW2	&0x03,%r6		# bytes remaining to be copied
	je	bcret

bcalw2:
	MOVB	0(%r0),0(%r1)		# copy any trailing unaligned bytes
	INCW	%r0
	INCW	%r1
	DECW	%r6
	jne	bcalw2
	
bcret:
	RESTORE	%r6
	RET


