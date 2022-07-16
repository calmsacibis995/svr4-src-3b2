/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:mail/setsig.c	1.4"
#ident "@(#)setsig.c	2.4 'attmail mail(1) command'"
#include "mail.h"
/*
	Signal reset
	signals that are not being ignored will be 
	caught by function f
		i	-> signal number
		f	-> signal routine
	return
		rc	-> former signal
 */
void (*setsig(i, f))()
int      i;
void      (*f)();
{
	register void (*rc)();

	if ((rc = signal(i, SIG_IGN)) != (void (*)()) SIG_IGN)
		signal(i, f);
	return(rc);
}
