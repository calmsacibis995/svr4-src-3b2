#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

	.ident	"@(#)kernel:ml/ttrap.s	1.29"
#
#   NOTE -- CHANGES TO USER.H, PROC.H AND CLASS.H MAY REQUIRE
#   CORRESPONDING CHANGES TO THE FOLLOWING .SETs.
#
	.set	u_iop,0x608	# offset of u.u_iop:
	.set	u_kpcb,0x70	# offset of u.u_kpcb:
	.set	u_ipsw,0x000	# offset of initial PSW in a pcb
	.set	u_isp,0x008	# offset of initial SP in a pcb
	.set	u_kpcb2,0x7c	# offset of u.u_kpcb:
	.set	u_pc,0x004	# offset of PC in a pcb
	.set	u_ipcb,0x0	# offset of u_ipcb
	.set	u_pcb,0xc	# offset of u.u_pcb:
	.set	u_pcbp,0x6c	# offset of u.u_pcbp:
	.set	u_psw,0x000	# offset of PSW in a pcb
	.set	u_r0tmp,0xc0	# offset of u.u_r0tmp:
	.set	u_stack,0x530	# offset of u.u_stack
	.set	u_sp,0x008	# offset of SP in a pcb
	.set	u_slb,0x00c	# offset of SLB in a pcb
	.set	u_sub,0x010	# offset of SUB in a pcb
	.set	u_ap,0x014	# offset of AP in a pcb
	.set	u_caddrflt,0x130 # offset to u_caddrflt
	.set	u_sigevpend,0x135 # offset to u_sigevpend
	.set	u_fpovr,0x4ac	# offset of u_fpovr
	.set	u_procp,0x4e4	# offset of u_procp
	.set	u_tracepc,0x1cc	# offset to u_tracepc
	.set	p_clproc,0xe0	# offset to p_clproc
	.set	p_clfuncs,0xe4	# offset to p_clfuncs
	.set	cl_trapret,0x4c	# offset to cl_trapret in classfuncs struct

	.set	PSW_TE,0x20000	# TM bit in psw.


#ifdef	KPERF
	.set	KPT_INTR, 1	# interrupt entry point
	.set	KPT_TRAP_RET, 2	# return from trap to user mode
	.set	KPT_INT_KRET, 3	# return from interrupt to kernel mode
	.set	KPT_INT_URET, 4	# return from interrupt to user mode
#endif	/* KPERF */


# *****
# Global symbols not defined in this file
# *****

	.globl	mmufltcr	# vuifile: address of MMU0 fault code reg.
	.globl	mmufltar	# vuifile: address of MMU0 address reg.
	.globl	intnull
	.globl	intpx		# trap.c: intpx( pcbp )
	.globl	intsx		# trap.c: intsx( pcbp )
	.globl	runrun
	.globl	u


#ifdef	KPERF
	.globl	kperf_write	# kperf.c: kperf_write(type, pc, kproc)
	.globl	kpftraceflg	# flag to enable Kernel PERFormance measurement
	.globl	int_flg		# flag to disdinquish trap_ret from int_ret
#endif	/* KPERF */

	.text

	.globl	int_null
int_null:
	CALL	-0(%sp),intnull
	jmp	int_ret


	.globl	kpcb_pswtch

#ifdef	KPERF
	.globl	KPswtch
KPswtch:
#else	/* !KPERF */
	.globl	swtch
swtch:
#endif	/* KPERF */
	MOVAW	kpcb_pswtch,%r0
	CALLPS
	RET


	.globl	callps
callps:
	MOVW	0(%ap),%r0
	CALLPS
	RET


	.globl	ps_swtch
	.globl	pswtch
ps_swtch:
	MOVW	-4(%isp),u+u_pcbp	# save current pcbp in u-block
	CALL	0(%sp),pswtch
	MOVW	u+u_pcbp,-4(%isp)	# establish pcbp for new process
	BRH	int_ret


	.globl	int_L9
 	.globl	pwrflag
	.globl	timeflag
	.globl	uartflag
	.globl	timein
	.globl	iupirint
 	.globl	intspwr
	.globl	sbdwcsr
