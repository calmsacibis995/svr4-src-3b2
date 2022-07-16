#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

	.ident	"@(#)libc-m32:csu/pcrtn.s	1.2"
	.file	"pcrtn.s"
#
# This code provides the end to the _init and _fini functions which are 
# used C++ static constructors and desctuctors.  This file is
# included by cc as the last component of the ld command line
#
	_section24_(.init,x,a,progbits)
	pushaw	0(%pc)
	call	&1,_CAstartSO
	ret	&0
#
	_section24_(.fini,x,a,progbits)
	pushaw	0(%pc)
	call	&1,_CAstopSO
	ret	&0
