/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:mail/dumpaff.c	1.5"
#ident "@(#)dumpaff.c	2.11 'attmail mail(1) command'"
#include "mail.h"
/*
 * Put out H_AFWDFROM and H_AFWDCNT lines if necessary, or
 * suppress their printing from the calling routine.
 */
void dumpaff (type, htype, didafflines, suppress, f)
register int	type;
register int	htype;
register int	*didafflines;
register int	*suppress;
register FILE	*f;
{
	int		affspot;	/* Place to put H_AFWDFROM lines */
	struct hdrs	*hptr;

	if (*didafflines == TRUE) {
		return;
	}

	affspot = pckaffspot();
	if (affspot == -1) {
		return;
	}

	switch (htype) {
	case H_AFWDCNT:
		*suppress = TRUE;
		return;
	case H_AFWDFROM:
		*suppress = TRUE;
		break;
	}

	if ((htype >= 0) && (affspot != htype)) {
		return;
	}

	*didafflines = TRUE;
	hptr = hdrlines[H_AFWDFROM].head;

	while (hptr != (struct hdrs *)NULL) {
		printhdr(type, H_AFWDFROM, hptr, f);
		hptr = hptr->next;
	}
	fprintf(f,"%s %d\n", header[H_AFWDCNT].tag, affcnt);
}
