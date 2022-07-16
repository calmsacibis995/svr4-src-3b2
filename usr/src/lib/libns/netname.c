/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libns:netname.c	1.5.2.1"
#include "string.h"
#include "errno.h"
#include "nserve.h"
#include "sys/utsname.h"
#include "sys/types.h"
#include "sys/nserve.h"
#include "sys/rf_sys.h"
#include <stdio.h>
#include "nslog.h"

int
netname(s)
char *s;
{
	struct utsname	name;

/*
 *	flow: 
 *
 *	1) make a call to rfsys to get domain name
 *	2) do a uname to get sysname 
 *	3) concatonate with "."
 *	4) return netnodename
 *
 */

	LOG2(L_TRACE, "(%5d) enter: netname\n", Logstamp);
	if(rfsys(RF_GETDNAME, s, MAXDNAME) < 0) {
		perror("netname");
		LOG2(L_TRACE, "(%5d) leave: netname\n", Logstamp);
		return(-1);
	}
	if(uname(&name) < 0) {
		perror("netname");
		LOG2(L_TRACE, "(%5d) leave: netname\n", Logstamp);
		return(-1);
	}
	(void)sprintf(s + strlen(s),"%c%s",SEPARATOR, name.nodename);
	LOG2(L_TRACE, "(%5d) leave: netname\n", Logstamp);
	return(0);
}
