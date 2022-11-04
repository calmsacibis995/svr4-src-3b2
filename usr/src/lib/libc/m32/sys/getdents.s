# C library -- getdents
	.ident	"@(#)libc-m32:sys/getdents.s	1.2.1.7"
	.file 	"getdents.s"

# num = getdents(file, buffer, count, flag);
# num is number of bytes read; num == -1 means error

	.set	__getdents,81*8

	.globl  _cerror

_m4_ifdef_(`ABI',`
	.globl	getdents
_fgdef_(getdents):
',`
_m4_ifdef_(`DSHLIB',`
	.globl	getdents
_fgdef_(getdents):
',`
_fwdef_(`getdents'):
')
')
	MCOUNT
	MOVW	&4,%r0
	MOVW	&__getdents,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
