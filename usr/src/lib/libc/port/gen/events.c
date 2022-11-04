/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/events.c	1.2"
#include	"synonyms.h"
#include	<sys/types.h>
#include	<sys/errno.h>
#include	<sys/evecb.h>
#include	<sys/hrtcntl.h>
#include	<sys/priocntl.h>
#include	<sys/signal.h>
#include	<sys/procset.h>
#include	<sys/events.h>
#include	<sys/evsyscall.h>


/*			File Contents
**			=============
**
**	This file contains the library routines which convert the
**	events function calls to the appropriate system calls.
**	The arguments to all of these functions are described on
**	the events manual pages.  A description of how this interface
**	works as well as the definitions of all of the interface
**	structures can be found in sys/fs/evsyscall.h.
*/


/*			Change History
**			==============
**
**	9/1/87	Original development by Steve Buroff.
*/


/*			Miscellaneous Definitions
**			=========================
**
**	I find the following handy.
*/

#define	reg	register


/*			External References
**			===================
**
**	We check errno in order to implement restartable system calls.
*/

extern int	errno;

/*			The __evpost Function
**			=====================
**
**	Function to post one or more events to event queues.
*/

int
__evpost(ver, elp, els, flags)
reg const evver_t	ver;
reg event_t *const	elp;
reg const int		els;
reg const int		flags;
{
	reg evsys_post_t	*ep;	/* A ptr to the		*/
					/* interface buffer.	*/
	reg int			rval;	/* Return value.	*/
	evsys_post_t		evsysb;	/* The interface	*/
					/* buffer.		*/

	/*	Set up the interface structure with the parameters from
	**	the function call.
	*/

	ep			= &evsysb;
	ep->evs_post_elp    	= elp;
	ep->evs_post_els	= els;
	ep->evs_post_flags  	= flags;

	/*	Now do the evsys.  Its return value is the return value
	**	for the function.  Keep trying if we get ERESTART.
	*/

	do {
		rval = evsys(EVS_EVPOST, ver, ep);
	} while(rval == -1  &&  errno == ERESTART);

	return(rval);
}

/*			The __evpoll Function
**			=====================
**
**	Function to poll to determine if an event expression is
**	satisfied.
*/

int
__evpoll(ver, cmd, elp, els, top)
reg const evver_t		ver;
reg const evpollcmds_t		cmd;
reg event_t *const		elp;
reg const int			els;
reg const hrtime_t *const	top;
{
	reg evsys_poll_t	*ep;	/* A ptr to the		*/
					/* interface buffer.	*/
	reg int			rval;	/* Return value.	*/
	evsys_poll_t		evsysb;	/* The interface	*/
					/* buffer.		*/

	/*	Set up the interface structure with the parameters from
	**	the function call.
	*/

	ep			= &evsysb;
	ep->evs_poll_cmd  	= cmd;
	ep->evs_poll_elp  	= elp;
	ep->evs_poll_els  	= els;
	ep->evs_poll_top	= top;

	/*	Now do the evsys.  Its return value is the return value
	**	for the function.  Keep trying if we get ERESTART.
	*/

	do {
		rval = evsys(EVS_EVPOLL, ver, ep);
	} while(rval == -1  &&  errno == ERESTART);

	return(rval);
}

/*			The __evpollmore Function
**			=========================
**
**	Function to poll after an evpoll or evtrap returned one or more
**	events with the EF_MORE flag set in ev_flags.
*/

int
__evpollmore(ver, elp, els)
reg const evver_t	ver;
reg event_t *const	elp;
reg const int		els;
{
	reg evsys_pollmore_t	*ep;	/* A ptr to the		*/
					/* interface buffer.	*/
	reg int			rval;	/* Return value.	*/
	evsys_pollmore_t	evsysb;	/* The interface	*/
					/* buffer.		*/

	/*	Set up the interface structure with the parameters from
	**	the function call.
	*/

	ep			= &evsysb;
	ep->evs_pollmore_elp  	= elp;
	ep->evs_pollmore_els  	= els;

	/*	Now do the evsys.  Its return value is the return value
	**	for the function.  Keep trying if we get ERESTART.
	*/

	do {
		rval = evsys(EVS_EVPOLLMORE, ver, ep);
	} while(rval == -1  &&  errno == ERESTART);

	return(rval);
}

