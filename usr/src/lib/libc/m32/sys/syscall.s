# C library -- syscall
.ident	"@(#)libc-m32:sys/syscall.s	1.9"

#  Interpret a given system call

	.globl	_cerror

_m4_ifdef_(`ABI',`
	.globl	syscall
_fgdef_(syscall):
',`
_m4_ifdef_(`DSHLIB',`
	.globl	syscall
_fgdef_(syscall):
',`
_fwdef_(`syscall'):
')
')
	MCOUNT
	MOVW	&4,%r0
	ALSW3	&3,0(%ap),%r1	# syscall number
	ADDW2	&4,%ap		# one fewer arguments
	GATE
	jgeu	.noerror
	SUBW2	&4,%ap		# reclaim stack space
	jmp	_cerror
.noerror:
	SUBW2	&4,%ap		# reclaim stack space
	RET
