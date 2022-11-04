# C library -- getegid
.ident	"@(#)libc-m32:sys/getegid.s	1.7"

# gid = getegid();
# returns effective gid

	.set	__getgid,47*8

_fwdef_(`getegid'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getgid,%r1
	GATE
	MOVW	%r1,%r0
	RET
