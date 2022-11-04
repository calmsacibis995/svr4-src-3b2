# C library -- getuid
.ident	"@(#)libc-m32:sys/getuid.s	1.7"

# uid = getuid();
#  returns real uid

	.set	__getuid,24*8

_fwdef_(`getuid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getuid,%r1
	GATE
	RET
