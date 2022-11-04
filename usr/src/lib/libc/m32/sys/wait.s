# C library -- wait
.ident	"@(#)libc-m32:sys/wait.s	1.9"

# pid = wait(0);
#   or,
# pid = wait(&status);

# pid == -1 if error
# status indicates fate of process, if given

	.set	__wait,7*8
	.set	ERESTART,91

	.globl  _cerror

	.align	1
_fwdef_(`wait'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__wait,%r1
	GATE
	jgeu 	.noerror
	CMPB	&ERESTART,%r0
	BEB	wait
	jmp 	_cerror
.noerror:
	TSTW	0(%ap)		# status desired?
	je	nostatus	# no
	MOVW	%r1,*0(%ap)	# store child's status
nostatus:
	RET