int_L9:

#ifdef	KPERF

	TSTW	kpftraceflg		# is Kernel PERFormance enabled?
	je	int_notk_L9		# no, continue in main line
	PUSHW	&KPT_INTR		# call kperf_write(KPT_INTR,pc,curproc)
	MOVW	-4(%isp),%r0		# save process pcb
	PUSHAW	int_L9			# push current pc
	PUSHW	$curproc		# push pointer to current proc struct
	CALL	-12(%sp),kperf_write	# do it already
int_notk_L9:

#endif	/* KPERF */

	MOVB	&0,$sbdwcsr+(13*4)+3		# clear pir9
	TSTB	pwrflag
 	je	not_pwr
 	CLRB	pwrflag
 	CALL	0(%sp),intspwr
not_pwr:
	TSTB	uartflag
	je	not_uart
	CLRB	uartflag
	CALL	0(%sp),iupirint
not_uart:
	TSTB	timeflag
	je	not_timer
	CLRB	timeflag
	CALL	0(%sp),timein
not_timer:
	jmp	int_ret


	.globl	int_L15
	.globl	clock_int
	.globl	intsyserr
int_L15:
	BITH	&0x0040,$sbdrcsr+2
	BEB	not_clock
	TSTB	$clrclkint	# clear clock interrupt
	MOVW	-4(%isp),%r0	
	PUSHW	4(%r0)		# pc
	PUSHW	0(%r0)		# psw
	CALL	-8(%sp),clock_int	# call clock( pc, psw )

#	The following test is to see if the user is doing
#	profiling.  Clock will return non-zero if it wanted
#	to call addupc.  It could not because it was running on
#	an interrupt stack and addupc can get a page fault and
#	sleep.

	TSTW	%r0
	je	int_ret
	MOVW	&u+u_kpcb,-4(%isp)	# push ptr to kernel pcb.
	MOVW	&addupc_clk,u+u_kpcb+u_pc	# return to addupc_clk.
	MOVW	&u+u_kpcb,u+u_pcbp	# remember which pcb we are on.
	RETPS				# off to addupc_clk.


#	Come here for all level 15 interrupts except clock.

not_clock:
	CALL	0(%sp),intsyserr
	jmp	int_ret


	.globl	int_ret
	.globl	u400

int_ret:
	BITW	&0x1800,*-4(%isp)	# Returning to user-mode?

#ifdef	KPERF
	je	int_kret		# Record transition before RETPS.
#else	/* !KPERF */
	je	int_retps		# No, simply RETPS.
#endif	/* KPERF */

# Still executing on an interrupt-PCB, time to switch to kernel-PCB.

int_ret2:


#ifdef	KPERF

	TSTW	kpftraceflg		# is Kernel PERFormance enabled?
	je	int_notk_ret2		# no, resume in main line code
	PUSHW	&KPT_INT_URET		# kperf_write(KPT_INT_URET,pc,curproc)
	MOVW	-4(%isp),%r0		# save process pcb
	PUSHW	u_pc(%r0)		# push current pc
	PUSHW	$curproc		# push pointer to current proc struct
	CALL	-12(%sp),kperf_write	# so call it already
	INCW	int_flg			# set int_flg so we know we from int_ret

int_notk_ret2:

#endif /* KPERF */
	MOVW	&u+u_kpcb2,%pcbp	# Change PCB-pointer.
	MOVW	u_slb(%pcbp),%sp	# Set SP to lower bound.
	MOVW	%pcbp,u+u_pcbp		# Also change user-pcbp.
	SUBW2	&4,%isp			# Get off interrupt-stack.



# This is the entry point for trap.c routines that return to the
# user.  The stack exception handler also goes through here.
# At this point, we are executing on the kernel-PCB.
# All exception/GATE handlers enter here since they run on the
#   kernel-PCB.

	.globl	trap_ret

