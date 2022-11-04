	.file	"vsprintf.s"
.ident	"@(#)libc-u3b:print/vsprintf.s	1.8"

#---------------------------------------------------------------#
#								#
# vsprintf for 3b20s						#
#								#
#								#
#								#
#---------------------------------------------------------------#


_m4_include_(print.defs)#	Shared definitions for printf family.


#
# vsprintf(destination buffer, format [, arg [, arg ...]] )
#
	.globl	vsprintf
	.align	4
_fgdef_(vsprintf):
	save	&.Rcnt			# Save registers.
	MCOUNT
	addw2	&.locals,%sp		# Reserve stack space for locals.
	movw	0(%ap),ptr		# First parameter--destination.
	movw	4(%ap),fmt		# Second parameter--format string.
	addw3	&8,%ap,%r0		# Get var_args ptr
	movw	0(%r0),argp(%fp)	# Get ptr to arguments
	mnegw	&1,bufend(%fp)		# Establish high buffer end pointer.
	movw	ptr,outstart(%fp)	# Remember where output started.
	jmp	_dref_(__doprnt)	# Do the heavy work.
