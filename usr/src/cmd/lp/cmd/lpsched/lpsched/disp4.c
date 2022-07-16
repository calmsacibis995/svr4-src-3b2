/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:cmd/lpsched/disp4.c	1.13.1.2"

#include "time.h"

#include "dispatch.h"

/**
 ** s_accept_dest()
 **/

void
#if	defined(__STDC__)
s_accept_dest (
	char *			m,
	MESG *			md
)
#else
s_accept_dest (m, md)
	char			*m;
	MESG			*md;
#endif
{
	char			*destination;

	ushort			status;

	register PSTATUS	*pps;

	register CSTATUS	*pcs;


	getmessage (m, S_ACCEPT_DEST, &destination);

	/*
	 * Have we seen this destination as a printer?
	 */
	if ((pps = search_ptable(destination)))
		if ((pps->status & PS_REJECTED) == 0)
			status = MERRDEST;
		else {
			pps->status &= ~PS_REJECTED;
			(void) time (&pps->rej_date);
			dump_pstatus ();
			status = MOK;
		}

	/*
	 * Have we seen this destination as a class?
	 */
	else if ((pcs = search_ctable(destination)))
		if ((pcs->status & CS_REJECTED) == 0)
			status = MERRDEST;
		else {
			pcs->status &= ~CS_REJECTED;
			(void) time (&pcs->rej_date);
			dump_cstatus ();
			status = MOK;
		}

	else
		status = MNODEST;

	mputm (md, R_ACCEPT_DEST, status);
	return;
}

/**
 ** s_reject_dest()
 **/

void
#if	defined(__STDC__)
s_reject_dest (
	char *			m,
	MESG *			md
)
#else
s_reject_dest (m, md)
	char			*m;
	MESG			*md;
#endif
{
	char			*destination,
				*reason;

	ushort			status;

	register PSTATUS	*pps;

	register CSTATUS	*pcs;


	getmessage (m, S_REJECT_DEST, &destination, &reason);

	/*
	 * Have we seen this destination as a printer?
	 */
	if ((pps = search_ptable(destination)))
		if (pps->status & PS_REJECTED)
			status = MERRDEST;
		else {
			pps->status |= PS_REJECTED;
			(void) time (&pps->rej_date);
			load_str (&pps->rej_reason, reason);
			dump_pstatus ();
			status = MOK;
		}

	/*
	 * Have we seen this destination as a class?
	 */
	else if ((pcs = search_ctable(destination)))
		if (pcs->status & CS_REJECTED)
			status = MERRDEST;
		else {
			pcs->status |= CS_REJECTED;
			(void) time (&pcs->rej_date);
			load_str (&pcs->rej_reason, reason);
			dump_cstatus ();
			status = MOK;
		}

	else
		status = MNODEST;

	mputm (md, R_REJECT_DEST, status);
	return;
}

/**
 ** s_enable_dest()
 **/

void
#if	defined(__STDC__)
s_enable_dest (
	char *			m,
	MESG *			md
)
#else
s_enable_dest (m, md)
	char			*m;
	MESG			*md;
#endif
{
	char			*printer;

	ushort			status;

	register PSTATUS	*pps;


	getmessage (m, S_ENABLE_DEST, &printer);

	/*
	 * Have we seen this printer before?
	 */
	if ((pps = search_ptable(printer)))
		if (enable(pps) == -1)
			status = MERRDEST;
		else
			status = MOK;
	else
		status = MNODEST;

	mputm (md, R_ENABLE_DEST, status);
	return;
}

/**
 ** s_disable_dest()
 **/

void
#if	defined(__STDC__)
s_disable_dest (
	char *			m,
	MESG *			md
)
#else
s_disable_dest (m, md)
	char			*m;
	MESG			*md;
