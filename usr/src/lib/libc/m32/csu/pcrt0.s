#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

	.file "pcrt0.s"
	.ident	"@(#)libc-m32:csu/pcrt0.s	1.5"
# C runtime startoff
# modified for System V - release 2 function-call floating-point

# exit() is system call #1, _exit == (call #)*(sizeof(gatevector))
	.set	_exit,1*8

# global entities defined in this file
	.globl	_start
	.globl	_mcount
	.globl	_CAproc

# global entities defined elsewhere, but used here
	.globl	__fpstart
	.globl	main
	.globl	exit

#
#	C language startup routine
#

_fgdef_(_start):
	PUSHW	0(%ap)		# argc

	MOVAW	4(%ap),%r0
	PUSHW	%r0		# argv
	MOVW	(%r0),_CAproc
.L1:
	TSTW	0(%r0)		# null args term ?
	je	.L2
	ADDW2	&4,%r0
	jmp	.L1
.L2:
#	MOVW	*8(%r0),_CAproc
	MOVAW	4(%r0),%r0
	MOVW	%r0,environ	# indir is 0 if no env ; not 0 if env

	PUSHW	%r0		# envp

	CALL	0(%sp),__fpstart	# set up floating-point state

	CALL	-3*4(%sp),main

	PUSHW	%r0
	CALL	-1*4(%sp),exit

	MOVW	&4,%r0
	MOVW	&_exit,%r1
	GATE
#
_fgdef_(_mcount):		# dummy version for the case when
	rsb			# files have been compiled with -p but
				# not loaded with load module
	.data
	.align	4
_dwdef_(`environ'):
	.word	0
_dgdef_(_CAproc):
	.word	0
