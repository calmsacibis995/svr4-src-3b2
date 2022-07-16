/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/lp/sop.c	1.6"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdio.h"
#include "string.h"
#include "errno.h"
#include "stdlib.h"

#include "lp.h"

/**
 ** sop_up_rest() - READ REST OF FILE INTO STRING
 **/

char *
#if	defined(__STDC__)
sop_up_rest (
	FILE *			fp,
	char *			endsop
)
#else
sop_up_rest (fp, endsop)
	FILE			*fp;
	register char		*endsop;
#endif
{
	register int		size,
				lenendsop;

	register char		*str,
				*here;

	char			buf[BUFSIZ];


	str = 0;
	size = 0;
	if (endsop)
		lenendsop = strlen(endsop);

	while (fgets(buf, BUFSIZ, fp)) {
		if (endsop && STRNEQU(endsop, buf, lenendsop))
			break;
		size += strlen(buf);
		if (str)
			str = realloc(str, size + 1);
		else
			here = str = malloc(size + 1);
		if (!str) {
			errno = ENOMEM;
			return (0);
		}
		strcpy (here, buf);
		here = str + size;
	}
	if (ferror(fp)) {
		free (str);
		return (0);
	}
	return (str);
}