#endif
{
	char			*destination,
				*reason,
				*req_id		= 0;

	ushort			when,
				status;

	register PSTATUS	*pps;


	getmessage (m, S_DISABLE_DEST, &destination, &reason, &when);

	/*
	 * Have we seen this printer before?
	 */
	if ((pps = search_ptable(destination))) {

		/*
		 * If we are to cancel a currently printing request,
		 * we will send back the request's ID.
		 * Save a copy of the ID before calling "disable()",
		 * in case the disabling loses it (e.g. the request
		 * might get attached to another printer). (Actually,
		 * the current implementation won't DETACH the request
		 * from this printer until the child process responds,
		 * but a future implementation might.)
		 */
		if (pps->request && when == 2)
			req_id = strdup(pps->request->secure->req_id);

		if (disable(pps, reason, (int)when) == -1) {
			if (req_id) {
				free (req_id);
				req_id = 0;
			}
			status = MERRDEST;
		} else
			status = MOK;

	} else
		status = MNODEST;

	mputm (md, R_DISABLE_DEST, status, NB(req_id));
	if (req_id)
		free (req_id);

	return;
}

/**
 ** s_load_filter_table()
 **/

void
#if	defined(__STDC__)
s_load_filter_table (
	char *			m,
	MESG *			md
)
#else
s_load_filter_table (m, md)
	char			*m;
	MESG			*md;
#endif
{
	ushort			status;


	trash_filters ();
	if (Loadfilters((char *)0) == -1)
		status = MNOOPEN;
	else {

		/*
		 * This is what makes changing filters expensive!
		 */
		queue_check (qchk_filter);

		status = MOK;
	}

	mputm (md, R_LOAD_FILTER_TABLE, status);
	return;
}

/**
 ** s_unload_filter_table()
 **/

void
#if	defined(__STDC__)
s_unload_filter_table (
	char *			m,
	MESG *			md
)
#else
s_unload_filter_table (m, md)
	char			*m;
	MESG			*md;
#endif
{
	trash_filters ();

	/*
	 * This is what makes changing filters expensive!
	 */
	queue_check (qchk_filter);

	mputm (md, R_UNLOAD_FILTER_TABLE, MOK);
	return;
}

/**
 ** s_load_user_file()
 **/

void
#if	defined(__STDC__)
s_load_user_file (
	char *			m,
	MESG *			md
)
#else
s_load_user_file (m, md)
	char			*m;
	MESG			*md;
#endif
{
	/*
	 * The first call to "getuser()" will load the whole file.
	 */
	trashusers ();

	mputm (md, R_LOAD_USER_FILE, MOK);
	return;
}

/**
 ** s_unload_user_file()
 **/

void
#if	defined(__STDC__)
s_unload_user_file (
	char *			m,
	MESG *			md
)
#else
s_unload_user_file (m, md)
	char			*m;
	MESG			*md;
#endif
{
	trashusers ();	/* THIS WON'T DO TRUE UNLOAD, SORRY! */

	mputm (md, R_UNLOAD_USER_FILE, MOK);
	return;
}

/**
 ** s_shutdown()
 **/

void
#if	defined(__STDC__)
s_shutdown (
	char *			m,
	MESG *			md
)
#else
s_shutdown (m, md)
	char			*m;
	MESG			*md;
#endif
{
	ushort			immediate;

	int			i;

	SSTATUS *		pss;


	(void)getmessage (m, S_SHUTDOWN, &immediate);

	switch (md->type) {

	case MD_UNKNOWN:	/* Huh? */
	case MD_BOUND:		/* MORE: Not sure about this one */
	case MD_MASTER:		/* This is us. */
		schedlog ("Received S_SHUTDOWN on a type %d connection\n", md->type);
		break;

	case MD_STREAM:
	case MD_SYS_FIFO:
	case MD_USR_FIFO:
		mputm (md, R_SHUTDOWN, MOK);
		lpshut (immediate);

	case MD_CHILD:
		/*
		 * A S_SHUTDOWN from a network child means that IT has
		 * shut down, not that WE are to shut down.
		 *
		 * We have to clear the message descriptor
		 * so we don't accidently try using it in the future.
		 * Unfortunately, this requires looking through the
		 * system table to see which network child died.
		 */		
		DROP_MD (md);
		if (SStatus) {
			for (i = 0; (pss = SStatus[i]); i++)
				if (pss->exec->md == md)
					break;
			if (pss) {
				schedlog (
					"Trying the connection again (request %s)\n",
					pss->exec->ex.request->secure->req_id
				);
				pss->exec->md = 0;
				resend_remote (pss, 0);
				schedule (EV_SYSTEM, pss);
			}
		}
		break;

	}

	return;
}