/*			The __evtrap Function
**			=====================
**
**	Function to specify that a trap handler should be called when
**	a specified event expression is satisfied.
*/

int
__evtrap(ver, cmd, elp, els, tid, handlerp, tap)
reg const evver_t	ver;
reg const evpollcmds_t	cmd;
reg event_t *const	elp;
reg const int		els;
const long		tid;
reg void		(*const handlerp)();
const evta_t *const	tap;
{
	reg evsys_trap_t	*ep;	/* A ptr to the		*/
					/* interface buffer.	*/
	reg int			rval;	/* Return value.	*/
	evsys_trap_t		evsysb;	/* The interface	*/
					/* buffer.		*/

	void			ev_traptousr();
					/* The function which	*/
					/* will call the actual	*/
					/* user trap handler	*/
					/* routine.  The	*/
					/* ev_traptousr 	*/
					/* function gets 	*/
					/* control from the	*/
					/* kernel but not by a	*/
					/* normal call.  The	*/
					/* stack is in a 	*/
					/* special state (see	*/
					/* the evsys.s file).	*/

	/*	Set up the interface structure with the parameters from
	**	the function call.
	*/

	ep			= &evsysb;
	ep->evs_trap_cmd	= cmd;
	ep->evs_trap_elp	= elp;
	ep->evs_trap_els	= els;
	ep->evs_trap_tid	= tid;
	ep->evs_trap_lfunc	= ev_traptousr;
	ep->evs_trap_ufunc	= handlerp;
	ep->evs_trap_tap	= tap;

	/*	Now do the evsys.  Its return value is the return value
	**	for the function.  Try again if we get ERESTART.
	*/

	do {
		rval = evsys(EVS_EVTRAP, ver, ep);
	} while(rval == -1  &&  errno == ERESTART);

	return(rval);
}

/*			The __evtrapcancel Function
**			=============================
**
**	Function to cancel one or more active trap expressions with the
**	same trap identifier.
*/

int
__evtrapcancel(ver, tidp, tids)
reg const evver_t	ver;
reg long *const		tidp;
reg const int		tids;
{
	reg evsys_trapcan_t	*ep;	/* A ptr to the 	*/
					/* interface buffer.	*/
	reg int			rval;	/* Return value.	*/
	evsys_trapcan_t		evsysb;	/* The interface	*/
					/* buffer.		*/
	
	/*	Set up the interface structure with the parameters from
	**	the function call.
	*/

	ep			= &evsysb;
	ep->evs_trapcan_tidp	= tidp;
	ep->evs_trapcan_tids	= tids;

	/*	Now do the evsys.  Its return value is the return value
	**	for the function.  Keep trying if we get ERESTART.
	*/

	do {
		rval = evsys(EVS_EVTRAPCAN, ver, ep);
	} while(rval == -1  &&  errno == ERESTART);

	return(rval);
}

/*			The __evcntl Function
**			=====================
**
**	Function to perform event control operations which do not apply
**	to any particular event queue.
*/

int
__evcntl(ver, cmd, arg1, arg2)
reg const evver_t	ver;
reg const evcntlcmds_t	cmd;
reg const long		arg1;
reg const long		arg2;
{
	reg evsys_cntl_t	*ep;	/* A ptr to the		*/
					/* interface buffer.	*/
	reg int			rval;	/* Return value.	*/
	evsys_cntl_t		evsysb;	/* The interface	*/
					/* buffer.		*/

	/*	Set up the interface structure with the parameters from
	**	the function call.
	*/

	ep			= &evsysb;
	ep->evs_cntl_cmd 	= cmd;
	ep->evs_cntl_arg1	= arg1;
	ep->evs_cntl_arg2	= arg2;

	/*	Now do the evsys.  Its return value is the return value
	**	for the function.  Try again if we get ERESTART.
	*/

	do {
		rval = evsys(EVS_EVCNTL, ver, ep);
	} while(rval == -1  &&  errno == ERESTART);

	return(rval);
}

/*			The __evqcntl Function
**			======================
**
**	Function to perform event control operations which apply to a
**	particular event queue.
*/

