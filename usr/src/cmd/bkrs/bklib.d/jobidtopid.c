/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:bklib.d/jobidtopid.c	1.3"

#include	<sys/types.h>

extern int strncmp();
extern int atoi();

/* given a jobid, return the corresponding pid */
pid_t
jobidtopid( jobid )
char *jobid;
{
	register pid;
	if( strncmp( jobid, "back-", 5 ) ) return( 0 );
	return( ((pid = (pid_t) atol( jobid + 5 )) < 0)? 0: pid );
}
