/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:mail/cksurg_rc.c	1.5"
#ident "@(#)cksurg_rc.c	2.5 'attmail mail(1) command'"
/*
    NAME
	cksurg_rc - check the surrogate return code

    SYNOPSIS
	int cksurg_rc(int surr_num, int rc)

    DESCRIPTION
	Cksurg_rc() looks up the return code in the list
	of return codes for the given surrogate entry.
*/

#include "mail.h"
cksurg_rc (surr_num, rc)
int	surr_num, rc;
{
    return rc >= 0 ? surrfile[surr_num].statlist[rc] : FAILURE;
}
