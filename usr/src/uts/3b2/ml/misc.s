#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kernel:ml/misc.s	1.23"

#	The following sets are used in icode.

	.set	u_pcb, 0xc	# Offset to u_pcb in u-block.
	.set	u_pcbp, 0x6c	# Offset to u_pcbp in u-block.
	.set	slb, 0xc	# Offset to slb in pcb.
	.set	u_stack, 0x530	# Offset to u_stack in u-block.
	.set	u_caddrflt,0x130 # Offset to u_caddrflt in u-block.
	.set	u_procp, 0x4e4	# Offset to u_procp in u-block.
	.set	u_flist, 0x6e4	# Offset to u_flist in u-block.
	.set	p_sysid, 0x88	# Offset to p_sysid in proc table.

	.text
#
#	addupc( pc, &u.u_prof, tics )
#

	.globl	addupc

#
#   NOTE -- CHANGES TO USER.H MAY REQUIRE CORRESPONDING CHANGES TO THE
#		FOLLOWING .SETs.
#
	.set	pr_base,0	# buffer base, short *
	.set	pr_size,4	# buffer size, unsigned int
	.set	pr_off,8	# pc offset, unsigned int
	.set	pr_scale,12	# pc scaling, unsigned int

addupc:
	MOVW	4(%ap),%r2		# &u.u_prof
	SUBW3	pr_off(%r2),0(%ap),%r0	# corrected pc
	jlu	addret2
	LRSW3	&1,%r0,%r0		# PC >> 1
	LRSW3	&1,pr_scale(%r2),%r1	# scale >> 1
	MULW2	{uword}%r1,%r0
	LRSW3	&14,%r0,%r0
	INCW	%r0
	ANDW2	&0XFFFFFFFE,%r0
	CMPW	%r0,pr_size(%r2)	# length
	jleu	addret2
	ADDW2	pr_base(%r2),%r0	# base
	MOVAW	adderr,u+u_caddrflt
	ADDW2	8(%ap),{shalf}0(%r0)
addret1:
	CLRW	u+u_caddrflt
addret2:
	RET
adderr:
	CLRW	pr_scale(%r2)
	jmp	addret1


#
#	SPL - Set interrupt priority level routines
#

	.globl	spl1
spl1:
	MOVW	%psw,%r0
	INSFW	&3,&13,&8,%psw
	RET

	.globl	spl4
spl4:
	MOVW	%psw,%r0
	INSFW	&3,&13,&10,%psw
	RET

	.globl	spl5
spl5:
	MOVW	%psw,%r0
	INSFW	&3,&13,&10,%psw		# device interrupts
	RET

	.globl	splpp
splpp:
	MOVW	%psw,%r0
	INSFW	&3,&13,&10,%psw
	RET

	.globl	spl6
spl6:
	MOVW	%psw,%r0
	INSFW	&3,&13,&12,%psw
	RET

	.globl	splvm
splvm:
	MOVW	%psw,%r0
	INSFW	&3,&13,&12,%psw
	RET

	.globl	splni
splni:
	MOVW	%psw,%r0
	INSFW	&3,&13,&12,%psw
	RET

	.globl	splimp
splimp:
	MOVW	%psw,%r0
	INSFW	&3,&13,&13,%psw
	RET

	.globl	spltty
spltty:
	MOVW	%psw,%r0
	INSFW	&3,&13,&13,%psw
	RET

	.globl	spl7
spl7:
	MOVW	%psw,%r0
	ORW2	&0x1e000,%psw
	RET

	.globl	splhi
splhi:
	MOVW	%psw,%r0
	ORW2	&0x1e000,%psw
	RET

	.globl	spl0
spl0:
	MOVW	%psw,%r0
	ANDW2	&0xfffe1fff,%psw
	RET

	.globl	splx
splx:
	MOVW	%psw,%r0
	MOVW	0(%ap),%psw
	RET


#
# This routine establishes the call frames for the virtual portion
# of the UNIX boot and calls mlsetup(), passing it memory managemnet
# information as parameters.
#
# Register 8 contains the first free click,set by pstart.
# Register 7 contains the autoconfig flag passed by lboot, 
# 	that is used to determine when auto configuration was done
#	by lboot.

	.globl	vstart_s
	.globl	mlsetup
	.globl	cmn_err
	.globl	u
	.globl	v
	.globl	u400
	.globl  sys3bautoconfig

vstart_s:
	MOVW	%r7,sys3bautoconfig
	MOVW	&u+u_flist,%r1		# Last entry in user area.
	ADDW2	&124,%r1		# Size of struct ufchunk.
	ADDW2	&8,%r1			# First entry in kernel stack.
	ADDW2	&3,%r1			# Round up to work boundary to
	ANDW2	&-4,%r1			# get ptr to start of stack.
	MOVW	%r1,u+u_stack		# Save start of kernel stack.
	MOVW	%r1,%sp			# Load the stack pointer.
	MOVW	%sp,%fp			# Set frame pointer.
	MOVW	%sp,%ap			# Set argument pointer.
	MOVW	%sp,u_slb(%pcbp)	# Set current stack lower bound.
	PUSHW	%r8			# Physclick.
	CALL	-4(%sp),mlsetup
	TSTB	u400
	BEB	L1_300
	ANDW2	&0x20104,%psw
	BRB	L1_400

L1_300:
	ANDW2	&0x2820104,%psw
	ORW2	&0x2800000,%psw

L1_400:
	CALL	0(%sp),main

#
#	Go to user process,

# 	BITW	&0x80000000,%r0
#	BEB	krnlmode
	PUSHW	%r0
	TSTB	u400
	BEB	L2_300
	PUSHW	&0x0700
	CFLUSH
	RETG

L2_300:
	PUSHW	&0x2800700
	RETG

#krnlmode:
#	CALL	0(%sp),0(%r0)
#	PUSHW	&3
#	PUSHAW	message
#	CALL	-(2*4)(%sp),cmn_err
#	jmp	.

#	.data
#message:	#Illegal return from main()
#	.byte	0x49,0x6c,0x6c,0x65,0x67,0x61,0x6c,0x20,0x72,0x65
#	.byte	0x74,0x75,0x72,0x6e,0x20,0x66,0x72,0x6f,0x6d,0x20
#	.byte	0x6d,0x61,0x69,0x6e,0x28,0x29,0
	.text


#
# This is the system wait routine
#
	.globl	idle
idle:
	INSFW	&3,&13,&8,%psw	# Enable all but swtch() interrupts.
	WAIT
_waitloc:
	RET

	.data

	.globl	waitloc
	.align 4
waitloc:
	.word	_waitloc

	.text


#
#	min() and max() routines
#

	.globl	min
min:
	MOVW	0(%ap),%r0
	CMPW	%r0,4(%ap)
	jgeu	minxit
	MOVW	4(%ap),%r0
minxit:
	RET


	.globl	max
max:
	MOVW	0(%ap),%r0
	CMPW	%r0,4(%ap)
	jleu	maxit
	MOVW	4(%ap),%r0
maxit:
	RET

#
#	rtrue() and rfalse() routines supported by self-config
#

	.globl rtrue
rtrue:
	MOVW	&1,%r0
	RET

	.globl rfalse
rfalse:
	CLRW	%r0
	RET

#
#	bzero(addr, len)
#
#	This is	the block zero routine; arguments are the
#	starting address and length.
#
#	struct_zero() was added to catch any references to the fast inline
#	structure zeroing routine that, for some reason, could not be put
#	inline. An example for this is that the file could not include
#	sys/inline.h .
#
#	bzeroba() was removed because bzero() was modified to supply the
#	same functionality; i.e. the word alignment and word number of
#	bytes restrictions were removed from bzero(). The name was kept
#	for compatibility.
#
#	kzero() is for compatibility with the VM code.
#
	.globl	bzero
	.globl	struct_zero
	.globl	bzeroba
	.globl	kzero

