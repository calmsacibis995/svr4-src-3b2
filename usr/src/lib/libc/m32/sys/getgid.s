# C library -- getgid
.ident	"@(#)libc-m32:sys/getgid.s	1.7"

# gid = getgid();
# returns real gid

	.set	__getgid,47*8

_fwdef_(`getgid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getgid,%r1
	GATE
	RET
