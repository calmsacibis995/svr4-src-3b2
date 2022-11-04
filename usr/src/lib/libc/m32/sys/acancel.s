
.ident	"@(#)libc-m32:sys/acancel.s	1.1"

# ndone = __acancel(ver, array, count)
# ndone == # async ops done; -1 means error

	.set	_acancel,110*8

	.globl	__acancel
	.globl  _cerror

_fgdef_(__acancel):
#	MCOUNT
	MOVW	&4,%r0
	MOVW	&_acancel,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