bzero:
struct_zero:
bzeroba:
kzero:
	PUSHW	%r3
	PUSHW	u+u_caddrflt		# save current fault handler
	MOVAW	bzeroflt,u+u_caddrflt	# set up ours
	MOVW	0(%ap),%r1		# %r1 = addr
	MOVW	4(%ap),%r0		# %r0 = len
	BEH	bzdone
bzalgn:					# align addr to word boundary
	BITW	&3,%r1			# if (addr is word aligned)
	BEB	bzwrds			#	goto word zero section;
	CLRB	0(%r1)			# *addr = 0;
	INCW	%r1			# ++addr;
	DECW	%r0			# --len;
	BEH	bzdone			# if (len == 0) return
	JMP	bzalgn			# loop until addr is word aligned
bzwrds:
	ANDW3	&0x7c,%r0,%r3		# if ((len & 3) is a multiple of 128)
	BEB	bz128m			#	goto bz128m
	SUBW3	%r3,&128,%r3		# skip = bytes to skip on first iter
	SUBW2	%r3,%r1			# adjust addr for offsets in loop
	LRSW3	&1,%r3,%r3		# %r3 = skip / 2 = 2 * number_of_words
	LRSW3	&1,%r3,%r2		# %r2 = (2*nwds) / 2 = nwds
	ADDW2	%r2,%r3			# jmp_offset = (2*nwds) + nwds = 3*nwds
	LRSW3	&7,%r0,%r2		# iter = len / 128
	INCW	%r2			# +1 needed for partial trip thru loop
	JMP	bzloop-1(%r3)		# -1 because first CLRW is only 2 bytes
bz128m:
	LRSW3	&7,%r0,%r2		# iter = len / 128
	BEH	bzclrb			# if (iter == 0) goto bzclrb
bzloop:
	CLRW	0(%r1)			# *((int *)(addr + 0)) = 0
	CLRW	4(%r1)			# *((int *)(addr + 4)) = 0
	CLRW	8(%r1)
	CLRW	12(%r1)
	CLRW	16(%r1)
	CLRW	20(%r1)
	CLRW	24(%r1)
	CLRW	28(%r1)
	CLRW	32(%r1)
	CLRW	36(%r1)
	CLRW	40(%r1)
	CLRW	44(%r1)
	CLRW	48(%r1)
	CLRW	52(%r1)
	CLRW	56(%r1)
	CLRW	60(%r1)
	CLRW	64(%r1)
	CLRW	68(%r1)
	CLRW	72(%r1)
	CLRW	76(%r1)
	CLRW	80(%r1)
	CLRW	84(%r1)
	CLRW	88(%r1)
	CLRW	92(%r1)
	CLRW	96(%r1)
	CLRW	100(%r1)
	CLRW	104(%r1)
	CLRW	108(%r1)
	CLRW	112(%r1)
	CLRW	116(%r1)
	CLRW	120(%r1)
	CLRW	124(%r1)
	MOVAW	128(%r1),%r1		# addr += 128 bytes
	DECW	%r2			# --iter
	BNEB	bzloop			# loop until no more words to clear
	ANDW2	&3,%r0			# len = number of bytes left to zero
	BEB	bzdone			# if (len == 0) return
bzclrb:
	CLRB	0(%r1)			# *addr = 0
	INCW	%r1			# ++addr
	DECW	%r0			# --len
	BNEB	bzclrb			# loop until no bytes left to clear
bzdone:
	POPW	u+u_caddrflt		# restore handler bzero inherited
	POPW	%r3
	RET				# %r0 = len = 0

# return an error

bzeroflt:
	POPW	u+u_caddrflt		# restore handler bzero inherited
	POPW	%r3
	MOVW	&-1,%r0
	RET

#
#	This is the block copy routine.
#
#		bcopy(from, to, bytes)
#		caddr_t from, to;
#		long bytes;
#		{
#			while( bytes-- > 0 )
#				*to++ = *from++;
#		}
#

	.globl	bcopy

bcopy:
	SAVE	%r6

	MOVW	0(%ap),%r0		# from-address to %r0
	MOVW	4(%ap),%r1		# to-address to %r1
	MOVW	8(%ap),%r6		# byte count to %r6
 	BEH	bcret

	ANDW3	%r0,&0x03,%r7		# get alignment of addresses
	ANDW3	%r1,&0x03,%r8
	BITB	&3,%r6			# check	if count is multiple of 4
	BNEB	bcopygen
	ORW3	%r7,%r8,%r2		# check both addresses word aligned
	BEH	bcopyblk
bcopygen:
	SUBW3	%r7,%r8,%r2
	BEH	bcalw			# addresses aligned modulo 4
	BITW	&0x01,%r2
	BEH	bcalh			# addresses aligned modulo 2

#
#	byte-aligned addresses;	copy 32 bytes at a time
#

	CMPW	&0x20, %r6		# if copying less than 32 bytes,
	BLH	bcwrap			# go to the copy wrap up code

#
#	Note: we could use the .byte statements (see fbclop)
#	to avoid SGS NOP generation because bcopy is strictly
#	kernel to kernel and we know we will not get any faults.
#	However, the next SGS will not insert NOPs, so we will 
#	wait for it.
#

bcalb:
	MOVB	0x0(%r0),0x0(%r1)	# unrolled loop to copy 32 bytes
	MOVB	0x1(%r0),0x1(%r1)
	MOVB	0x2(%r0),0x2(%r1)
	MOVB	0x3(%r0),0x3(%r1)
	MOVB	0x4(%r0),0x4(%r1)
	MOVB	0x5(%r0),0x5(%r1)
	MOVB	0x6(%r0),0x6(%r1)
	MOVB	0x7(%r0),0x7(%r1)
	MOVB	0x8(%r0),0x8(%r1)
	MOVB	0x9(%r0),0x9(%r1)
	MOVB	0xa(%r0),0xa(%r1)
	MOVB	0xb(%r0),0xb(%r1)
	MOVB	0xc(%r0),0xc(%r1)
	MOVB	0xd(%r0),0xd(%r1)
	MOVB	0xe(%r0),0xe(%r1)
	MOVB	0xf(%r0),0xf(%r1)
	MOVB	0x10(%r0),0x10(%r1)
	MOVB	0x11(%r0),0x11(%r1)
	MOVB	0x12(%r0),0x12(%r1)
	MOVB	0x13(%r0),0x13(%r1)
	MOVB	0x14(%r0),0x14(%r1)
	MOVB	0x15(%r0),0x15(%r1)
	MOVB	0x16(%r0),0x16(%r1)
	MOVB	0x17(%r0),0x17(%r1)
	MOVB	0x18(%r0),0x18(%r1)
	MOVB	0x19(%r0),0x19(%r1)
	MOVB	0x1a(%r0),0x1a(%r1)
	MOVB	0x1b(%r0),0x1b(%r1)
	MOVB	0x1c(%r0),0x1c(%r1)
	MOVB	0x1d(%r0),0x1d(%r1)
	MOVB	0x1e(%r0),0x1e(%r1)
	MOVB	0x1f(%r0),0x1f(%r1)
	ADDW2	&0x20, %r0		# increment the from pointer
	ADDW2	&0x20, %r1		# increment the to pointer
	SUBW2	&0x20, %r6		# decrement the count
	BLEH	bcret			# return if count is 0
	CMPW	&0x20, %r6		# if at least 32 bytes left,
	BGEH	bcalb			# go to start of unrolled loop

bcwrap:					# copy wrap up code (less than 32)
	MOVB	0(%r0),0(%r1)		# copy byte by byte
	INCW	%r0
	INCW	%r1
	DECW	%r6
	BNEB	bcwrap
	jmp	bcret

#
#	half-word aligned addresses; copy 16 half-words at a time
#

bcalh:
	BITW	&0x01,%r0		# copy any initial odd-aligned byte
	BEB	bcalh1

	MOVB	0(%r0),0(%r1)
	INCW	%r0
	INCW	%r1
	DECW	%r6
	BEH	bcret

