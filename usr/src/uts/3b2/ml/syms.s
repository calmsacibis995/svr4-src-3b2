#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


	.ident	"@(#)kernel:ml/syms.s	1.1"

#	The following symbols are defined here for ELF CCS.
#	ELF will not allow definition in mapfiles as we
#	would have done in COFF ifiles

	.section	.kvsdbint,"ax"

	.globl	unxsbdst
	.globl	mmusdc1
	.globl	mmusdc2
	.globl	mmupdc1r
	.globl	mmupdc2r
	.globl	mmupdc1l
	.globl	mmupdc2l
	.globl	mmusrama
	.globl	mmusramb
	.globl	mmufltcr
	.globl	mmufltar
	.globl	mmucr
	.globl	mmuvar
	.globl	sbdpit
	.globl	clrclkint
	.globl	sbdnvram
	.globl	sbdrcsr
	.globl	sbdwcsr
	.globl	dmaid
	.globl	dmaiuA
	.globl	dmaiuB
	.globl	dmac
	.globl	duart
	.globl	idisk
	.globl	ifloppy
	.globl	dmaif

	.set	unxsbdst,. 
	.set	mmusdc1	,. 
	.set	mmusdc2	,. + 0x100
	.set	mmupdc1r,. + 0x200
	.set	mmupdc2r,. + 0x300
	.set	mmupdc1l,. + 0x400
	.set	mmupdc2l,. + 0x500
	.set	mmusrama,. + 0x600
	.set	mmusramb,. + 0x700
	.set	mmufltcr,. + 0x800
	.set	mmufltar,. + 0x900
	.set	mmucr	,. + 0xa00
	.set	mmuvar	,. + 0xb00
	.set	sbdpit	,. + 0x2000
	.set	clrclkint,. + 0x2013
	.set	sbdnvram,. + 0x3000
	.set	sbdrcsr	,. + 0x4000
	.set	sbdwcsr	,. + 0x4000
	.set	dmaid	,. + 0x5000
	.set	dmaiuA	,. + 0x6000
	.set	dmaiuB	,. + 0x7000
	.set	dmac	,. + 0x8000
	.set	duart	,. + 0x9000
	.set	idisk	,. + 0xa000
	.set	ifloppy	,. + 0xd000
	.set	dmaif	,. + 0xe000

	.section	.kvsegmap,"ax"
	.globl	kvsegmap
	.set	kvsegmap,.

	.section	.kvsegu,"aw"
	.globl	kvsegu
	.set	kvsegu,.

	.section	.kvsysseg,"ax"
	.globl	syssegs
	.set	syssegs,.

	.section	.kvwindow,"ax"
	.globl	win_ublk
	.set	win_ublk,.

	.section	.uvstack,"awx"
	.globl	userstack
	.set	userstack,.

	.section	.uvblock,"aw"
	.globl	u
	.set	u,.
