/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:cmd/lpsched/checkchild.c	1.2"

#include	"lpsched.h"

/**
 ** ev_checkchild() - CHECK FOR DECEASED CHILDREN
 **/

#if	defined(CHECK_CHILDREN)

void
#if	defined(__STDC__)
ev_checkchild (
	void
)
#else
ev_checkchild ()
#endif
{
	register EXEC		*ep	= &Exec_Table[0],
				*epend	= &Exec_Table[ET_Size];


	/*
	 * This routine is necessary to find out about child
	 * processes that disappear without a trace. An example
	 * of how they might disappear: kill -9 pid.
	 * To minimize a race condition with a dying child,
	 * we don't mark the child as gone unless it hasn't been
	 * seen for two cycles.
	 */
	for ( ; ep < epend; ep++)
		if (ep->pid > 0 && kill(ep->pid, 0) == -1)
			if (ep->flags & EXF_GONE) {
				ep->pid = -99;
				ep->status = SIGTERM;
				ep->errno = 0;
				DoneChildren++;
#if	defined(DEBUG)
	    if (debug) {
		FILE			*fp	= open_logfile("exec");

		time_t			now = time((time_t *)0);

		char			buffer[BUFSIZ];

		extern char		*ctime();


		if (fp) {
			setbuf (fp, buffer);
			fprintf (
				fp,
			"LOST! %24.24s slot %d, pid %d, type %d%s%s\n",
				ctime(&now),
				ep - Exec_Table,
				ep->pid,
				ep->type,
				(ep->type == EX_INTERF? ", req " : ""),
				(ep->type == EX_INTERF?
				    ep->ex.request->secure->req_id : "")
			);
			close_logfile (fp);
		}
	    }
#endif
			} else
				ep->flags |= EXF_GONE;

	schedule (EV_LATER, WHEN_CHECKCHILD, EV_CHECKCHILD);
	return;
}

#endif
