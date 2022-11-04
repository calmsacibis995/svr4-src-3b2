	.file	"sigsuspend.s"
	.ident	"@(#)libc-m32:sys/sigsuspend.s	1.3"

	.set	SYSGATE,1*4
	.set	SIGSUSPEND,96*8	

#
# int sigsuspend(sigset_t *)	/* release signal mask if there is one held */ 
#

_fwdef_(`sigsuspend'):
	MCOUNT

	movw	&SYSGATE,%r0
	movw	&SIGSUSPEND,%r1
	GATE
	jlu	_cerror
	RET


