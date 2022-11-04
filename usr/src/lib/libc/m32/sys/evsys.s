.ident	"@(#)libc-m32:sys/evsys.s	1.1"
#			File Contents
#			=============
#
#	This code contains assembly language interfaces for the events
#	VFS.  This includes the code for the evsys system call and the
#	ev_traptousr and ev_usrtrapret code for transfering to and 
#	returning from a user's trap handler.

	.file	"evsys.s"
	.globl	_cerror

	.set	__evsys,101*8
	.set	__evtrapret,102*8




#			The evsys Function
#			==================
#
#	The following is the code for the evsys system call.

_fwdef_(`evsys'):
	MCOUNT
	MOVW	&4,%r0			# Do the evtrapret system
	MOVW	&__evsys,%r1		# call.
	GATE				# ...
	jgeu	.noerror			# Jump if all o.k.
	jmp	_cerror			# Off to common error code.

.noerror:
	RET















#			The ev_traptousr Function
#			=========================
#
#	The following is the code used to transfer control to a
#	user trap handler and return to the kernel afterwords.  On
#	entry to this routine, the stack looks as follows.
#
#
#		_________________________________
#		|				|
#		|				|
#   old sp ---->|	old u.u_pcb		|
#		|				|
#		|_______________________________|
#		|				|
#		|				|
#		|	u.u_mau (maybe)		|
#		|				|
#		|				|
#		|_______________________________|
#		|				|
#		|	u.u_spop (maybe)	|
#		|				|
#		|_______________________________|
#		|				| 
#    ap,fp ---->|	ectxt_elp		|<-- cntxtp
#		|_______________________________|
#		|				|
#		|	ectxt_els		|
#		|_______________________________|
#		|				|
#		|	ectxt_tid		|
#		|_______________________________|
#		|				|
#		|	ectxt_cntxtp		|
#		|_______________________________|
#		|				|
#		|	ectxt_lvl		|
#		|_______________________________|
#		|				|
#		|	ectxt_ufunc		|
#		|_______________________________|
#		|				|
#       sp ---->|				|
#		|_______________________________|
#
#
#	The value of u.u_spop is saved only if "mau_present" is true.
#	The value of u.u_mau is save only if "mau_present" is true
#	and the U_SPOP_MAU bit is set in u.u_spop.








#			The ev_traptousr Function (Continued)
#			=====================================
#
#	On entry to the user's trap handler, the stack looks as follows
#	before the user does a SAVE.  It is assumed that he does do
#	a SAVE and RESTORE.  Otherwise, the registers won't get saved
#	properly since we don't do it.
#
#
#		_________________________________
#		|				|
#		|				|
#   old sp ---->|	old u.u_pcb		|
#		|				|
#		|_______________________________|
#		|				|
#		|				|
#		|	u.u_mau (maybe)		|
#		|				|
#		|				|
#		|_______________________________|
#		|				|
#		|	u.u_spop (maybe)	|
#		|				|
#		|_______________________________|
#		|				| 
#    ap,fp ---->|	ectxt_elp		|<-- cntxtp
#		|_______________________________|
#		|				|
#		|	ectxt_els		|
#		|_______________________________|
#		|				|
#		|	ectxt_tid		|
#		|_______________________________|
#		|				|
#		|	ectxt_cntxtp		|
#		|_______________________________|
#		|				|
#		|	ectxt_lvl		|
#		|_______________________________|
#		|				|
#		|	ectxt_ufunc		|
#		|_______________________________|
#		|				|
#		|	&ev_usrtrapret		|
#		|_______________________________|
#		|				|
#		|	saved ap		|
#		|_______________________________|
#		|				|
#       sp ---->|				|
#		|_______________________________|






#			The ev_traptousr Function (Continued)
#			=====================================
#
#	We come here from the kernel to transfer to a user's trap
#	handler.

_fwdef_(`ev_traptousr'):

	MCOUNT
	MOVW	-4(%sp),%r0		# Get func to call.
	PUSHW	_daref_(ev_usrtrapret) 	# Return address.
	PUSHW	%ap			# AP to restore.
	JMP	0(%r0)			# Off to function.



#			The ev_usrtrapret Function
#			==========================
#
#	A user's trap handler come's back here if it does a normal
#	function return.

_fwdef_(`ev_usrtrapret'):

#	Do the "evtrapret" system call.  Note that we should never
#	return from this system call.

	MCOUNT
	MOVW	&4,%r0			# Do the evtrapret system
	MOVW	&__evtrapret,%r1	# call.
	GATE				# ...
	jmp	_cerror			# Shouldn't ever come back.
