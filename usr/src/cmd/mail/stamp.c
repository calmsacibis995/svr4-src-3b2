/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:mail/stamp.c	1.4"
#ident "@(#)stamp.c	2.4 'attmail mail(1) command'"
#include "mail.h"
/*
	If the mailfile still exists (it may have been deleted), 
	time-stamp it; so that our re-writing of mail back to the 
	mailfile does not make shell think that NEW mail has arrived 
	(by having the file times change).
*/
void stamp()
{
	if (access(mailfile, A_EXIST) == A_OK) 
		if (utime(mailfile, utimep) != A_OK)
			errmsg(E_FILE,"Cannot time-stamp mailfile");
}
