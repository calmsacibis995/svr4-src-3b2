/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:mail/pickFrom.c	1.5"
#ident "@(#)pickFrom.c	2.5 'attmail mail(1) command'"
#include "mail.h"
/*
 * pickFrom (line) - scans line, ASSUMED TO BE of the form
 *	[>]From <fromU> <date> [remote from <fromS>]
 * and fills fromU and fromS global strings appropriately.
 */

void pickFrom (lineptr)
register char *lineptr;
{
	register char *p;
	static char rf[] = "remote from ";
	int rfl;

	if (*lineptr == '>')
		lineptr++;
	lineptr += 5;
	for (p = fromU; *lineptr; lineptr++) {
		if (isspace (*lineptr))
			break;
		*p++ = *lineptr;
	}
	*p = '\0';
	rfl = strlen (rf);
	while (*lineptr && strncmp (lineptr, rf, rfl))
		lineptr++;
	if (*lineptr == '\0') {
		fromS[0] = 0;
	} else {
		lineptr += rfl;
		for (p = fromS; *lineptr; lineptr++) {
			if (isspace (*lineptr))
				break;
			*p++ = *lineptr;
		}
		*p = '\0';
	}
}