bcalh1:
	LRSW3	&1,%r6,%r2		# number of half-words
	BEH	bcalh2

	CMPW	&0x10, %r2		# if less than 16 halfwords
	BLH	bcalhwrap1		# go to copy wrap up code

bcalhcopy:
	MOVH	0x0(%r0),0x0(%r1)	# unrolled loop to copy 32 bytes
	MOVH	0x2(%r0),0x2(%r1)
	MOVH	0x4(%r0),0x4(%r1)
	MOVH	0x6(%r0),0x6(%r1)
	MOVH	0x8(%r0),0x8(%r1)
	MOVH	0xa(%r0),0xa(%r1)
	MOVH	0xc(%r0),0xc(%r1)
	MOVH	0xe(%r0),0xe(%r1)
	MOVH	0x10(%r0),0x10(%r1)
	MOVH	0x12(%r0),0x12(%r1)
	MOVH	0x14(%r0),0x14(%r1)
	MOVH	0x16(%r0),0x16(%r1)
	MOVH	0x18(%r0),0x18(%r1)
	MOVH	0x1a(%r0),0x1a(%r1)
	MOVH	0x1c(%r0),0x1c(%r1)
	MOVH	0x1e(%r0),0x1e(%r1)
	ADDW2	&0x20, %r0		# increment the from pointer
	ADDW2	&0x20, %r1		# increment the to pointer
	SUBW2	&0x10, %r2		# decrement the count (of halfwords)
	BLEH	bcalhwrap2		# if no halfwords left, go to wrap up
	CMPW	&0x10, %r2		# if at least 16 halfwords left,
	BGEH	bcalhcopy		# go to top of unrolled loop

bcalhwrap1:				# wrap up copy of remaining halfwords
	MOVH	0(%r0),0(%r1)		# copy halfword by halfword
	ADDW2	&2,%r0
	ADDW2	&2,%r1
	DECW	%r2
	BNEB	bcalhwrap1

bcalhwrap2:
	BITW	&0x01,%r6		# copy any trailing odd-aligned byte
	BEH	bcret

bcalh2:
	MOVB	0(%r0),0(%r1)
	jmp	bcret

#
#	word aligned addresses; use block move instruction
#
bcalw:
	BITW	&0x03,%r0		# copy any initial unaligned bytes
	BEB	bcalw1

	MOVB	0(%r0),0(%r1)
	INCW	%r0
	INCW	%r1
	DECW	%r6
	BNEB	bcalw
	jmp	bcret

bcalw1:
	LRSW3	&2,%r6,%r2		# number of words
	BEB	bcalw2

	MOVBLW				# copy words

	ANDW2	&0x03,%r6		# bytes remaining to be copied
	BEB	bcret

bcalw2:
	MOVB	0(%r0),0(%r1)		# copy any trailing unaligned bytes
	INCW	%r0
	INCW	%r1
	DECW	%r6
	BNEB	bcalw2
	jmp	bcret

#
#	word aligned addresses and count; copy 128 byte blocks
#
bcopyblk:
	ARSW3	&2,%r6,%r2		# get size in words
	ANDW2	&0x1f,%r2		# move enough to get to 128 byte mult.
	BEB	bcybmain
	MOVBLW
bcybmain:
	ARSW3	&7,%r6,%r2		# no. of 128 byte blocks
	BEB	bcret
	JSB	fbclop			# copy remainder
bcret:
	CLRW	%r0			# return success flag
#					# can fail for rfs
	RESTORE	%r6
	RET


#
#	fbcopy(from, to, n)
#
#	Fast memory to memory copy of n clicks.  Both from
#	and to addresses assumed to be word aligned.
#

	.globl	fbcopy

fbcopy:
	MOVW	0(%ap),%r0	# from address
	MOVW	4(%ap),%r1	# to address
	ALSW3	&4,8(%ap),%r2	# loop count=clicks * 16
	JSB	fbclop
	RET


#
#	Fast block copy subroutine.
#
#	%r0 = from address
#	%r1 = to address
#	%r2 = no. 128 byte blocks to be copied
#

	.globl	fbclop

fbclop:

#	The following move instructions are coded as text
#	to avoid NOP generation from SGS.  NOPs are not
#	necessary since interrupts are blocked.

	.text
fbclopstrt:
	.byte	0x84,0x50,0x51		# MOVW	0(%r0),0(%r1)
	NOP				# in case a fault is taken
	NOP				# (padding)
	.byte	0x84,0xc0,0x04,0xc1,0x04	# MOVW	4(%r0),4(%r1)
	.byte	0x84,0xc0,0x08,0xc1,0x08	# MOVW	8(%r0),8(%r1)
	.byte	0x84,0xc0,0x0c,0xc1,0x0c	# MOVW	12(%r0),12(%r1)
	.byte	0x84,0xc0,0x10,0xc1,0x10	# MOVW	16(%r0),16(%r1)
	.byte	0x84,0xc0,0x14,0xc1,0x14	# MOVW	20(%r0),20(%r1)
	.byte	0x84,0xc0,0x18,0xc1,0x18	# MOVW	24(%r0),24(%r1)
	.byte	0x84,0xc0,0x1c,0xc1,0x1c	# MOVW	28(%r0),28(%r1)
	.byte	0x84,0xc0,0x20,0xc1,0x20	# MOVW	32(%r0),32(%r1)
	.byte	0x84,0xc0,0x24,0xc1,0x24	# MOVW	36(%r0),36(%r1)
	.byte	0x84,0xc0,0x28,0xc1,0x28	# MOVW	40(%r0),40(%r1)
	.byte	0x84,0xc0,0x2c,0xc1,0x2c	# MOVW	44(%r0),44(%r1)
	.byte	0x84,0xc0,0x30,0xc1,0x30	# MOVW	48(%r0),48(%r1)
	.byte	0x84,0xc0,0x34,0xc1,0x34	# MOVW	52(%r0),52(%r1)
	.byte	0x84,0xc0,0x38,0xc1,0x38	# MOVW	56(%r0),56(%r1)
	.byte	0x84,0xc0,0x3c,0xc1,0x3c	# MOVW	60(%r0),60(%r1)
	.byte	0x84,0xc0,0x40,0xc1,0x40	# MOVW	64(%r0),64(%r1)
	.byte	0x84,0xc0,0x44,0xc1,0x44	# MOVW	68(%r0),68(%r1)
	.byte	0x84,0xc0,0x48,0xc1,0x48	# MOVW	72(%r0),72(%r1)
	.byte	0x84,0xc0,0x4c,0xc1,0x4c	# MOVW	76(%r0),76(%r1)
	.byte	0x84,0xc0,0x50,0xc1,0x50	# MOVW	80(%r0),80(%r1)
	.byte	0x84,0xc0,0x54,0xc1,0x54	# MOVW	84(%r0),84(%r1)
	.byte	0x84,0xc0,0x58,0xc1,0x58	# MOVW	88(%r0),88(%r1)
	.byte	0x84,0xc0,0x5c,0xc1,0x5c	# MOVW	92(%r0),92(%r1)
	.byte	0x84,0xc0,0x60,0xc1,0x60	# MOVW	96(%r0),96(%r1)
	.byte	0x84,0xc0,0x64,0xc1,0x64	# MOVW	100(%r0),100(%r1)
	.byte	0x84,0xc0,0x68,0xc1,0x68	# MOVW	104(%r0),104(%r1)
	.byte	0x84,0xc0,0x6c,0xc1,0x6c	# MOVW	108(%r0),108(%r1)
	.byte	0x84,0xc0,0x70,0xc1,0x70	# MOVW	112(%r0),112(%r1)
	.byte	0x84,0xc0,0x74,0xc1,0x74	# MOVW	116(%r0),116(%r1)
	.byte	0x84,0xc0,0x78,0xc1,0x78	# MOVW	120(%r0),120(%r1)
	.byte	0x84,0xc0,0x7c,0xc1,0x7c	# MOVW	124(%r0),124(%r1)
	ADDW2	&128,%r0
	ADDW2	&128,%r1
	DECW	%r2
	BGH	fbclop
