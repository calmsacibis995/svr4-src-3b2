/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:lib/bsd/rsendjob.c	1.2"

#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "msgs.h"
#include "lpd.h"

extern MESG	*lp_Md;

/*
 * Package and send R_SEND_JOB to lpexec
 */
void
#if defined (__STDC__)
r_send_job(int status, char *msg)
#else
r_send_job(status, msg)
int	 status;
char	*msg;
#endif
{
	if (!msg)			/* overly cautious! */
		status = MTRANSMITERR;
	(void)mputm(lp_Md, R_SEND_JOB, Rhost, status, msg ? msize(msg) : 0, msg);
}
