	.file	"sprintf.s"
.ident	"@(#)libc-u3b:print/sprintf.s	1.7"

#-----------------------------------------------------------------------#
#									#
# sprintf for 3b20s							#
#									#
#									#
#									#
#-----------------------------------------------------------------------#


_m4_include_(print.defs)#		Shared definitions for printf family.


#
# sprintf(destination buffer, format [, arg [, arg ...]] )
#
	.globl	sprintf
	.align	4
_fgdef_(sprintf):
	save	&.Rcnt			# Save registers.
	MCOUNT
	addw2	&.locals,%sp		# Reserve stack space for locals.
	movw	0(%ap),ptr		# First parameter--destination.
	movw	4(%ap),fmt		# Second parameter--format string.
	addw3	&8,%ap,argp(%fp)	# Get pointer to format arguments.
	mnegw	&1,bufend(%fp)		# Establish high buffer end pointer.
	movw	ptr,outstart(%fp)	# Remember where output started.
	jmp	_dref_(__doprnt)	# Do the heavy work.
