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