fbcdon:
	RSB



#
#	This is the copyin and copyout routine.
#
#	copyin(from,to,count)
#	caddr_t from, to;
#	long count;
#	{
#
#		if
#		(
#			from < MINUVTXT
#			||
#			UVUBLK < from+count  &&  from < UVSTACK
#		)
#			return(-1);
#
#	label:
#		if (caddrflt) {
#			caddrflt = 0;
#			return(-1);
#		}
#
#		caddrflt = label;
#		bcopy( from, to, count );
#		caddrflt = 0;
#		return(0);
#	}
#
#	copyout(from,to,count)
#	caddr_t from, to;
#	long count;
#	{
#
#		if
#		(
#			to < MINUVTXT
#			||
#			UVUBLK < to+count  &&  to < UVSTACK
#		)
#			return(-1);
#
#	label:
#		if (caddrflt) {
#			caddrflt = 0;
#			return(-1);
#		}
#
#		caddrflt = label;
#		bcopy( from, to, count );
#		caddrflt = 0;
#		return(0);
#	}
#
#	lcopyin() and lcopyout() are local-only versions of the same
#	routines, i.e. they are guaranteed to refer to virtual addresses
#	on the local machine and NOT to do the "if (RF_SERVER())" check
#	done by the regular copyin()/copyout().
#

	.globl	copyin
	.globl	copyout
	.globl	lcopyin
	.globl	lcopyout
	.globl	u

#
#   NOTE -- CHANGES TO USER.H MAY REQUIRE CORRESPONDING CHANGES TO THE
#		FOLLOWING .SETs.
#
	.set	UVTEXT,0x80800000	# main store beginning virtual address
	.set	UVUBLK,0xC0000000	# ublock virtual address
	.set	UVSTACK,0xC0020000	# stack start virtual address
	.set	MINUVTXT,0x80000000	# Minimum user virtual text.
	
copyin:
	SAVE	%r5
	MOVW	&0,%r5			# r0 contains the user address
	MOVW	u+u_procp,%r0		# check if the process is an RFS server
	CMPH	p_sysid(%r0),&0		# true if u.u_procp->p_sysid != 0
	je 	ci_kern
	PUSHW	0(%ap)
	PUSHW	4(%ap)
	PUSHW	8(%ap)
	PUSHW	&0			# !explicit
	call	&4,rcopyin
	RESTORE	%r5
	RET
#					# check end-points of copy
lcopyin:
	SAVE	%r5
	MOVW	&0,%r5			# r0 contains the user address

ci_kern:				# kernel mem
	CMPW	&MINUVTXT,0(%ap)
	BLUH	copybad
ci_ublk:				# ublock
	ADDW3	{uword}0(%ap),{uword}8(%ap),%r0
	CMPW	%r0,&UVUBLK
	BGEUH	copycom
	CMPW	&UVSTACK,0(%ap)
	BLUB	copybad
	JMP	copycom

copyout:
	SAVE	%r5
	MOVW	&1,%r5			# r1 contains the user address
	MOVW	u+u_procp,%r0		# check if the process is an RFS server
	CMPH	p_sysid(%r0),&0		# true if u.u_procp->p_sysid != 0
	je 	co_kern
	PUSHW	0(%ap)
	PUSHW	4(%ap)
	PUSHW	8(%ap)
	PUSHW	&0			# (struct msgb **)0
	call	&4,rcopyout
	RESTORE	%r5
	RET
lcopyout:
	SAVE	%r5
	MOVW	&1,%r5			# r1 contains the user address

#					# check end-points of copy
co_kern:				# kernel memory
	CMPW	&MINUVTXT,4(%ap)
	BLUB	copybad
co_ublk:				# ublock
	ADDW3	{uword}4(%ap),{uword}8(%ap),%r0	
	CMPW	%r0,&UVUBLK
	BGEUB	copycom
	CMPW	&UVSTACK,4(%ap)
	BLUB	copybad
	JMP	copycom

#
#	If a paging fault happens in fbclop, control is returned here.
#
fbclopflt:
	POPW	%r0		# get the saved pc off the stack
#
#	If a paging fault happens in copycom, control is returned here.
#
copyfault:
	CLRW	u+u_caddrflt
copybad:
	MOVW	&-1,%r0			# return failure flag
	RESTORE	%r5
	RET

copycom:
	MOVW	0(%ap),%r0		# from-address to %r0
	MOVW	4(%ap),%r1		# to-address to %r1
	MOVW	8(%ap),%r2		# byte count to %r2
 	BEH	copygood

	MOVAW	copyfault,u+u_caddrflt

	ANDW3	%r0,&0x03,%r7		# get alignment of addresses
	ANDW3	%r1,&0x03,%r8
	BITB	&0x3,%r2		# check	count multiple of 4
	BNEB	copygen
	ORW3	%r7,%r8,%r6		# check both addresses word aligned
	BEH	copyblk

copygen:
	SUBW3	%r7,%r8,%r6
	BEH	copyalw			# addresses aligned modulo 4
	BITW	&0x01,%r6
	BEH	copyalh			# addresses aligned modulo 2

#
#	byte-aligned addresses;	copy 32 bytes at a time
#

	CMPW	&0x20, %r2		# if copying less than 32 bytes,
	BLH	copyalwrap		# go to the copy wrap up code

copyalb:
	MOVB	0x0(%r0),0x0(%r1)	# unrolled loop to copy 32 bytes
	MOVB	0x1(%r0),0x1(%r1)
	MOVB	0x2(%r0),0x2(%r1)
	MOVB	0x3(%r0),0x3(%r1)
	MOVB	0x4(%r0),0x4(%r1)
	MOVB	0x5(%r0),0x5(%r1)
	MOVB	0x6(%r0),0x6(%r1)
	MOVB	0x7(%r0),0x7(%r1)
	MOVB	0x8(%r0),0x8(%r1)
	MOVB	0x9(%r0),0x9(%r1)
	MOVB	0xa(%r0),0xa(%r1)
	MOVB	0xb(%r0),0xb(%r1)
	MOVB	0xc(%r0),0xc(%r1)
	MOVB	0xd(%r0),0xd(%r1)
	MOVB	0xe(%r0),0xe(%r1)
	MOVB	0xf(%r0),0xf(%r1)
	MOVB	0x10(%r0),0x10(%r1)
	MOVB	0x11(%r0),0x11(%r1)
	MOVB	0x12(%r0),0x12(%r1)
	MOVB	0x13(%r0),0x13(%r1)
	MOVB	0x14(%r0),0x14(%r1)
	MOVB	0x15(%r0),0x15(%r1)
	MOVB	0x16(%r0),0x16(%r1)
	MOVB	0x17(%r0),0x17(%r1)
	MOVB	0x18(%r0),0x18(%r1)
	MOVB	0x19(%r0),0x19(%r1)
	MOVB	0x1a(%r0),0x1a(%r1)
	MOVB	0x1b(%r0),0x1b(%r1)
	MOVB	0x1c(%r0),0x1c(%r1)
	MOVB	0x1d(%r0),0x1d(%r1)
	MOVB	0x1e(%r0),0x1e(%r1)
	MOVB	0x1f(%r0),0x1f(%r1)
	ADDW2	&0x20, %r0		# increment the from pointer
	ADDW2	&0x20, %r1		# increment the to pointer
	SUBW2	&0x20, %r2		# decrement the count
	BLEH	copygood		# return if count is 0
	CMPW	&0x20, %r2		# if at least 32 bytes left,
	BGEH	copyalb			# go to top of unrolled loop

copyalwrap:				# copy wrap up code (less than 32)
	MOVB	0(%r0),0(%r1)		# copy byte byte by byte
	INCW	%r0
	INCW	%r1
	DECW	%r2
	BNEB	copyalwrap
	jmp	copygood

