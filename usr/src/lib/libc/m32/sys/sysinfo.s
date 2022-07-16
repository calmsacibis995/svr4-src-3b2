#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

.ident	"@(#)libc-m32:sys/sysinfo.s	1.2"
# OS library -- sysinfo

# error = sysinfo(cmd, buf, count)

	.set	_systeminfo,139*8

	.globl	_cerror

_m4_ifdef_(`ABI',`
	.globl	sysinfo
_fgdef_(sysinfo):
',`
_m4_ifdef_(`DSHLIB',`
	.globl	sysinfo
_fgdef_(sysinfo):
',`
_fwdef_(`sysinfo'):
')
')
	#
	MOVW	&4,%r0
	MOVW	&_systeminfo,%r1
	GATE
	jgeu 	.noerror
	jmp 	_cerror
.noerror:
	RET