int
__evqcntl(ver, eqd, cmd, arg)
reg const evver_t	ver;
reg const int		eqd;
reg const evqcntlcmds_t	cmd;
reg const long		arg;
{
	reg evsys_qcntl_t	*ep;	/* A ptr to the		*/
					/* interface buffer.	*/
	reg int			rval;	/* Return value.	*/
	evsys_qcntl_t		evsysb;	/* The interface	*/
					/* buffer.		*/

	/*	Set up the interface buffer with the parameters from
	**	the function call.
	*/

	ep			= &evsysb;
	ep->evs_qcntl_eqd   	= eqd;
	ep->evs_qcntl_cmd   	= cmd;
	ep->evs_qcntl_arg   	= arg;

	/*	Now do the evsys.  Its return value is the return value
	**	for the function.  Try again if we get ERESTART.
	*/

	do {
		rval = evsys(EVS_EVQCNTL, ver, ep);
	} while(rval == -1  &&  errno == ERESTART);

	return(rval);
}

/*			The __evexit Function
**			=====================
**
**	Function to cause an event to be posted when a process exits.
**	This is just a simple interface to evexitset.
*/

int
__evexit(ver, idtype, id, ecbp)
reg const evver_t	ver;
reg const idtype_t	idtype;
reg const id_t		id;
reg const ecb_t *const	ecbp;
{
	reg procset_t	*psp;	/* Ptr to the process set we	*/
				/* construct.			*/
	procset_t	pset;	/* The process set we are going	*/
				/* to construct.		*/
	
	/*	Build a procset_t structure which is equivalent to
	**	the simple request specified by idtype and id.
	*/

	psp		= &pset;
	psp->p_op	= POP_AND;
	psp->p_lidtype	= idtype;
	psp->p_lid	= id;
	psp->p_ridtype	= P_ALL;

	/*	Now just do an evexitset.
	*/

	return(__evexitset(ver, psp, P_MYHOSTID, ecbp));

}

/*			The __evexitset Function
**			========================
**
**	Function to cause an event to be posted when each of a set of
**	processes exits.
*/

int
__evexitset(ver, psp, hostid, ecbp)
reg const evver_t		ver;
reg const procset_t *const	psp;
reg const hostid_t		hostid;
reg const ecb_t *const		ecbp;
{
	reg evsys_exit_t	*ep;	/* A ptr to the		*/
					/* interface buffer.	*/
	reg int			rval;	/* Return value.	*/
	evsys_exit_t		evsysb;	/* The interface	*/
					/* buffer.		*/

	/*	Set up the interface structure with the parameters from
	**	the function call.
	*/


	ep			= &evsysb;
	ep->evs_exit_psp  	= psp;
	ep->evs_exit_hostid	= hostid;
	ep->evs_exit_ecbp	= ecbp;

	/*	Now do the evsys system call.  Try again if we get
	**	ERESTART.
	*/

	do {
		rval = evsys(EVS_EVEXIT, ver, ep);
	} while(rval == -1  &&  errno == ERESTART);

	return(rval);
}

/*			The __evsig Function
**			====================
**
**	Function to cause an event to be generated when a signal is sent
**	to a process.
*/

int
__evsig(ver, sigsetp, ecbp, silp, sils)
reg const evver_t		ver;
reg const sigset_t *const	sigsetp;
reg const ecb_t *const		ecbp;
reg evsiginfo_t *const		silp;
reg const int			sils;
{
	reg evsys_sig_t	*ep;	/* A ptr to the interface	*/
				/* buffer.			*/
	reg int		rval;	/* Return value.		*/
	evsys_sig_t	evsysb;	/* The interface buffer.	*/

	/*	Set up the interface structure with the parameters from
	**	the function call.
	*/


	ep			= &evsysb;
	ep->evs_sig_setp	= sigsetp;
	ep->evs_sig_ecbp	= ecbp;
	ep->evs_sig_silp	= silp;
	ep->evs_sig_sils	= sils;

	/*	Now do the evsys system call.  Try again if ERESTART is
	**	returned.
	*/

	do {
		rval = evsys(EVS_EVSIG, ver, ep);
	} while(rval == -1  &&  errno == ERESTART);

	return(rval);
}
