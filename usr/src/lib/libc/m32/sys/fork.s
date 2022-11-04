#
.ident	"@(#)libc-m32:sys/fork.s	1.7"
# C library -- fork

# pid = fork();

# r1 == 0 in parent process, r1 = 1 in child process.
# r0 == pid of child in parent, r0 == pid of parent in child.

	.set	__fork,2*8

	.globl	_cerror

_fwdef_(`fork'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__fork,%r1
	GATE
	jgeu	forkok
	jmp	_cerror
forkok:
	TSTW	%r1
	je	parent
	CLRW	%r0
parent:
	RET
