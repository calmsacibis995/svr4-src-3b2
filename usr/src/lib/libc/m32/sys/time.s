# C library -- time
.ident	"@(#)libc-m32:sys/time.s	1.5"

# tvec = time(tvec);

	.set	__time,13*8

_fwdef_(`time'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__time,%r1
	GATE
	TSTW	0(%ap)
	je	nostore
	MOVW	%r0,*0(%ap)
nostore:
	RET
