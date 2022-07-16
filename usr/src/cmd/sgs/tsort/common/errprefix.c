/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)tsort:errprefix.c	1.1"

/*	Set prefix string.
*/

#include	"errmsg.h"


void
errprefix( str )
char	*str;
{
	Err.prefix = str;
}
