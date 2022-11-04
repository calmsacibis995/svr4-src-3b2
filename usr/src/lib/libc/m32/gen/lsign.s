# Determine the sign of a double-long number.

.ident	"@(#)libc-m32:gen/lsign.s	1.1"

	.file	"lsign.s"
	.text
	.align	4

_fwdef_(`lsign'):
	SAVE	%fp
#	MCOUNT

	LRSW3	&31,0(%ap),%r0
	ANDW2	&1,%r0

	RESTORE	%fp
	RET
