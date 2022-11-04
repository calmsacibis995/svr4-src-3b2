# OS library -- lchown
.ident	"@(#)libc-m32:sys/lchown.s	1.1"

# error = lchown(fd,owner,group)

	.set	__lchown,130*8

	.globl	_cerror

_fwdef_(`lchown'):
	MOVW	&4,%r0
	MOVW	&__lchown,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
