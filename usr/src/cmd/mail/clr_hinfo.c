/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:mail/clr_hinfo.c	1.5"
#ident "@(#)clr_hinfo.c	2.7 'attmail mail(1) command'"
/*
    NAME
	clr_hinfo, clrhdr - clean out mail header information

    SYNOPSIS
	void clr_hinfo()
	void clrhdr(int hdrtype)

    DESCRIPTION
	Clr_hinfo() cleans out hdrlines[] and other associated data
	in preparation for the next message.

	Clrhdr() does a single hdrlines[].
*/

#include "mail.h"

void
clr_hinfo()
{
	register	int	i;
	static		int	firsttime = 1;
	static char		pn[] = "clr_hinfo";

	Dout(pn, 0, "\n");
	if (firsttime) {
		firsttime = 0;
		return;
	}
	fnuhdrtype = 0;
	orig_aff = orig_rcv = 0;
	for (i = 0; i < H_CONT; i++) {
		clrhdr(i);
	}
	return;
}

void clrhdr(hdrtype)
int	hdrtype;
{
	while (hdrlines[hdrtype].head != (struct hdrs *)NULL) {
		poplist (hdrtype, HEAD);
	}
}
