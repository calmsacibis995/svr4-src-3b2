/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fumount:sndmes.c	1.8.4.1"

#include <stdio.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/rf_messg.h>
#include <sys/nserve.h>
#include <sys/rf_sys.h>

sndmes(sysid, time, resrc)
sysid_t sysid;
char *time;
char *resrc;
{
	static char msg[200];

		/* 
		 * send a warning message to a remote system.
		 * syntax of message:	fuwarn domain.resource [time] 
		 */
	strcpy(msg, "fuwarn ");
	strncat(msg, resrc, sizeof(msg) - strlen(msg));
	strncat(msg, " ", sizeof(msg) - strlen(msg));
	strncat(msg, time, sizeof(msg) - strlen(msg));
	return(rfsys(RF_SENDUMSG, sysid, msg, strlen(msg)));
}