#
#	half-word aligned addresses; copy 16 half-words at a time
#
copyalh:
	BITW	&0x01,%r0		# copy any initial odd-aligned byte
	BEB	copyalh1

	MOVB	0(%r0),0(%r1)
	INCW	%r0
	INCW	%r1
	DECW	%r2
	BEH	copygood

copyalh1:
	LRSW3	&1,%r2,%r6		# number of half-words
	BEH	copyalh2

	CMPW	&0x10, %r6		# if less than 16 halfwords
	BLH	copyalhwrap1		# go to copy wrap up code

copyalhcopy:
	MOVH	0x0(%r0),0x0(%r1)	# unrolled loop to copy 32 bytes
	MOVH	0x2(%r0),0x2(%r1)
	MOVH	0x4(%r0),0x4(%r1)
	MOVH	0x6(%r0),0x6(%r1)
	MOVH	0x8(%r0),0x8(%r1)
	MOVH	0xa(%r0),0xa(%r1)
	MOVH	0xc(%r0),0xc(%r1)
	MOVH	0xe(%r0),0xe(%r1)
	MOVH	0x10(%r0),0x10(%r1)
	MOVH	0x12(%r0),0x12(%r1)
	MOVH	0x14(%r0),0x14(%r1)
	MOVH	0x16(%r0),0x16(%r1)
	MOVH	0x18(%r0),0x18(%r1)
	MOVH	0x1a(%r0),0x1a(%r1)
	MOVH	0x1c(%r0),0x1c(%r1)
	MOVH	0x1e(%r0),0x1e(%r1)
	ADDW2	&0x20, %r0		# increment the from pointer
	ADDW2	&0x20, %r1		# increment the to pointer
	SUBW2	&0x10, %r6		# decrement the count (of halfwords)
	BLEH	copyalhwrap2		# if no halfwords left, go to wrap up
	CMPW	&0x10, %r6		# if at least 16 halfwords left, 
	BGEH	copyalhcopy		# go to top of unrolled loop

copyalhwrap1:				# wrap up copy of remaining halfwords
	MOVH	0(%r0),0(%r1)		# copy halfword by halfword
	ADDW2	&2,%r0
	ADDW2	&2,%r1
	DECW	%r6
	BNEB	copyalhwrap1

copyalhwrap2:
	BITW	&0x01,%r2		# copy any trailing odd-aligned byte
	BEH	copygood

copyalh2:
	MOVB	0(%r0),0(%r1)
	jmp	copygood

#
#	word aligned addresses; use block move instruction
#
copyalw:
	BITW	&0x03,%r0		# copy any initial unaligned bytes
	BEB	copyalw1

	MOVB	0(%r0),0(%r1)
	INCW	%r0
	INCW	%r1
	DECW	%r2
	BNEB	copyalw
	jmp	copygood

copyalw1:
	MOVW	%r2,%r6
	LRSW3	&2,%r2,%r2		# number of words
	BEB	copyalw2

	MOVBLW				# copy words

	ANDW2	&0x03,%r6		# bytes remaining to be copied
	BEH	copygood

copyalw2:
	MOVB	0(%r0),0(%r1)		# copy any trailing unaligned bytes
	INCW	%r0
	INCW	%r1
	DECW	%r6
	BNEB	copyalw2
	jmp	copygood

#
#	word aligned addresses and count; copy 128 byte blocks
#
copyblk:
	CMPW	&0x80,%r2
	BLH	copysmall
	PUSHW	&copylast		# where we return to
	MOVAW	fbclopflt,u+u_caddrflt	# psw has been pushed
	TSTW	%r5		# which r[0-1] contains the user address?
	BNEB	cpalgr1		#align on r1
#
#	align on %r0 (the user address being copied from)
#
	ANDW3	%r0,&0x7f,%r6		# how much of the current block copied
	SUBW2	%r6,%r1			# set offset for the loop
	ARSW3	&2,%r6,%r6		# bytes to words
	LLSW3	&2,%r6,%r7		# words to loop offset
	ADDW2	%r7,%r6			# --> MULW2  &5,%r6
	ADDW2	%r0,%r2			# addr + size
	ANDW2	&0xffffff80,%r0		# set bases for loop
	SUBW3	%r0,%r2,%r7
	ARSW3	&7,%r7,%r2		# no of 128 byte blocks
	ANDW2	&0x7f,%r7		# number of bytes left over.
	JMP	fbclopstrt(%r6)		# do the copies
#
#	align on %r1 (the user address being copied to)
#
cpalgr1:
	ANDW3	%r1,&0x7f,%r6		# how much of the current block copied.
	SUBW2	%r6,%r0			# set offset for the loop
	ARSW3	&2,%r6,%r6		# bytes to words
	LLSW3	&2,%r6,%r7		# words to loop offset
	ADDW2	%r7,%r6			# --> MULW2  &5,%r6
	ADDW2	%r1,%r2			# addr + size
	ANDW2	&0xffffff80,%r1		# set base for loop
	SUBW3	%r1,%r2,%r7
	ARSW3	&7,%r7,%r2		# no of 128 byte blocks
	ANDW2	&0x7f,%r7		# number of bytes left over.
	JMP	fbclopstrt(%r6)		# do the copies
#
#	After bulk copies are done, come here to finish the copy
#
copylast:
	MOVW	%r7,%r2
copysmall:
	MOVAW	copyfault,u+u_caddrflt
	ARSW3	&2,%r2,%r2
	MOVBLW

copygood:
	CLRW	u+u_caddrflt
	CLRW	%r0		# return success flag
	RESTORE	%r5
	RET


# Code for fubyte, fuibyte, fuword, etc.
#
# functions to fetch and store bytes and words from user space.
#
#
#extern int caddrflt;
#
#fubyte(src)
#register unsigned char * src;
#{
#	register unsigned int tbyte;
#
#	if
#	(
#		(int) src < (unsigned)MINUVTXT
#		||
#		(unsigned)UVBLK <= (unsigned)src  &&  (unsigned)src < (unsigned)UVSTACK
#	)
#		return(-1);
#
#label:
#	if (caddrflt) {
#		caddrflt = 0;
#		return(-1);
#	}
#
#	caddrflt = (int) label;
#	tbyte = *src;
#	caddrflt = 0;
#	return(tbyte);
#}
#
#fuibyte(src)
#unsigned char * src;
#{
#	return(fubyte(src));
#}
#
#fuword(src)
#register unsigned int * src;
#{
#	register unsigned int tword;
#
#	if
#	(
#		(int) src < (unsigned)MINUVTXT
#		||
#		(unsigned)UVBLK <= (unsigned)src  &&  (unsigned)src < (unsigned)UVSTACK
#	)
#		return(-1);
#
#label:
#	if (caddrflt) {
#		caddrflt = 0;
#		return(-1);
#	}
#
#	caddrflt = (int) label;
#	tword = *src;
#	caddrflt = 0;
#	return(tword);
#}
#
#fuiword(src)
#unsigned int * src;
#{
#	return(fuword(src));
#}
#
#subyte(dst,tbyte)
#register unsigned char * dst;
#register unsigned char tbyte;
#{
#	if
#	(
#		(int) dst < (unsigned)MINUVTXT
#		||
#		(unsigned)UVBLK <= (unsigned)dst  &&  (unsigned)dst < (unsigned)UVSTACK
#	)
#		return(-1);
#
#label:
#	if (caddrflt) {
#		caddrflt = 0;
#		return(-1);
#	}
#
#	caddrflt = (int) label;
#	*dst = tbyte;
#	caddrflt = 0;
#	return(0);
#}
#
#suibyte(dst,tbyte)
#unsigned char * dst;
#unsigned char tbyte;
#{
#	subyte(dst,tbyte);
#}
#
#suword(dst,tword)
#register unsigned int * dst;
#register unsigned int tword;
#{
#	if
#	(
#		(int) dst < (unsigned)MINUVTXT
#		||
#		(unsigned)UVBLK <= (unsigned)dst  &&  (unsigned)dst < (unsigned)UVSTACK
#	)
#		return(-1);
#
#label:
#	if (caddrflt) {
#		caddrflt = 0;
#		return(-1);
#	}
#
#	caddrflt = (int) label;
#	*dst = tword;
#	caddrflt = 0;
#	return(0);
#}
#
#suiword(dst,tword)
#unsigned int * dst;
#unsigned int tword;
#{
#	return(suword(dst,tword));
#}
#

	.globl	fubyte
	.globl	fuibyte
	.globl	lfubyte