trap_ret:


	MOVAW	trap_ret2,u+u_caddrflt	# In case addr really bad.
	TSTB	*u+u_pcb+u_pc		# Fetch required byte.


	.globl	trap_ret2
	.globl	kpcb_qrun
	.globl	qrunflag
	.globl	queuerun
	.globl	spsave

trap_ret2:

	CLRW	u+u_caddrflt		# Clear for next time.

# don''t run queues if returning to address in sections 0 or 1.
	CMPW	&0x80000000,u+u_pcb+u_pc
	BLUB	trap_ret3

# kick off streams queue scheduling.

	MOVW	%psw,%r0
	ORW2	&0x1e000,%psw		# entering critical section
	TSTB	qrunflag
	je	not_qrun		# don''t run if no enabled queues
	TSTB	queueflag
	jne	not_qrun		# don''t run if already in queuerun()
	INCB	queueflag		# block another call to queuerun()
	MOVW	&kpcb_qrun+u_pcb,%pcbp	# change to qrun pcb
	MOVW	%sp,spsave		# save current stack pointer
	MOVW	u_slb(%pcbp),%sp	# change to qrun stack
	MOVW	%pcbp,u+u_pcbp		# change user pcbp
	MOVW 	%r0,%psw		# leaving critical section
	CALL	0(%sp),queuerun
	MOVW	&u+u_kpcb2,%pcbp	# back to default kernel pcb
	MOVW	spsave,%sp		# and back to the saved sp
	MOVW	%pcbp,u+u_pcbp		# reset user pcbp
	CLRB	queueflag		# allow calls to queuerun()
	jmp	trap_ret3

	.globl	not_qrun

not_qrun:
	MOVW	%r0,%psw		# leaving critical section

	.globl	trap_ret3

trap_ret3:

#ifdef	KPERF
	TSTW	int_flg			# are we from int_ret?
	jne	trap_ret3A		# if yes, do not record KPT_TRAP_RET

	TSTW	kpftraceflg		# is Kernel PERFormance enabled?
	je	trap_ret3A		# no, skip to main line code
	PUSHW	&KPT_TRAP_RET		# kperf_write(KPT_TRAP_RET,pc,curproc)
	PUSHW	u+u_pcb+u_pc		# push current pc
	PUSHW	$curproc		# push pointer to current proc struct
	CALL	-12(%sp),kperf_write	# so call it already

trap_ret3A:
        CLRW	int_flg			# clear the flag for next time	
#endif	/* KPERF */

	MOVW	$curproc,%r0
	PUSHW	p_clproc(%r0)		# Push first arg to cl_trapret
	MOVW	p_clfuncs(%r0),%r0	# Locate appropriate classfuncs struct
	MOVW	cl_trapret(%r0),%r0	# Get the cl_trapret function address
	CALL	-4(%sp),0(%r0)		# Class specific cl_trapret call

# We need to take a final look just prior to returning to user mode to see
# if we need to do signal processing or preemption.  If we are returning from
# an interrupt taken while in user mode this is the only check that is done.
# If we are returning from any of the trap.c routines we have already done
# these checks but it is possible we have been hit with an interrupt since
# that time that caused a signal to be posted or runrun to be set.  We can''t
# afford to go back to the user with pending signals or a pending preemption
# request so we block out interrupts for a short time and check.  If signal
# processing or preemption is required it is done by s_trap().  If s_trap()
# is called it will return via trap_ret so we will be back here again to
# make another check before returning to the user.

	.globl	trap_ret4

trap_ret4:
	MOVW	%psw,%r0
	ORW2	&0x1e000,%psw		# splhi

	.globl	trap_ret5
	.globl	strap
	.globl	no_strap
	.globl	s_trap

trap_ret5:
	TSTW	runrun
	jne	strap
	TSTB	u+u_sigevpend
	je	no_strap

strap:
	MOVW	%r0,%psw		# spl back to old level
	CALL	0(%sp),s_trap
	jmp	trap_ret3


