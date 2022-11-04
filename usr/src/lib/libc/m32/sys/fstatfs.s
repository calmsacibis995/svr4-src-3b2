.ident	"@(#)libc-m32:sys/fstatfs.s	1.4"
# C library -- fstatfs

# int
# fstatfs(fd, sp, len, fstyp);
# int fd;
# struct statfs *sp;
# int len, fstyp;

	.set	__fstatfs,38*8

	.globl  _cerror

_fwdef_(`fstatfs'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__fstatfs,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	CLRW	%r0
	RET
