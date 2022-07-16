#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

        .file   "fnegd.s"
.ident	"@(#)libc-m32:fp/fnegs.s	1.5"
#       float _fnegs(srcF)
#       float srcF;

        .text
        .align  4
        .globl  _fnegs
_fgdef_(_fnegs):
        MCOUNT
        xorw3   &0x80000000,0(%ap),%r0
        RET

