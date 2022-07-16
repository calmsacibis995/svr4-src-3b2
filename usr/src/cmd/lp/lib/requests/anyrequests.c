/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/requests/anyrequests.c	1.5"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "sys/types.h"

#include "lp.h"

/**
 ** anyrequests() - SEE IF ANY REQUESTS ARE ``QUEUED''
 **/

int
#if	defined(__STDC__)
anyrequests (
	void
)
#else
anyrequests ()
#endif
{
	long			lastdir		= -1;

	char *			name;


	/*
	 * This routine walks through the requests (secure)
	 * directory looking for files, descending one level
	 * into each sub-directory, if any. Finding at least
	 * one file means that a request is queued.
	 */
	while ((name = next_dir(Lp_Requests, &lastdir))) {

		long			lastfile	= -1;

		char *			subdir;


		if (!(subdir = makepath(Lp_Requests, name, (char *)0)))
			return (1);	/* err on safe side */

		if (next_file(subdir, &lastfile)) {
			free (subdir);
			return (1);
		}

		free (subdir);
	}
	return (0);
}
