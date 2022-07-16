/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:lib/msgs/mdisconnect.c	1.4"
/* LINTLIBRARY */

# include	<stropts.h>
# include	<errno.h>
# include	<stdlib.h>

#include "lp.h"
#include "msgs.h"

#if	defined(__STDC__)
static void	disconnect3_2 ( MESG * );
#else
static void	disconnect3_2();
#endif

#if	defined(__STDC__)
int mdisconnect ( MESG * md )
#else
int mdisconnect (md)
    MESG	*md;
#endif
{
    int		retvalue = 0;
    void	(**fnp)();
    MQUE	*p;
    
    if (!md)
    {
	errno = ENXIO;
	return(-1);
    }

    switch(md->type)
    {
	case MD_CHILD:
	case MD_STREAM:
	case MD_BOUND:
	    if (md->writefd >= 0)
		(void) Close(md->writefd);
	    if (md->readfd >= 0)
		(void) Close(md->readfd);
	    break;

	case MD_USR_FIFO:
	case MD_SYS_FIFO:
	   disconnect3_2(md);
	   break;
    }

    if (md->on_discon)
    {
	for (fnp = md->on_discon; *fnp; fnp++)
	{
	    (*fnp)(md);
	    retvalue++;
	}
	free(md->on_discon);
    }

    if (md->file)
	free(md->file);
    
    if (md->mque)
    {
	while ((p = md->mque) != NULL)
	{
	    md->mque = p->next;
	    free(p->dat->buf);
	    free(p->dat);
	    free(p);
	}
    }
    free(md);

    return(retvalue);
}

int	discon3_2_is_running = 0;

#if	defined(__STDC__)
static void disconnect3_2 ( MESG * md )
#else
static void disconnect3_2 (md)
    MESG	*md;
#endif
{
    char	*msgbuf = 0;
    int		size;

    discon3_2_is_running = 1;

    if (md->writefd != -1)
    {
	size = putmessage((char *)0, S_GOODBYE);
	if ((msgbuf = (char *)malloc((unsigned)size)))
	{
	    (void)putmessage (msgbuf, S_GOODBYE);
	    (void)msend (msgbuf);
	    free (msgbuf);
	}

	(void) Close (md->writefd);
    }

    if (md->readfd != -1)
	(void) Close (md->readfd);

    if (md->file)
    {
	(void) Unlink (md->file);
	free (md->file);
    }	

    discon3_2_is_running = 0;
}
