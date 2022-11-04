	.ident	"@(#)libc-m32:csu/crtn.s	1.6"
	.file	"crtn.s"
#
# This code provides the end to the _init and _fini functions which are 
# used C++ static constructors and desctuctors.  This file is
# included by cc as the last component of the ld command line
#
	_section24_(.init,x,a,progbits)
	ret	&0
#
	_section24_(.fini,x,a,progbits)
	ret	&0
