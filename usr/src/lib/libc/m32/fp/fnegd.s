	.file	"fnegd.s"
.ident	"@(#)libc-m32:fp/fnegd.s	1.5"
#	double _fnegd(srcD)
#	double srcD;

	.text
	.align	4
	.globl	_fnegd
_fgdef_(_fnegd):
	MCOUNT
	xorw3	&0x80000000,0(%ap),%r0
	movw	4(%ap),%r1
	RET

