/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:lib/msgs/mrecv.c	1.3"
/* LINTLIBRARY */
# include	<errno.h>

# include	"lp.h"
# include	"msgs.h"

extern MESG	*lp_Md;

/*
** mrecv() - RECEIVE A MESSAGE
*/

int
mrecv (msgbuf, size)
char	*msgbuf;
int	size;
{
    int		n;

    /*
    **	Restart interrupted reads for binary compatibility.
    */
    do
	n = mread(lp_Md, msgbuf, size);
    while (n < 0 && errno == EINTR);
    
    /*
    **	Return EIDRM on disconnect for binary compatibility.
    */
    if (errno == EPIPE)
	errno = EIDRM;

    if (n <= 0)
	return(-1);

    return(getmessage(msgbuf, I_GET_TYPE));
}
