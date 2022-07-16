/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/msgs/mopen.c	1.5"
/* LINTLIBRARY */

# include	<errno.h>

# include	"lp.h"
# include	"msgs.h"


MESG	*lp_Md = 0;

/*
** mopen() - OPEN A MESSAGE PATH
*/

int
mopen ()
{
    if (lp_Md != NULL)
    {
	errno = EEXIST;
	return (-1);
    }

    if ((lp_Md = mconnect(Lp_FIFO, 0, 0)) == NULL)
	return(-1);

    return(0);
}
