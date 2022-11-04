.ident	"@(#)libc-m32:sys/sysinfo.s	1.1"
# OS library -- sysinfo

# error = sysinfo(cmd, buf, count)

	.set	_systeminfo,139*8

	.globl	_cerror

_fwdef_(`sysinfo'):
	#
	MOVW	&4,%r0
	MOVW	&_systeminfo,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
