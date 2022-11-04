# profil
.ident	"@(#)libc-m32:sys/profil.s	1.7"

	.set	_prof,44*8

_fwdef_(`profil'):
	MCOUNT
	MOVW	&4,%r0
	MOVW	&_prof,%r1
	GATE
	RET
