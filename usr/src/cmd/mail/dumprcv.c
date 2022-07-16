/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:mail/dumprcv.c	1.5"
#ident "@(#)dumprcv.c	2.8 'attmail mail(1) command'"
#include "mail.h"
/*
 * Put out H_RECEIVED lines if necessary, or
 * suppress their printing from the calling routine.
 */
void dumprcv (type, htype, didrcvlines, suppress, f)
register int	type;
register int	htype;
register int	*didrcvlines;
register int	*suppress;
register FILE	*f;
{
	int		rcvspot;	/* Place to put H_AFWDFROM lines */
	struct hdrs	*hptr;

	if (*didrcvlines == TRUE) {
		return;
	}

	rcvspot = pckrcvspot();
	if (rcvspot == -1) {
		return;
	}

	if (htype == H_RECEIVED) {
		*suppress = TRUE;
	}

	if ((htype >= 0) && (rcvspot != htype)) {
		return;
	}

	*didrcvlines = TRUE;
	hptr = hdrlines[H_RECEIVED].head;
	while (hptr != (struct hdrs *)NULL) {
		printhdr(type, H_RECEIVED, hptr, f);
		hptr = hptr->next;
	}
}
