	.ident	"@(#)libc-m32:csu/pcrti.s	1.2"
	.file	"pcrti.s"

# stubs for C++ static constructor and desctructor initialization
# routines
# This file is included by cc/ld before all application and library
# objects.  If ld is building an executable, this file comes after
# crt1.o; if ld is building a shared library, it is the first object
# passed to ld by cc
#
# e.g, a.out: ld /lib/crt1.o /lib/crti.o a.o b.o -lc -lsys /lib/crtn.o
# shared lib: ld -G /lib/crti.o a.o b.o /lib/crtn.o
#
	.globl	_init
	.globl	_fini
#
	_section24_(.init,x,a,progbits)
	.align	4
_fgdef_(_init):
	SAVE	%fp

	_section24_(.fini,x,a,progbits)
	.align	4
_fgdef_(_fini):
	SAVE	%fp
