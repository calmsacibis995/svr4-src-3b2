# C-library sbrk
.ident	"@(#)libc-m32:sys/sbrk.s	1.6.1.6"

# oldend = sbrk(increment);

# sbrk gets increment more core, and returns a pointer
#	to the beginning of the new core area
#

	.globl	_nd

_fwdef_(`sbrk'):
	MCOUNT
	SAVE	%r8
	TSTW	0(%ap)
	je	.is_zero	# we know the answer without trying
	ADDW3   _dref_(_nd),0(%ap),%r8
	PUSHW	%r8
	CALL	-4(%sp),_fref_(brk)
 	TSTW	%r0
 	jne	brkerr
.is_zero:
	MOVW    _dref_(_nd),%r0
	SUBW2	0(%ap),%r0
brkerr:
	RESTORE	%r8
	RET

# brk(value)
# as described in man2.
# returns 0 for ok, -1 for error


	.set	_break,17*8

	.globl	_cerror

_fwdef_(`brk'):
	MOVW	&4,%r0
	MOVW	&_break,%r1
	GATE
	jgeu	noerr
	jmp	_cerror
noerr:
	MOVW    0(%ap),_dref_(_nd)
	CLRW	%r0
	RET
_m4_ifdef_(`DSHLIB',
`',
`	.data
_nd:
	.word	end
')
