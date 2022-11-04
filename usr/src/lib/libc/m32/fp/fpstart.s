	.file	"fpstart.s"
.ident	"@(#)libc-m32:fp/fpstart.s	1.4"
#
# __FPSTART -- glue routine for floating-point startup.
#
# called only from crt0, this routine's sole purpose is to delay
# binding the actual floating-point startup code until after it is
# known if any floating-point code is included in the program.

	.globl	__fpstart
	.globl	_fpstart
_fgdef_(__fpstart):
	jmp	_fpstart
