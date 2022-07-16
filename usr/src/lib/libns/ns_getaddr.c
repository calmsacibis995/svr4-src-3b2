/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libns:ns_getaddr.c	1.4.3.1"
#include <stdio.h>
#include <tiuser.h>
#include <nsaddr.h>
#include "nserve.h"
#include "nslog.h"

struct address *
ns_getaddr(resource, ro_flag, dname)
char	 *resource;
int	  ro_flag;
char	 *dname;
{
	struct nssend send;
	struct nssend *rtn;
	struct nssend *ns_getblock();

	LOG2(L_TRACE, "(%5d) enter: ns_getaddr\n", Logstamp);
	/*
	 *	Initialize the information structure to send to the
	 *	name server.
	 */

	send.ns_code = NS_QUERY;
	send.ns_flag = ro_flag;
	send.ns_name = resource;
	send.ns_type = 0;
	send.ns_desc = NULL;
	send.ns_path = NULL;
	send.ns_addr = NULL;
	send.ns_mach = NULL;

	/*
	 *	start up communication with name server.
	 */

	if ((rtn = ns_getblock(&send)) == (struct nssend *)NULL) {
		LOG2(L_TRACE, "(%5d) leave: ns_getaddr\n", Logstamp);
		return((struct address *)NULL);
	}

	if (dname)
		strncpy(dname,rtn->ns_mach[0],MAXDNAME);
	LOG2(L_TRACE, "(%5d) leave: ns_getaddr\n", Logstamp);
	return(rtn->ns_addr);
}

struct address *
ns_getaddrs(resource, ro_flag, dname)
char	 *resource;
int	  ro_flag;
char	 *dname;
{
	struct nssend send;
	struct nssend *sdp;
	struct nssend *rtn;
	struct nssend *ns_getblocks();

	LOG2(L_TRACE, "(%5d) enter: ns_getaddrs\n", Logstamp);
	/*
	 *	Initialize the information structure to send to the
	 *	name server.
	 */

	if (resource)
	{
		send.ns_code = NS_QUERY;
		send.ns_flag = ro_flag;
		send.ns_name = resource;
		send.ns_type = 0;
		send.ns_desc = NULL;
		send.ns_path = NULL;
		send.ns_addr = NULL;
		send.ns_mach = NULL;

		/*
		 *	start up communication with name server.
		 */
		sdp = &send;
	}
	else
		sdp = NULL;

	if ((rtn = ns_getblocks(sdp)) == (struct nssend *)NULL) {
		LOG2(L_TRACE, "(%5d) leave: ns_getaddrs\n", Logstamp);
		return((struct address *)NULL);
	}

	if (dname)
		strncpy(dname,rtn->ns_mach[0],MAXDNAME);
	LOG2(L_TRACE, "(%5d) leave: ns_getaddrs\n", Logstamp);
	return(rtn->ns_addr);
}

