.ident	"@(#)libc-m32:sys/setgroups.s	1.1"
# setgroups(ngroups, grouplist);

	.set	__setgroups,91*8

	.globl	_cerror

_fwdef_(`setgroups'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__setgroups,%r1
	GATE
	jgeu	.noerror
	jmp	_cerror
.noerror:
	CLRW	%r0
	RET
