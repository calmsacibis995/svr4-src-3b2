# C return sequence which
.ident	"@(#)libc-m32:crt/cerror.s	1.6"
# sets errno, returns -1.
# errno is define in libc-m32:gen/gen_data.s

	.globl	_cerror
	.globl	errno

_fgdef_(_cerror):
	MOVW	%r0,_dref_(errno)
	MNEGW	&1,%r0
	RET

