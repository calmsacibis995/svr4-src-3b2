/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:sendlist.c	1.3"
#ident "@(#)sendlist.c	1.4 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	sendlist - send copy to specified users

    SYNOPSIS
	int sendlist(reciplist *list, int letnum)

    DESCRIPTION
	sendlist() will traverse the current recipient list and
	send a copy of the given letter to each user specified,
	invoking send() to do the sending. If flgw is set, the
	processing will be done within the background. It returns
	1 if the sending fails, 0 otherwise.
 */

static int dosendlist();

int sendlist(list, letnum)
reciplist	*list;
int		letnum;
{
	static char	pn[] = "sendlist";
	pid_t pid;
	int ret = 0;

	Dout(pn, 0, "entered\n");
	if (flgw) {
		Dout(pn, 3, "-w\n");
		if ((pid = fork()) == 0) {
			Dout(pn, 3, "fork succeeded\n");
			setpgrp();
			(void) dosendlist(list, letnum);
			_exit(0);
		} else if (pid == CERROR) {
			Dout(pn, 3, "fork failed\n");
			ret = dosendlist(list, letnum);
		}
	} else {
		Dout(pn, 3, "!-w\n");
		ret = dosendlist(list, letnum);
	}
	return ret;
}

static int
dosendlist(plist, letnum)
reciplist	*plist;
int		letnum;
{
	static char	pn[] = "dosendlist";
	int ret = 0;
	recip		*r = &plist->recip_list;

	Dout(pn, 0, "entered\n");
	Dout(pn, 5, "recip_list->next = %#lx, dflag=%d\n",
		(long)r->next, dflag);
	while ((r->next !=(struct recip *)NULL) && dflag != 2) {
		surg_rc = 0;
		r = r->next;
		Dout(pn, 5, "recip_list->name = '%s'\n",
		    r->name ? r->name : "");
		if (!send(plist, letnum, r->name, 0))
			ret++;
		Dout(pn, 5, "r->next = %#lx, dflag=%d\n",
			(long)r->next, dflag);
	}
	return ret;
}
