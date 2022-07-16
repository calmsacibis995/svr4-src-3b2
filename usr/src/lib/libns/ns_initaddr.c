/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libns:ns_initaddr.c	1.4.2.1"
#include <stdio.h>
#include <tiuser.h>
#include <nsaddr.h>
#include <nserve.h>
#include "stdns.h"
#include "nsdb.h"
#include "nslog.h"

ns_initaddr(name)
char *name;
{
	struct nssend send;
	struct nssend *rtn;
	struct nssend *ns_getblock();

	/*
	 *	Initialize the information structure to send to the
	 *	name server.
	 */

	LOG2(L_TRACE, "(%5d) enter: ns_initaddr\n", Logstamp);
	send.ns_code = NS_INIT;
	send.ns_name = name;
	send.ns_type = 0;
	send.ns_flag = 0;
	send.ns_desc = NULL;
	send.ns_path = NULL;
	send.ns_addr = NULL;
	send.ns_mach = NULL;

	while ((rtn = ns_getblock(&send)) == (struct nssend *)NULL &&
		ns_errno == R_INREC)
			sleep(1);

	if (rtn == (struct nssend *) NULL) {
		LOG2(L_TRACE, "(%5d) leave: ns_initaddr\n", Logstamp);
		return(FAILURE);
	}

	LOG2(L_TRACE, "(%5d) leave: ns_initaddr\n", Logstamp);
	return(SUCCESS);
}
