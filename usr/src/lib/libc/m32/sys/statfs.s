.ident	"@(#)libc-m32:sys/statfs.s	1.4"
# C library -- statfs

# int
# statfs(filename, sp, len, fstyp);
# char *filename;
# struct statfs *sp;
# int len, fstyp;

	.set	__statfs,35*8

	.globl  _cerror

_fwdef_(`statfs'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__statfs,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
