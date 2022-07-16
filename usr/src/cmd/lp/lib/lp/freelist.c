/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/lp/freelist.c	1.5"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "sys/types.h"
#include "stdlib.h"

/**
 ** freelist() - FREE ALL SPACE USED BY LIST
 **/

void
#if	defined(__STDC__)
freelist (
	char **			list
)
#else
freelist (list)
	char			**list;
#endif
{
	register char		**pp;

	if (list) {
		for (pp = list; *pp; pp++)
			free (*pp);
		free ((char *)list);
	}
	return;
}
