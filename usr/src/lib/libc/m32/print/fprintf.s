	.file	"fprintf.s"
.ident	"@(#)libc-u3b:print/fprintf.s	1.13"

#-----------------------------------------------------------------------#
#									#
# fprintf for 3b20s							#
#									#
#									#
#									#
#-----------------------------------------------------------------------#


_m4_include_(print.defs)#		Shared definitions for printf family.


#
# fprintf(stream, format [, arg [, arg ...]] )
#
	.globl	fprintf
	.align	4
_fgdef_(fprintf):
	save	&.Rcnt			# Save registers.
	MCOUNT
	addw2	&.locals,%sp		# Reserve stack space for locals.
	movw	0(%ap),iop		# First parameter--stream (FILE ptr).
	movw	4(%ap),fmt		# Second parameter--format string.
	addw3	&8,%ap,argp(%fp)	# Get pointer to format arguments.
	movw	iop,iophold(%fp)	# Save FILE pointer.
	bitb	&_IOWRT,_flag(iop)	# Check that write flag set
	jnz	.wset
	bitb	&_IORW,_flag(iop)	# Check that write flag set
	jnz	.rwset
	mnegw	&EOFN,%r0		# Set error return
	ret	&.Rcnt
.rwset:	orb2	&_IOWRT,_flag(iop)	# Set write bit
.wset:
	cmpw	&0,_base(iop)		# Check if first I/O to the stream
	jne	.skip1
	pushw	iop
	call	&1,_fref_(_findbuf)	# If first I/O to the stream, get a buffer
	cmpw	&0,%r0			# _findbuf returns nonzero if successful
	jne	.skip1	
	mnegw	&EOFN,%r0		# Set error return
	ret	&.Rcnt
.skip1:
	movw	_ptr(iop),ptr		# ptr = iop->_ptr
	movzbw	_file(iop),%r0		# Get file number
#
# Get _bufend(iop).  If _file(iop) < _NFILE then compute _bufend(iop)
# as the index into the _bufendtab array. Otherwise, call _realbufend(iop).
#
	cmpw	%r0, &_NFILE		# Check if _file(iop) < _NFILE
	jge	.lab1
	llsw2	&2,%r0			# Get word index into _bufendtab
_m4_ifdef_(`DSHLIB',
`	addw2	_daref_(_bufendtab),%r0
	movw	0(%r0),bufend(%fp)	# Pick up copy of buffer-end ptr.
',
`	movw	_bufendtab(%r0),bufend(%fp)# Pick up copy of buffer-end ptr.
')
	jmp	.lab2
.lab1:
	pushw	iop			# _file(iop) >= _NFILE
	call	&1,_fref_(_realbufend)
	movw	%r0,bufend(%fp)
.lab2:
	movw	&0,wcount(%fp)		# Initialize output character count.
	jmp	_dref_(__doprnt)	# Do the heavy work.
