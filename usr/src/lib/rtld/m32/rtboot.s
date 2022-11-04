	.ident	"@(#)rtld:m32/rtboot.s	1.5"
	.file	"rtboot.s"

# bootstrap routine for run-time linker
# we get control from exec which has the run-time linker and
# the a.out and created the process stack
#
# on entry, the process stack looks like this:
#
#			# <- %sp
#_______________________#  high addresses
#	strings		#  
#_______________________#
#	0 word		#
#_______________________#
#	Auxiliary	#
#	entries		#
#	...		#
#	(size varies)	#
#_______________________#
#	0 word		#
#_______________________#
#	Environment	#
#	pointers	#
#	...		#
#	(one word each)	#
#_______________________#
#	0 word		#
#_______________________#
#	Argument	# low addresses
#	pointers	#
#	Argc words	#
#_______________________#
#	argc		# 
#_______________________# <- %ap

#
# We must first find the address at which ld.so was loaded,
# find the addr of the dynamic section of ld.so, of argv[0], of
# the process' environment pointers, and the beginning of the 
# auxiliary vector for the process.  We then call _rt_setup - on return
# we jump to the entry point for the a.out.

	.text
	.globl	_rt_boot
	.globl	_rt_setup
	.globl	_kill
	.globl	_base
	.globl	_getpid
	.globl	_write
	.type	_rt_boot,@function
	.align	4
_rt_boot:
				# get addresses of global offset
				# table and ld.so load point
				# load point is reference by special
				# symbol _base
	PUSHAW	_base@PC
	PUSHW	_DYNAMIC@GOT	# address of dynamic structure
				# get addr of environment pointers 
	MOVW	0(%ap),%r0	# argc
	PUSHW	4(%ap)		# argv[0]
	INCW	%r0		# add in 0 word
	LLSW3   &2,%r0,%r0	# get byte count ((argc + 1)* 4)
	MOVAW	4(%ap),%r1
	ADDW2	%r0,%r1		# %r1 is environ
	PUSHW	%r1
.L1:
	TSTW	0(%r1)		# loop through env pointers to find auxv
	je	.L2
	ADDW2	&4,%r1
	jmp	.L1
.L2:
	ADDW2	&4,%r1
	PUSHW	%r1		# %r1 is auxv
				# invoke run-time linker - it returns
				# entry point for a.out
	call	&5,_rt_setup@PLT

	JMP	0(%r0) 		# transfer control to a.out
	.size	_rt_boot,.-_rt_boot
