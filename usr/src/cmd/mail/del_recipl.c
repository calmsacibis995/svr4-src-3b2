/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:del_recipl.c	1.3"
#ident "@(#)del_recipl.c	1.4 'attmail mail(1) command'"
#include "mail.h"

/*
    NAME
	del_reciplist - delete a recipient list

    SYNOPSIS
	del_reciplist (reciplist *list)

    DESCRIPTION
	Free the space used by a recipient list.
*/

void del_reciplist (plist)
reciplist	*plist;
{
	static char	pn[] = "del_reciplist";
	recip		*r = &plist->recip_list;
	Dout(pn, 0, "entered\n");
	if (r->next != (struct recip *)NULL) {
		for (r = r->next; r != (struct recip *)NULL; ) {
			recip *old = r;
			r = old->next;
			free(old->name);
			free((char*)old);
		}
	}
}