no_strap:
	ORW2	&0x100,u+u_pcb+u_psw	# Set the R flag.
	CLRW	u+u_tracepc		# Assume no tracing.
	BITW	&PSW_TE,u+u_pcb+u_psw	# Is user tracing?
	BEB	trap_ret6		# Branch if not.
	MOVW	u+u_pcb+u_pc,u+u_tracepc # Save pc of expected trace trap.

# Push user-PCB onto the interrupt stack.

	.globl	trap_ret6

trap_ret6:
#ifdef KPERF
	.globl  int_kret               
#endif /* KPERF */
	ADDW2	&4,%isp			# Bump isp pointer.
	MOVW	&u+u_pcb,-4(%isp)	# move user-PCB on stack.
	MOVW	&u+u_pcb,u+u_pcbp	# Update ublock field.


#ifdef	KPERF
	BRB	int_retps		# skip to main line

int_kret:
	TSTW	kpftraceflg		# is Kernel PERFormance enabled?
	je	int_retps		# no, skip to main line
	PUSHW	&KPT_INT_KRET		# kperf_write(KPT_INT_KRET,pc,curproc)
	MOVW	-4(%isp),%r0		# get pcb in r0
	PUSHW	u_pc(%r0)		# push current pc
	PUSHW	$curproc		# push pointer to current proc struct
	CALL	-12(%sp),kperf_write	# so call it already and fall thru....
#endif	/* KPERF */


# Now all we do is return to the process.

int_retps:
	RETPS


# *****************************************************************
# PROCESS EXCEPTION PROCESSING
#
# This section has one entry point: int_px
#
# A process exception is VERY BAD.  UNIX will attempt to print
# a little diagnostic information and will panic.
#
	.globl	int_px
int_px:
	PUSHW	%psw
	PUSHW	-4(%isp)
	CALL	-8(%sp),intpx
	BRH	int_ret		# actually, we never get here


# *****************************************************************
# STACK EXCEPTION PROCESSING
#
# This section has one entry point: int_sx
#
# Since a stack exception means that the stack may be corrupt,
# the hardware does a complete process switch to service a stack
# fault.  The complete process state is saved in *%pcbp, and the
# pcbp is pushed on the interrupt stack.  All we have to do to be
# in the same state as a normal exception after its CALLPS, is
# change the current pcbp, and stack pointer.
# BUT FIRST, we must check that we were not already on the u_block
# stack (this is a fatal error).
#
# Control Flow: this code represents a partial integration of the
#    Schan finesse.  Stack processing is divided into two parts,
#    one which takes care not to sleep() and one which can.
#    Actually, with this implementation, this is not necessary.
#
#    IF (previous process was not running on user stack) {
#	intsxk( %isp[-1] );	/* panic with useful message */
#	/*NOTREACHED*/
#    } ELSE {
#	%pcbp   := &u.u_kpcb.psw; /* set past initial PC, PSW, SP */
#	u.u_pcbp:= &u.u_kpcb + 12;
#	%sp     := &u.u_stack;
#	/* the PSW loaded getting here is fine */
#	/* we are now in the same state as nrmx, after the CALLPS */
#	intsx( %isp[-1] );
#	jmp int_ret
#    }
#	    
	.globl	int_sx
int_sx:
	CMPW	&u+u_pcb,-4(%isp)	# check previous pcbp
	BEB	sx_user			# jump if WAS user
	PUSHW	-4(%isp)
	CALL	-4(%sp),intsxk		# trouble!
	BRH	int_ret			# if we do not panic...

sx_user:
	MOVW	&u+u_kpcb2,%pcbp	# simulate init. pcb processing
	MOVW	u+u_stack,%sp
	MOVW	&u+u_kpcb2,u+u_pcbp	# keep u.u_pcbp == %pcbp

	PUSHW	-4(%isp)		# intsx( faulting pcbp )
	SUBW2	&4,%isp			# get off the interrupt stack
	CALL	-4(%sp),intsx


