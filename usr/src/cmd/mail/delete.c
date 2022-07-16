/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:mail/delete.c	1.5"
#ident "@(#)delete.c	2.7 'attmail mail(1) command'"
#include "mail.h"
/*
	signal catching routine --- reset signals on quits and interupts
	exit on other signals
		i	-> signal #
*/
void delete(i)
register int i;
{
	static char pn[] = "delete";
	setsig(i, delete);

	if (i > SIGQUIT) {
		fprintf(stderr, "%s: ERROR signal %d\n",program,i);
		Dout(pn, 0, "caught signal %d\n", i);
	} else {
		fprintf(stderr, "\n");
	}

	if (delflg && (i==SIGINT || i==SIGQUIT)) {
		longjmp(sjbuf, 1);
	}
	done(0);
}
