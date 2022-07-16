/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/filters/freefilter.c	1.9"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "stdlib.h"

#include "lp.h"
#include "filters.h"

/**
 ** freefilter() - FREE INTERNAL SPACE ALLOCATED FOR A FILTER
 ** free_filter() - FREE INTERNAL SPACE ALLOCATED FOR A _FILTER
 **/

static void
#if	defined(__STDC__)
freetypel (
	TYPE *			typel
)
#else
freetypel (typel)
	register TYPE		*typel;
#endif
{
	register TYPE		*pt;

	if (typel) {
		for (pt = typel; pt->name; pt++)
			free (pt->name);
		free ((char *)typel);
	}
	return;
}

void
#if	defined(__STDC__)
freetempl (
	TEMPLATE *		templ
)
#else
freetempl (templ)
	register TEMPLATE	*templ;
#endif
{
	register TEMPLATE	*pt;

	if (templ) {
		for (pt = templ; pt->keyword; pt++) {
			free (pt->keyword);
			if (pt->pattern)
				free (pt->pattern);
			if (pt->re)
				free (pt->re);
			if (pt->result)
				free (pt->result);
		}
		free ((char *)templ);
	}
	return;
}

void
#if	defined(__STDC__)
freefilter (
	_FILTER *		pf
)
#else
freefilter (pf)
	_FILTER			*pf;
#endif
{
	if (!pf)
		return;
	if (pf->name)
		free (pf->name);
	if (pf->command)
		free (pf->command);
	freelist (pf->printers);
	freetypel (pf->printer_types);
	freetypel (pf->input_types);
	freetypel (pf->output_types);
	freetempl (pf->templates);

	return;
}

void
#if	defined(__STDC__)
free_filter (
	_FILTER *		pf
)
#else
free_filter (pf)
	_FILTER			*pf;
#endif
{
	if (!pf)
		return;
	if (pf->name)
		free (pf->name);
	if (pf->command)
		free (pf->command);
	freelist (pf->printers);
	freetypel (pf->printer_types);
	freetypel (pf->input_types);
	freetypel (pf->output_types);
	freetempl (pf->templates);

	return;
}
