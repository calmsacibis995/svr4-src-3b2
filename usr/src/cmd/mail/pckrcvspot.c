/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:mail/pckrcvspot.c	1.4"
#ident "@(#)pckrcvspot.c	2.5 'attmail mail(1) command'"
#include "mail.h"
/*
 * If any H_RECEIVED lines in msg, decide where to put them.
 * Returns :
 *	-1 ==> No H_RECEIVED lines to be printed.
 *	> 0 ==> Header line type after (before) which to place H_RECEIVED lines
 */
pckrcvspot()
{
	static char pn[] = "pckrcvspot";
	register int	rc;	

	if (hdrlines[H_RECEIVED].head == (struct hdrs *)NULL) {
		rc = -1;
	} else if (orig_rcv) {
		rc = H_RECEIVED;
	} else if (orig_aff) {
		rc = H_AFWDFROM;
	} else if (fnuhdrtype == H_RVERS) {
		if (hdrlines[H_EOH].head != (struct hdrs *)NULL) {
			if (hdrlines[H_DATE].head != (struct hdrs *)NULL) {
				rc = H_DATE;
			} else {
				rc = H_EOH;
			}
		}
	} else if ((fnuhdrtype == H_MVERS) &&
	    (hdrlines[H_EOH].head != (struct hdrs *)NULL)) {
		rc = H_EOH;
	} else {
		rc = H_CTYPE;
	}
	Dout(pn, 3, "'%s'\n", (rc == -1 ? "No Received: lines": header[rc].tag));
	return (rc);
}
