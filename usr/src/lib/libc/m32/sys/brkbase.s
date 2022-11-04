# OS library -- brkbase
.ident	"@(#)libc-m32:sys/brkbase.s	1.1"

# error = brkbase(base_addr)
# sets the kernel's notion of where the
# program break is to base_addr 

	.set	__brkbase,127*8

	.globl	_cerror

_fwdef_(`brkbase'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__brkbase,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
