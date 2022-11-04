# C library -- _rename
.ident	"@(#)libc-m32:sys/_rename.s	1.3"

# _rename() is the system call version of rename()

	.set	__rename,134*8

	.globl	_rename
	.globl  _cerror

_fgdef_(_rename):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__rename,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
