# C library -- execv
.ident	"@(#)libc-m32:sys/execv.s	1.6.1.4"

# execv(file, argv);

# where argv is a vector argv[0] ... argv[x], 0
# last vector element must be 0

	.globl	execve

_fwdef_(`execv'):
	MCOUNT
	PUSHW	0(%ap)		#  file
	PUSHW	4(%ap)		#  argv
#
# The following macro call is for shared libraries.
# If SHLIB is defined, substitute environ for a pointer to environ (_libc__environ)
#
	PUSHW   _dref_(environ) #  default environ
	CALL	-12(%sp),_fref_(execve)
	RET
