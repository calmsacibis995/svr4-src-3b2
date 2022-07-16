/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/requests/freerequest.c	1.5"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "sys/types.h"
#include "stdlib.h"

#include "lp.h"
#include "requests.h"

/**
 ** freerequest() - FREE STRUCTURE ALLOCATED FOR A REQUEST STRUCTURE
 **/

void
#if	defined(__STDC__)
freerequest (
	REQUEST *		reqbufp
)
#else
freerequest (reqbufp)
	register REQUEST	*reqbufp;
#endif
{
	if (!reqbufp)
		return;
	if (reqbufp->destination)
		free (reqbufp->destination);
	if (reqbufp->file_list)
		freelist (reqbufp->file_list);
	if (reqbufp->form)
		free (reqbufp->form);
	if (reqbufp->alert)
		free (reqbufp->alert);
	if (reqbufp->options)
		free (reqbufp->options);
	if (reqbufp->pages)
		free (reqbufp->pages);
	if (reqbufp->charset)
		free (reqbufp->charset);
	if (reqbufp->modes)
		free (reqbufp->modes);
	if (reqbufp->title)
		free (reqbufp->title);
	if (reqbufp->input_type)
		free (reqbufp->input_type);
	if (reqbufp->user)
		free (reqbufp->user);
	return;
}