# *****************************************************************
# SYSTEM CALL PROCESSING
#
# This section has one entry point: Xsyscall
#
# Control Flow: Since we know that the kernel never GATEs to itself,
#    on entry to this section we are sure to be on the user stack.
#    Also, u.u_kpcb.ipc already points to systrap(), since that is the
#    default case.
#	u.u_r0tmp = %r0;
#	u.u_pcbp  = &u.u_kpcb.psw; /* value of %pcbp AFTER callps */
#	%r0       = &u.u_kpcb;
#	CALLPS;
#	/*NOTREACHED*/
# Optimization: since this code sequence is so similar to that of
#    normal exception handling, the Xsyscall entry is defined in the
#    middle of the normal exception code.

# *****************************************************************
# NORMAL EXCEPTION PROCESSING
#
# This section has four entry points:
#	nrmx_XX  -- standard entry point for normal exceptions
#	nrmx_iop -- special case for illegal opcode (to improve the
#		performance of the software floating-point emulation.
#	nrmx_ilc -- RETG workaround entry point.
#	nrmx_YY  -- either a hardware failure, or a user doing a GATE
#		with invalid arguments. This is distinguished from
#		nrmx_XX entry at the C level.
#
# Control Flow:
#	IF ( normal exception occured while on user stack ) {
#
#		u.u_kpcb.ipc = &u_trap;
#		u.u_r0tmp    = %r0;
#		u.u_pcbp     = &u.u_kpcb.psw;
#		%r0          = &u.u_kpcb;
#		CALLPS;
#		/*NOTREACHED*/
#		}
#	ELSE { /* exception occured while on kernel stack */
#	       /* argument to k_trap is r0ptr whose value */
#	       /* is such that r0ptr[PC] == PC and	  */
#	       /* r0ptr[PS] = PSW.  Note that other regs */
#	       /* are not accessible this way.		  */
#
#		push saved PC, %r0, %r1, %r2, r0ptr;
#		k_trap(r0ptr);
#		pop r0ptr, %r2, %r1, %r0;
#		copy duplicate PC back over original; /* caddrflt */
#		RETG;
#		}
#
# Stack Picture:
# On entry, we may be running on either the user or kernel stack.
# The only pieces of stack that are sure to be here are:
#
#	%sp:	|		| <- this word may not be paged in
#		+---------------+
#		| saved PSW	| <- MUST be paged in
#		+---------------+
#		| saved PC	| <- MUST be paged in
#		+---------------+
#		| rest of stack	| <- this word may not be paged in

#
# RETG workaround.  RETG is not restartable if the stack is present and the
#	user text page is not.  An illegal level change is forced in the PSW
#	passed to the user signal handler.
#
	.globl	nrmx_ilc
	.globl	nrmx_ilc2
	.globl	nrmx_r0save
	.globl	nrmx_r1save
	.globl	nrmx_r2save
	.globl	fixuserpsw

	.set	ilcframe,8
	.set	retgframe,8
	.set	argpframe,12
	.set	ucontext,256

	.set	sigframe,ilcframe+retgframe+argpframe+ucontext

nrmx_ilc:
	MOVW	%r0,nrmx_r0save		# Save r0.
	MOVW	%r1,nrmx_r1save		# Save r1.
	MOVW	%r2,nrmx_r2save		# Save r2.

	ANDW3	-12(%sp),&0x00001000,%r0	# check if super or exec
	BEB	signal_ilc

	MOVW	-16(%sp),u+u_ipcb+u_pc		# normal stack frame
	MOVW	-12(%sp),u+u_ipcb+u_psw		# floating point code
	MOVAW	-16(%sp),u+u_ipcb+u_sp
	PUSHAW	u+u_ipcb+u_psw
	CALL	-4(%sp),fixuserpsw		# fix the users psw
	BRB	ilcskip

signal_ilc:
	PUSHAW	-sigframe(%sp)		# push address of user context
	PUSHW	-20(%sp)		# push possibly corrected pc value
	CALL	-8(%sp),retsig		# restore user context after RETG
	MOVW	%r0,%ap			# returns AP at time of sendsig

