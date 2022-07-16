/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/lp/duplist.c	1.6"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "errno.h"
#include "string.h"
#include "stdlib.h"

#include "lp.h"

/**
 ** duplist() - DUPLICATE A LIST OF STRINGS
 **/

char **
#if	defined(__STDC__)
duplist (
	char **			src
)
#else
duplist (src)
	register char		**src;
#endif
{
	register char		**dst;

	register int		nitems,
				n;

	if (!src || !*src)
		return (0);

	nitems = lenlist(src);
	if (!(dst = (char **)malloc((nitems + 1) * sizeof(char *)))) {
		errno = ENOMEM;
		return (0);
	}

	for (n = 0; n < nitems; n++)
		if (!(dst[n] = strdup(src[n]))) {
			while (--n)
				free (dst[n]);
			free ((char *)dst);
			errno = ENOMEM;
			return (0);
		} 
	dst[nitems] = 0;

	return (dst);
}