/**
 ** s_quiet_alert()
 **/

void
#if	defined(__STDC__)
s_quiet_alert (
	char *			m,
	MESG *			md
)
#else
s_quiet_alert (m, md)
	char			*m;
	MESG			*md;
#endif
{
	char			*name;

	ushort			type,
				status;

	register FSTATUS	*pfs;

	register PSTATUS	*pps;

	register PWSTATUS	*ppws;


	/*
	 * We quiet an alert by cancelling it with "cancel_alert()"
	 * and then resetting the active flag. This effectively just
	 * terminates the process running the alert but tricks the
	 * rest of the Spooler into thinking it is still active.
	 * The alert will be reactivated only AFTER "cancel_alert()"
	 * has been called (to clear the active flag) and then "alert()"
	 * is called again. Thus:
	 *
	 * For printer faults the alert will be reactivated when:
	 *	- a fault is found after the current fault has been
	 *	  cleared (i.e. after successful print or after manually
	 *	  enabled).
	 *
	 * For forms/print-wheels the alert will be reactivated when:
	 *	- the form/print-wheel becomes mounted and then unmounted
	 *	  again, with too many requests still pending;
	 *	- the number of requests falls below the threshold and
	 *	  then rises above it again.
	 */

	(void)getmessage (m, S_QUIET_ALERT, &name, &type);

	if (!*name)
		status = MNODEST;

	else switch (type) {
	case QA_FORM:
		if (!(pfs = search_ftable(name)))
			status = MNODEST;

		else if (!pfs->alert->active)
			status = MERRDEST;

		else {
			cancel_alert (A_FORM, pfs);
			pfs->alert->active = 1;
			status = MOK;
		}
		break;
		
	case QA_PRINTER:
		if (!(pps = search_ptable(name)))
			status = MNODEST;

		else if (!pps->alert->active)
			status = MERRDEST;

		else {
			cancel_alert (A_PRINTER, pps);
			pps->alert->active = 1;
			status = MOK;
		}
		break;
		
	case QA_PRINTWHEEL:
		if (!(ppws = search_pwtable(name)))
			status = MNODEST;

		else if (!ppws->alert->active)
			status = MERRDEST;

		else {
			cancel_alert (A_PWHEEL, ppws);
			ppws->alert->active = 1;
			status = MOK;
		}
		break;
	}
	
	mputm (md, R_QUIET_ALERT, status);
	return;
}

/**
 ** s_send_fault()
 **/

void
#if	defined(__STDC__)
s_send_fault (
	char *			m,
	MESG *			md
)
#else
s_send_fault (m, md)
	char			*m;
	MESG			*md;
#endif
{
	long			key;

	char			*printer,
				*alert_text;

	ushort			status;

	register PSTATUS	*pps;


	getmessage (m, S_SEND_FAULT, &printer, &key, &alert_text);

	if (
		!(pps = search_ptable(printer))
	     || !pps->exec
	     || pps->exec->key != key
	     || !pps->request
	)
		status = MERRDEST;

	else {
		printer_fault (pps, pps->request, alert_text, 0);
		status = MOK;
	}

	mputm (md, R_SEND_FAULT, status);
	return;
}