ilcskip:
	MOVW	nrmx_r0save,%r0		# Restore r0.
	MOVW	nrmx_r1save,%r1		# Restore r1.
	MOVW	nrmx_r2save,%r2		# Restore r2.

	ORW2	&0x00000080,u+u_ipcb+u_psw	# I=1
	TSTW	u+u_fpovr		# user trapping on ovrflow ?
	BEB	check_oe		# branch if not.
	ORW2	&0x00400000,u+u_ipcb+u_psw	# restore OE bit

#	Chip bug work-around: if the OE bit is on, the V bit must be
#	off. Otherwise the chip will get upset and UNIX will panic
#	with a process exception.
#
check_oe:
	BITW	&0x00400000,u+u_ipcb+u_psw	# is the OE bit on ?
	BEB	nrmx_ilc2			# branch if not
	ANDW2	&0xfff7ffff,u+u_ipcb+u_psw	# Turn off V bit.

#	The following label must be immediately before the TSTB
#	instruction because the RETG instruction is not restartable.
#	This label is referenced in trap.c/u_trap.
#
#	DON''T MOVE THIS LABEL.
#

nrmx_ilc2:
	TSTB	*u+u_ipcb+u_pc		# Fetch the pc''s page.
	CLRW	u+u_tracepc		# Assume no tracing.
	BITW	&PSW_TE,u+u_ipcb+u_psw	# Is user tracing?
	BEB	nrmx_ilc3		# Branch if not.
	MOVW	u+u_ipcb+u_pc,u+u_tracepc # Save pc of expected trace trap.

	.globl	nrmx_ilc3
nrmx_ilc3:
	ADDW2	&4,%isp
	MOVW	&u+u_ipcb,-4(%isp)		# Initial user-PCB
	RETPS

	.data
	.align	4

nrmx_r0save:
	.word	0
nrmx_r1save:
	.word	0
nrmx_r2save:
	.word	0
spsave:
	.word	0
#ifdef KPERF
int_flg:
        .word	0
#endif /* KPERF */
	.text
#
# Fast illegal op-code interface to handler for floating point emulation
# An illegal level change is forced which is also done for old signals.
# We distinguish which change it is by forcing supervisor mode for
# iop and exec for signals.
#
	.globl	nrmx_iop
	.globl	nrmx_iop2

nrmx_iop:
	BITW	&0x1800,-4(%sp)		# in kernel mode?
	je	nrmx_XX			# ...yes

#	Falling through: retry instruction in user SIGILL handler.

	TSTW	u+u_iop			# u_iop enabled?
	je	nrmx_XX			# ...no

	ANDW2	&0xfffff7ff,-4(%sp)	# Set CM = 2 (supervisor).
	MOVW	u+u_iop,u+u_ipcb+u_pc	# PC of FP routine.
	MOVW	-4(%sp),u+u_ipcb+u_psw	# Move in psw.
	MOVW	%sp,u+u_ipcb+u_sp		# Move in stack pointer.
	ANDW2	&0xfffffeff,u+u_ipcb+u_psw	# Turn off R bit.
	ORW2	&0x00001e80,u+u_ipcb+u_psw	# I=1, PM & CM = user.

#	This label must be here too.  See the comment at nrmx_ilc2
#	above.

nrmx_iop2:
	TSTB	*u+u_iop		# Fetch page.
	CLRW	u+u_tracepc		# Assume no tracing.
	BITW	&PSW_TE,u+u_ipcb+u_psw	# Is user tracing?
	BEB	nrmx_iop3		# Branch if not.
	MOVW	u+u_ipcb+u_pc,u+u_tracepc # Save pc of expected trace trap.

	.globl	nrmx_iop3
nrmx_iop3:
	ADDW2	&4,%isp			# Bump interrupt stack pointer.
	MOVW	&u+u_ipcb,-4(%isp)	# Point to initial pcb.
	RETPS				# Off to F.P. handler.

