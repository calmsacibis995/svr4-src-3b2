	.ident	"@(#)libc-m32:gen/m32_data.s	1.10"
	.file   "m32_data.s"
# This file contains
# the definition of the
# global symbols errno, d.vect and _siguhandler
# 
# int errno;

	.globl	errno
	.comm	errno,4

	.data
	.align	4
	.globl	d.vect
	.globl	_siguhandler

				# the user's signal vectors
_dgdef2_(d.vect,128):		# alias used in libc:signal.s
_dgdef2_(_siguhandler,128): 	# alias used in libos:sigaction.c
	.zero	32 * 4