fubyte:
fuibyte:
	MOVW	u+u_procp,%r0		# check if the process is an rfs
	CMPH	p_sysid(%r0),&0		# true if u.u_procp->p_sysid != 0
	je	lfubyte
	PUSHW	0(%ap)
	call	&1,rfubyte
	RET
lfubyte:				# local fubyte
	MOVW	0(%ap),%r1
	CMPW	&MINUVTXT,%r1
	jlu	sufubad
	CMPW	&UVUBLK,%r1
	jlu	cont2
	CMPW	&UVSTACK,%r1
	jlu	sufubad

cont2:	MOVAW	sf_fault,u+u_caddrflt
	MOVB	0(%r1),%r0
	CLRW	u+u_caddrflt
	RET

	.globl	fuword
	.globl	fuiword
	.globl	lfuword
fuword:
fuiword:
	MOVW	u+u_procp,%r0		# check if the process is an RFS server
	CMPH	p_sysid(%r0),&0		# true if u.u_procp->p_sysid != 0
	je	lfuword
	PUSHW	0(%ap)
	call	&1,rfuword
	RET
lfuword:				# local fuword
	MOVW	0(%ap),%r1
	CMPW	&MINUVTXT,%r1
	jlu	sufubad
	CMPW	&UVUBLK,%r1
	jlu	cont4
	CMPW	&UVSTACK,%r1
	jlu	sufubad

cont4:	MOVAW	sf_fault,u+u_caddrflt
	MOVW	0(%r1),%r0
	CLRW	u+u_caddrflt
	RET

	.globl	subyte
	.globl	suibyte
subyte:
suibyte:
	MOVW	u+u_procp,%r0		# check if the process is an RFS server
	CMPH	p_sysid(%r0),&0		# true if u.u_procp->p_sysid != 0
	je	lsubyte
	PUSHW	0(%ap)
	PUSHW	4(%ap)
	call	&2,rsubyte		# remote subyte
	RET
lsubyte:				# local lsubyte
	MOVW	0(%ap),%r1
	CMPW	&MINUVTXT,%r1
	jlu	sufubad
	CMPW	&UVUBLK,%r1
	jlu	cont6
	CMPW	&UVSTACK,%r1
	jlu	sufubad

cont6:	MOVAW	sf_fault,u+u_caddrflt
	MOVB	7(%ap),0(%r1)
	CLRW	u+u_caddrflt
	CLRW	%r0
	RET

	.globl	suword
	.globl	suiword
suword:
suiword:
	MOVW	u+u_procp,%r0		# check if the process is an RFS server
	CMPH	p_sysid(%r0),&0		# true if u.u_procp->p_sysid != 0
	je	lsuword
	PUSHW	0(%ap)
	PUSHW	4(%ap)
	call	&2,rsuword		# remote suword
	RET
lsuword:				# local suword
	MOVW	0(%ap),%r1
	CMPW	&MINUVTXT,%r1
	jlu	sufubad
	CMPW	&UVUBLK,%r1
	jlu	cont8
	CMPW	&UVSTACK,%r1
	jlu	sufubad

cont8:	MOVAW	sf_fault,u+u_caddrflt
	MOVW	4(%ap),0(%r1)
	CLRW	u+u_caddrflt
	CLRW	%r0
	RET

sf_fault:
	CLRW	u+u_caddrflt
sufubad:
	MOVW	&-1,%r0			# return failure flag
	RET

# memprobe
# assumes vaddr is a potentially valid virtual address.
# returns 0 if the referenced page is valid, -1 otherwise.
#	
#	int
#	memprobe(vaddr)
#		caddr_t		vaddr;
#	{
#		extern int	memproberr();
#	
#		mpprio = splhi();
#		vaddr &= ~3;
#		mpcaddrsave = u.u_caddrflt;
#		u.u_caddrflt = (int)memproberr;
#		asm("	MOVTRW *0(%ap),%r0");
#		asm("	NOP");
#		u.u_caddrflt = mpcaddrsave;
#		splx(mpprio);
#		return 0;
#	}

# Saving caddrflt and psw here assumes we''re running at splhi
	.data
	.align 4
mpcaddrsave:
	.word 0
mpprio:
	.word 0

	.text
	.globl memprobe
memprobe:
	MOVW    %psw,mpprio		# mpprio = splhi();
	ORW2    &0x1e000,%psw
	ANDW2   &-0x4,0(%ap)		# vaddr &= ~3;
	MOVW    u+u_caddrflt,mpcaddrsave
	MOVAW   memproberr,u+u_caddrflt
	MOVTRW  *0x0(%ap),%r0		# /dev/null
	CLRW    %r0			# success
	jmp	memprobeout
	
# Trap probe of bad kernel address.  Assumes it runs at high enough
# priority that previous values of u_caddrflt and psw are safely stored
# in mpcaddrsave and mpprio, respectively.  Returns 0.

memproberr:
	MOVW	&-1,%r0			# return failure flag
memprobeout:
	MOVW	mpcaddrsave,u+u_caddrflt
	MOVW    mpprio,%psw		# splx(mpprio)
	RET

#	Read in pathname from kernel space
#
#	spath (from, to, maxbufsize);
#
#	Returns -2 if pathname is too long, otherwise returns
#	the pathname length.

	.globl	spath
spath:
	MOVW	0(%ap), %r1		# calculate the pathname 
	MOVW	%r1, %r0		# length and leave 
	STREND				# the result in 
	SUBW2	%r1, %r0		# r0 and r2
	MOVW	%r0, %r2
	SUBW3	%r0, 8(%ap), %r0	# Compare the pathname length
#					# with the maximum size of 
#					# the buffer, difference is in 
#					# r0
	jle	plenerr			# Error, if the length is 
#					# > (maxbufsize-1), -1 for NULL
	MOVW	%r1, %r0		# r0 = from
	MOVW	4(%ap), %r1		# r1 = to
	STRCPY
	MOVW	%r2, %r0		# return the pathname length
	RET

#	Read in pathname from user space
#
#	upath (from, to, maxbufsize);
#
#	Returns -2 if the pathname is too long, -1 if a bad user
#	address is supplied, otherwise returns the pathname length.

	.globl	upath
upath:
	MOVW	0(%ap),%r1
	CMPW	&MINUVTXT,%r1	# if from < (unsigned)MINUVTXT ||
	jlu	upbaddr
	CMPW	&UVUBLK,%r1	# if (unsigned) UVBLK <= from &&
	jlu	upcont
	CMPW	&UVSTACK,%r1	# if from < (unsigned)UVSTACK -> error
	jlu	upbaddr
upcont:
	MOVAW	sf_fault,u+u_caddrflt	# initialize fault code
	MOVW	%r1, %r0		# calculate the pathname 
	STREND				# length and leave 
	SUBW2	%r1, %r0		# the result in
	MOVW	%r0, %r2		# r0 and r2
	SUBW3	%r0, 8(%ap), %r0	# Compare the pathname length
#					# with the maximum size of 
#					# the buffer, difference is in 
#					# r0
	jle	plenerr			# Error, if the length is 
#					# > (maxbufsize-1), -1 for NULL
	MOVW	%r1, %r0		# r0 = from
	MOVW	4(%ap), %r1		# r1 = to
	STRCPY
	MOVW	%r2, %r0		# return the pathname length
	CLRW	u+u_caddrflt
	RET

