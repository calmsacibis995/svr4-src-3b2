	.file	"sigprocmsk.s"
	.ident	"@(#)libc-m32:sys/sigprocmsk.s	1.3"

	.set	SYSGATE,1*4
	.set	SIGPROCMASK,95*8
#
# sigprocmask(how,set,oset)		/* hold signal mask */ 
#
_fwdef_(`sigprocmask'):
	MCOUNT

	movw	&SYSGATE,%r0
	movw	&SIGPROCMASK,%r1
	GATE
	jlu	_cerror
	RET


