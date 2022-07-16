#	Copyright (c) 1988 AT&T
#	All Rights Reserved 
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any 
#	actual or intended publication of such source code.

# count subroutine called during profiling
.ident	"@(#)libc-m32:crt/mcount.s	1.10"
#
# calling sequence:	MOVW	&a_word,%r0
#			JSB	_mcount
#			.data
#		a_word:	.word	0
#

	.globl	_mcount

# # #
# reserve ``call to _mcount_newent still pending'' flag, used
# to prevent recursion.
# isPending == 0	means we here have no outstanding call
# isPending == 1	means we here Have an outstanding call, and
#			may not call until that call returns!
#
	.data
.isPending:
	.word	0
	.text

# # #
#
# _mcount: call count increment routine for cc -p/-qp profiling (prof(1))
#
# Call with R0 pointing to a private, static cell (initially zero);
#
# Answer with that cell (now) pointing to a call count entry,
#  consisting of a ``backpointer to the function'' and an incremented
#  call count (allocate a new one for this fcn if cell was zero).
#
# All knowledge of call count entry management is handled in the
#  function _mcount_newent() et. al., which all live in libc-port:gen/mon.c.
#
#
# _mcount
#	if  nonzero cell contents	//i.e. points at an entry
#	  access entry, increment counter and return.

#	else				//fcn as of yet has no entry; get 1
#	  if  a pointer request is still pending..
#	    return without an entry - last request unsatisfied, and
#	    a recursive request cannot be satisfied.

#	  else
#	    get an entry pointer (mark `request pending' until it answers)
#	    if  entry pointer == 0
#	      return without an entry - profiling is Off.

#	    else
#	      initialize this entry: set backpointer to return address,
#	      set count to 1 for this call.
#	      return

#	    fi
#	  fi
#	fi
#

_fgdef_(_mcount):
	MOVW	0(%r0),%r1		# a_word to %r1
	jz	.do_inits		# do initialization if zero

	INCW	0(%r1)			# haveAnEntry: increm count for
	RSB				# this call and return.

# # #
# need to initialize the function's private cell (a_word) to point
# at a call count entry, which we shall try to allocate.
#
# we will do this only if mcount has not already been called and
# is waiting for mcount_newent to return!  if we are waiting, then
# in order to prevent recursion we shall forgo calling again.
#
.do_inits:


	CMPW	.isPending,&1		# Are we allowed to call newent?
	REQL				# If flag==1 we cannot.

	MOVW	&1,.isPending		# we CAN call: disable further Calls.

	PUSHW	%r0			# sith has no cell, preserve a_word, &
					# call to get a call count entry (or 0).

	call	&0,_fref_(_mcount_newent) # Now Call.
	MOVW	&0,.isPending		  # Now enable Calls again.
				# NOTE: return value is in **r0**:
				# 0==profg OFF, !0==ptr to callCntEntry
	POPW	%r1			# get a_word back: put in r1 now.

	CMPW	%r0,&0			# is profiling on? if no==0, if
	REQL				# yes, r0 has addr of entry.

# NOTE: for the sake of efficiency (since the call count entry address
# return value comes in r0), r0 and r1 now swap roles;
# r1 points at a_word and r0 accesses the allocated call count entry.

	MOVW	-4(%sp),0(%r0)		# save _mcount return addr in callcnt
	ADDW2	&4,%r0			#  entry: APROX'LY calling fcn's addr.
	MOVW	&1,0(%r0)		# set callcnt to count this 1st call.
	MOVW	%r0,0(%r1)		# store &pair+4 in a_word
	RSB				#  and return.