upbaddr:
	MOVW	&-1, %r0		# return error (-1) on 
#					# out of range address
	RET

plenerr:
	CLRW	u+u_caddrflt
	MOVW	&-2, %r0		# return error (-2) on pathname
#					# length error 
	RET



#	Read in pathname from kernel space
#	available for fast path name extraction in kernel space
#
#	kpath (from, to, maxbufsize);
#
#	Returns -2 if the pathname is too long.

	.globl	kpath
kpath:
	MOVW	0(%ap),%r1
	MOVW	%r1, %r0		# calculate the pathname
	STREND				# length and leave
	SUBW2	%r1, %r0		# the result in
	MOVW	%r0, %r2		# r0 and r2
	SUBW3	%r0, 8(%ap), %r0	# Compare the pathname length
#					# with the maximum size of
#					# the buffer, difference is in
#					# r0
	jle	kplenerr			# Error, if the length is
#					# > (maxbufsize-1), -1 for NULL
	MOVW	%r1, %r0		# r0 = from
	MOVW	4(%ap), %r1		# r1 = to
	STRCPY
	MOVW	%r2, %r0		# return the pathname length
	RET

kplenerr:
	MOVW	&-2, %r0		# return error (-2) on pathname
#					# length error
	RET



#
# This code is the init process; it is copied to user text space
# and it then does exec( "/sbin/init", "/sbin/init", 0 );
#
	.data
	.align 4

	.globl	icode
	.globl	szicode

	.set	_exec,11*8

icode:
	MOVW	&UVTEXT,%r0	# this is the virtual address of "icode"

#	We are still running on the kernel pcb and kernel stack.
#	We must continue to run in kernel mode since we are
#	going to write %pcbp in order to switch to the user''s
#	pcb and stack.

	MOVAW	u+u_pcb,%pcbp	# Point to new pcb.
	MOVAW	userstack,%sp	# Switch to user stack.
	MOVW	%sp,%ap		# Init arg pointer to empty stack.
	MOVW	%sp,%fp		# Init frame pointer to empty stack.
	MOVAW	u+u_pcb,u+u_pcbp # Set ptr to current pcb.

	PUSHAW	sbin_off(%r0)	# address of sbin_init
	PUSHAW	argv_off(%r0)	# argv[] array

	CALL	-8(%sp),icode1_off(%r0)	# call icode1
icode1:
	.set	icode1_off,.-icode
	MOVW	&4,%r0
	MOVW	&_exec,%r1
	GATE			# exec( "/sbin/init", argv )
icode2:
	BRB	icode2

	.align 4
argv:		# argv area
	.set	argv_off,.-icode
	.word	UVTEXT+sbin_off
	.word	0

sbin_init:	# /sbin/init
	.set	sbin_off,.-icode
	.byte	0x2f,0x73,0x62,0x69,0x6e,0x2f,0x69,0x6e,0x69,0x74,0
icode_end:

	.data
	.align	4
szicode:	# length of icode
	.word	icode_end-icode

	.globl	cputype
cputype:
	.half	0x3b2


#	searchdir(buf, n, target) - search a directory for target
#	returns offset of match, or empty slot, or -1

	.globl	searchdir

	.text
searchdir:
	save	&4
	MOVW	0(%ap), %r8			# pointer to directory
	MOVW	4(%ap), %r7			# directory length in bytes
	MOVW	&16, %r6			# sizeof(struct direct)
	MOVW	&0, %r5				# pointer to empty slot

	.align	4
s_top:
	CMPW	%r6, %r7			# length less than 16?
	jl	sdone				# jump if r7 < r6
	CMPH	0(%r8), &0			# directory entry empty?
	je	sempty				# jump if true
	MOVAW	2(%r8), %r0			# address of file name
	MOVW	8(%ap), %r2			# address of target name
	CMPB	0(%r0), 0(%r2)			# compare characters
	jne	scont				# jump if different
	CMPB	1(%r0), 1(%r2)
	jne	scont
	TSTB	1(%r0)				# after second character,
	je	smatch				# if equal and zero, match

	CMPB	2(%r0), 2(%r2)
	jne	scont
	TSTB	2(%r0)
	je	smatch

	CMPB	3(%r0), 3(%r2)
	jne	scont
	TSTB	3(%r0)
	je	smatch

	CMPB	4(%r0), 4(%r2)
	jne	scont
	TSTB	4(%r0)
	je	smatch

	CMPB	5(%r0), 5(%r2)
	jne	scont
	TSTB	5(%r0)
	je	smatch

	CMPB	6(%r0), 6(%r2)
	jne	scont
	TSTB	6(%r0)
	je	smatch

	CMPB	7(%r0), 7(%r2)
	jne	scont
	TSTB	7(%r0)
	je	smatch

	CMPB	8(%r0), 8(%r2)
	jne	scont
	TSTB	8(%r0)
	je	smatch

	CMPB	9(%r0), 9(%r2)
	jne	scont
	TSTB	9(%r0)
	je	smatch

	CMPB	10(%r0), 10(%r2)
	jne	scont
	TSTB	10(%r0)
	je	smatch

	CMPB	11(%r0), 11(%r2)
	jne	scont
	TSTB	11(%r0)
	je	smatch

	CMPB	12(%r0), 12(%r2)
	jne	scont
	TSTB	12(%r0)
	je	smatch

	CMPB	13(%r0), 13(%r2)
	je	smatch				# jump if match even if not zero
scont:
	ADDW2	%r6, %r8			# increment directory pointer
	SUBW2	%r6, %r7			# decrement size
	jmp	s_top				# keep looking

sempty:
	CMPW	%r5, &0				# do we need an empty slot?
	jne	scont				# jump if no
	MOVW	%r8, %r5			# save current offset
	jmp	scont				# and goto to next entry

smatch:
	SUBW2	0(%ap), %r8			# convert to offset
	MOVW	%r8, %r0			# return offset
	ret	&4

sdone:
	MCOMW	&0, %r0				# save failure return
	CMPW	%r5, &0				# empty slot found?
	je	sfail				# jump if false
	SUBW2	0(%ap), %r5			# convert to offset
	MOVW	%r5, %r0			# return empty slot
sfail:
	ret	&4

	.globl is32b

is32b:
	MVERNO			# puts version number in r0
	SUBW2	&0x19,%r0
	jle	WE32001		# ver. number <= 25 is a WE32001 - ret. 0
	MOVW	&0x1,%r0	# ver. number >  25 is a WE32100 - ret. 1
	RET
WE32001:
	MOVW	&0x0,%r0
	RET

#	Compute number of arguments and total size of strings
#	in an argument list.
#
#	arglistsz(ap, acp, ssp, sslimit);
#
#	where ap is the argument list pointer (a user address)
#	and sslimit is the string size limit.
#
#	Returns -2 if the string size is too long, -1 if a bad user
#	address is supplied, otherwise returns  0 with
#	*acp = argument count and *ssp = total string size.

	.globl	arglistsz
arglistsz:
	PUSHW	%r3
	PUSHW	%r4
	PUSHW	%r5
	MOVAW	alsbaddr,u+u_caddrflt	# initialize fault code
	MOVW	0(%ap),%r2		# arg ptr
	MOVW	*4(%ap),%r3		# initial arg count
	MOVW	*8(%ap),%r4		# initial string size
	MOVW	0xc(%ap),%r5		# string size limit
	CMPW	&MINUVTXT,%r2	# if from < (unsigned)MINUVTXT ||
	jlu	alsbaddr
	CMPW	&UVUBLK,%r2	# if (unsigned) UVBLK <= from &&
	jlu	als2argl
	CMPW	&UVSTACK,%r2	# if from < (unsigned)UVSTACK -> error
	jlu	alsbaddr

