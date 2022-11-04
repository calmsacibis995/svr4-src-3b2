# C library -- geteuid
.ident	"@(#)libc-m32:sys/geteuid.s	1.7"

# uid = geteuid();
#  returns effective uid

	.set	__getuid,24*8

_fwdef_(`geteuid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getuid,%r1
	GATE
	MOVW	%r1,%r0
	RET