#
# All other normal exception handling
#
	.globl	nrmx_XX
	.globl	nrmx_YY
	.globl	Xsyscall

nrmx_XX:	# usual normal exception entry
nrmx_YY:	# invalid gate from user mode (detected later)
	CMPW	&u+u_pcb,%pcbp		# were we on kernel stack?
	jne	nrmx_KK			# ...yes: jump to special code
	MOVW	&u_trap,u+u_kpcb+u_pc	# execution point after CALLPS

Xsyscall:
	MOVW	&u+u_kpcb2,u+u_pcbp	# keep u.u_pcbp == %pcbp
	MOVW	%r0,u+u_r0tmp		# save %r0 before modifying
	MOVW	&u+u_kpcb,%r0
	CALLPS				# switch stacks & enter kernel

#
# Handling an exception while already on the kernel stack calls for
# special treatment, since the kernel (IF it panics) is going to
# use r0ptr[PC] and r0ptr[PS] for error reporting.
#
# Just before the call to k_trap, the stack looks like:
#		|		|
#		+---------------+
#		|		| <-- r0ptr points here
#		+---------------+
#	%sp:	|		|
#		+---------------+
#		| r0ptr        	|
#		+---------------+
#		| saved %r2	|
#		+---------------+
#		| saved %r1	|
#		+---------------+
#		| saved %r0	|
#		+---------------+
#		| copy of PC	|	r0ptr[ PC ] == r0ptr[-6]
#		+---------------+
#	GATE:	| saved PSW	|	r0ptr[ PS ] == r0ptr[-7]
#		+---------------+
#		| saved PC	|
#		+---------------+
#		| rest of stack	|
#
	.data
	.align 4
nrmx_pcb:
	.word 16

	.text

	.globl	nrmx_KK

nrmx_KK:	# normal exception while on kernel stack
	PUSHW	-8(%sp)		# copy saved PC above saved PSW

	PUSHW	%r0		# save registers that SAVE doesn''t
	PUSHW	%r1
	PUSHW	%r2

	PUSHAW	8(%sp)		# arg so that r0ptr[PC] == PC and
#				  r0ptr[PS] == PSW.

	CALL	-4(%sp),k_trap
	jz	nrmx_KK_ok

	MOVW -5*4(%sp), nrmx_pcb+0x00	# psw
	MOVW -6*4(%sp), nrmx_pcb+0x04	# pc
	MOVAW -6*4(%sp), nrmx_pcb+0x08	# sp
	MOVW %ap,  nrmx_pcb+0x14	# ap
	MOVW %fp,  nrmx_pcb+0x18	# fp
	MOVW -3*4(%sp),  nrmx_pcb+0x1c	# r0
	MOVW -2*4(%sp),  nrmx_pcb+0x20	# r1
	MOVW -1*4(%sp),  nrmx_pcb+0x24	# r2
	MOVW %r3,  nrmx_pcb+0x28
	MOVW %r4,  nrmx_pcb+0x2c
	MOVW %r5,  nrmx_pcb+0x30
	MOVW %r6,  nrmx_pcb+0x34
	MOVW %r7,  nrmx_pcb+0x38
	MOVW %r8,  nrmx_pcb+0x3c

	MOVAW nrmx_pcb+0x1c, save_r0ptr

	PUSHW	-0x14(%sp)	# push saved PSW
	CALL	-4(%sp),krnlflt

#	don't expect krnlflt to ever return


nrmx_KK_ok:
	POPW	%r2		# restore user registers
	POPW	%r1
	POPW	%r0

	POPW	-12(%sp)	# copy PC back (in case of change)
	TSTB	u400
	jz	nrmx_KK_1
	CFLUSH

nrmx_KK_1:
	RETG

#####################################################################################
#  routine to return to the firmware on panics etc.

	.globl rtnfirm
rtnfirm:
	MOVB	&1, $sbdwcsr+(2*4)+3
	BRB	.