#	Arglist in section 3
#
als3argl:
	MOVW	0(%r2),%r1	# get next arg ptr
	je	alsdone		# if it is NULL, we are done
	ADDW2	&1,%r3		# increment arg count
	CMPW	&MINUVTXT,%r1	# if from < (unsigned)MINUVTXT ||
	jlu	alsbaddr
	CMPW	&UVUBLK,%r1	# if (unsigned) UVBLK <= from &&
	jlu	als3stg
	CMPW	&UVSTACK,%r1	# if from < (unsigned)UVSTACK -> error
	jlu	alsbaddr
als3stg:
	MOVW	%r1,%r0
	STREND				# length and leave 
	SUBW2	%r1, %r0		# the result in
	ADDW2	&1, %r0			# count the NULL byte
	ADDW2	%r0,%r4			# increment string size
	SUBW3	%r4, %r5, %r0		# does it still fit
	jl	alsenerr		# Error, if the length is 
#					# > maxstringsz
	ADDW2	&4, %r2			# increment argp
	CMPW	&UVSTACK,%r2		# if argp < (unsigned)UVSTACK
#					  then it has wrapped around: error
	jgeu	als3argl
alsbaddr:
	CLRW	u+u_caddrflt
	POPW	%r5
	POPW	%r4
	POPW	%r3
	MOVW	&-1, %r0		# return error (-1) on 
#					# out of range address
	RET

alsdone:
	CLRW	u+u_caddrflt
	MOVW	%r3, *4(%ap)		# return the final arg count
	MOVW	%r4, *8(%ap)		# return the final string size
	POPW	%r5
	POPW	%r4
	POPW	%r3
	MOVW	&0, %r0
	RET

#	arglist in section 2
#
als2argl:
	MOVW	0(%r2),%r1	# get next arg ptr
	je	alsdone		# if it is NULL, we are done
	ADDW2	&1,%r3		# increment arg count
	CMPW	&MINUVTXT,%r1	# if from < (unsigned)MINUVTXT ||
	jlu	alsbaddr
	CMPW	&UVUBLK,%r1	# if (unsigned) UVBLK <= from &&
	jlu	als2stg
	CMPW	&UVSTACK,%r1	# if from < (unsigned)UVSTACK -> error
	jlu	alsbaddr
als2stg:
	MOVW	%r1,%r0
	STREND				# length and leave 
	SUBW2	%r1, %r0		# the result in
	ADDW2	&1, %r0			# count the NULL byte
	ADDW2	%r0,%r4			# increment string size
	SUBW3	%r4, %r5, %r0		# does it still fit
	jl	alsenerr		# Error, if the length is 
#					# > (maxstringsz
	ADDW2	&4, %r2			# increment argp
	CMPW	&UVUBLK,%r2	# if (unsigned) UVBLK <= from &&
#				  then it has hit section 3: error
	jlu	als2argl
	jmp	alsbaddr

alsenerr:
	CLRW	u+u_caddrflt
	POPW	%r5
	POPW	%r4
	POPW	%r3
	MOVW	&-2, %r0		# return error (-2) on pathname
#					# length error 
	RET

#	Copy an argument list and its strings
#	into a compact form at an interim address.
#	Although we can assume the intermin addresses are OK,
#	the pointers in the array must be rechecked because
#	their image could be in shared writable pages.
#
#	copyarglist(ac, fap, pdelta, tap, tsp, fkernel);
#
#	where ac is the argument count,
#	fap is the "from" argumunt pointer (a user address
#	if fkernel is zero, or a kernel/uarea address if fkernel is 1),
#	pdelta is the pointr delta to apply to the values
#	in the new pointer list, tap is the "to" argument
#	pointer (a user address), and tsp is the "to"
#	string pointer.
#
#	The strings pointed to by the from pointer list
#	are copied to the "to" string space and the new pointer
#	list is constructed with pointers to the string starts
#	offset by pdelta.  The reason for pdelta is that
#	exec first builds the stack frame at the wrong place
#	(since it cannot clobber existing data), then the image
#	is moved to the right place virtually in the new address space.
#
#	The size of the copied strings is returned on success
#	and -1 is returned on failure.
#

	.globl	copyarglist
copyarglist:
	PUSHW	%r3
	PUSHW	%r4
	PUSHW	%r5
	PUSHW	%r6
	PUSHW	%r7
	MOVW	0(%ap),%r2
	MOVW	4(%ap),%r3
	MOVW	8(%ap),%r4
	MOVW	0xc(%ap),%r5
	MOVW	0x10(%ap),%r1
	MOVW	0x14(%ap), %r7
	MOVAW	calbaddr,u+u_caddrflt	# initialize fault code
#
#	Note that the fap has already been checked
#	but the string pointers can get clobbered because
#	they may be in shared pages that change.
#	Thus the pointers must get checked.
#	But the strings need not get checked for length now.
#	This relies on there being no virtual neighbor at the end
#	of the temporary image virtual space.
#	Thus, there would be a fault that fails.
#	Of course, implementations on other machines must make sure
#	that an overrun will not be processed as a stack growth.
#	That affects the code that choses the virtual hole to use.
calargl:
	CMPW	&0, %r2		# check first to do nothing gracefully
	jle	caldone
	MOVW	0(%r3),%r0	# get next arg ptr
	CMPW	&0, %r7		# is it a kernel address?
	jne	calstg
#				# a NULL ptr means the code is insane
	CMPW	&MINUVTXT,%r0	# if from < (unsigned)MINUVTXT ||
	jlu	calbaddr
	CMPW	&UVUBLK,%r0	# if (unsigned) UVBLK <= from &&
	jlu	calstg
	CMPW	&UVSTACK,%r0	# if from < (unsigned)UVSTACK -> error
	jlu	calbaddr
calstg:
	ADDW3	%r4, %r1, %r6	# calculate the corrected new arg ptr
	MOVW	%r6, 0(%r5)	# and store it in new list
	STRCPY
	ADDW2	&1, %r1		# go past the NULL byte
	ADDW2	&4, %r3		# increment fap
	ADDW2	&4, %r5		# increment tap
	SUBW2	&1, %r2		# decrement ac
	jmp	calargl

calbaddr:
	CLRW	u+u_caddrflt
	POPW	%r7
	POPW	%r6
	POPW	%r5
	POPW	%r4
	POPW	%r3
	MOVW	&-1, %r0		# return error (-1) on 
#					# out of range address
	RET

caldone:
	SUBW3	0x10(%ap), %r1, %r0
	CLRW	u+u_caddrflt
	POPW	%r7
	POPW	%r6
	POPW	%r5
	POPW	%r4
	POPW	%r3
	RET

#
#	a caddrflt version of strlen for fname in exec:
#
	.globl	userstrlen
	.text
	.align	1
userstrlen:
	save	&0		# pgm uses scratch reg 0
	movw	0(%ap),%r0	# ptr to s1
	MOVAW	uslbaddr,u+u_caddrflt	# initialize fault code
	CMPW	&MINUVTXT,%r0	# if from < (unsigned)MINUVTXT ||
	jlu	uslbaddr
	CMPW	&UVUBLK,%r0	# if (unsigned) UVBLK <= from &&
	jlu	uslgetc
	CMPW	&UVSTACK,%r0	# if from < (unsigned)UVSTACK -> error
	jlu	uslbaddr
	jmp	uslgetc
uslloop:
	addw2	&1,%r0
uslgetc:
	cmpb	0(%r0),&0	# search for a terminating null
	jne	uslloop		# go back for more
	subw2	0(%ap),%r0	# calculate string length in r0
	CLRW	u+u_caddrflt
	ret	&0

uslbaddr:
	CLRW	u+u_caddrflt
	MOVW	&-1, %r0		# return error (-1) on 
	ret	&0

#
#	void
#	setintret(pc)
#		tmp = isp - 4;
#		(struct pcb *)(*tmp)->pc = pc;
#
#

.set	pcb_pc, 4	# Offset to pc in pcb

.globl	setintret

setintret:
	MOVW	-4(%isp), %r0
	MOVW	0(%ap), pcb_pc(%r0)
	RET
