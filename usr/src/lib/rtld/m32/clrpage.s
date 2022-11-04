	.file	"clrpage.s"
	.ident	"@(#)rtld:m32/clrpage.s	1.1"

# _clrpage(dst, cnt)
#	Fast assembly routine to zero page.
#	Sets cnt bytes to zero, starting at dst
#	Dst and cnt must be coordinated to give word alignment
#		(guaranteed if used to zero a page)

	.set	.roll, 64
	.globl	_clrpage
	.type	_clrpage,@function
	.text
	.align	4
_clrpage:
	movw	0(%ap), %r0
	movw	4(%ap), %r1
	jz	.done
	BITW	&.roll-1, %r1	# check whether count aligned
	jz	.loop
	BITW	&1, %r1
	jz	.2
	CLRB	0(%r0)
	INCW	%r0
.2:
	BITW	&2, %r1
	jz	.4
	CLRH	0(%r0)
	ADDW2	&2, %r0
.4:
	BITW	&4, %r1
	jz	.8
	CLRW	0(%r0)
	addw2	&4, %r0
.8:
	BITW	&8, %r1
	jz	.16
	CLRW	0(%r0)
	CLRW	4(%r0)
	addw2	&8, %r0
.16:
	BITW	&16, %r1
	jz	.32
	CLRW	0(%r0)
	CLRW	4(%r0)
	CLRW	8(%r0)
	CLRW	12(%r0)
	addw2	&16, %r0
.32:
	BITW	&32, %r1
	jz	.64
	CLRW	0(%r0)
	CLRW	4(%r0)
	CLRW	8(%r0)
	CLRW	12(%r0)
	CLRW	16(%r0)
	CLRW	20(%r0)
	CLRW	24(%r0)
	CLRW	28(%r0)
	addw2	&32, %r0
.64:
	andw2	&-.roll, %r1
	jle	.done
	.align	4
.loop:
	CLRW	0(%r0)
	CLRW	4(%r0)
	CLRW	8(%r0)
	CLRW	12(%r0)
	CLRW	16(%r0)
	CLRW	20(%r0)
	CLRW	24(%r0)
	CLRW	28(%r0)
	CLRW	32(%r0)
	CLRW	36(%r0)
	CLRW	40(%r0)
	CLRW	44(%r0)
	CLRW	48(%r0)
	CLRW	52(%r0)
	CLRW	56(%r0)
	CLRW	60(%r0)
	addw2	&.roll, %r0
	addw2	&-.roll, %r1
	jg	.loop
.done:
	RET
	.size	_clrpage,.-_clrpage
