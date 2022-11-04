/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/waitpid.c	1.1"

#ifdef __STDC__
	#pragma weak waitpid = _waitpid
#endif
#include "synonyms.h"
#include <wait.h>

pid_t
waitpid(pid,stat_loc,options)
pid_t pid;
int *stat_loc;
int options;
{
	idtype_t idtype;
	id_t id;
	siginfo_t info;
	register error, stat;

	options |= (WEXITED|WTRAPPED);
	if (pid > 0) {
		idtype = P_PID;
		id = pid;
	} else if (pid < -1) {
		idtype = P_PGID;
		id = -pid;
	} else if (pid == -1) {
		idtype = P_ALL;
		id = 0;
	} else {
		idtype = P_PGID;
		id = getpgid(0);
	}

	error = waitid(idtype, id, &info, options);
	if (error < 0 || stat_loc == 0 || info.si_pid == 0)
		return error;

	stat = (info.si_status & 0377);

	switch (info.si_code) {
		case CLD_EXITED:
			stat <<= 8;
			break;
		case CLD_DUMPED:
			stat |= WCOREFLG;
			break;
		case CLD_KILLED:
			break;
		case CLD_TRAPPED:
		case CLD_STOPPED:
			stat <<= 8;
			stat |= WSTOPFLG;
			break;
		case CLD_CONTINUED:
			stat = WCONTFLG;
			break;
	}

	*stat_loc = stat;

	return info.si_pid;
}
