/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:lib/msgs/mread.c	1.5"
/* LINTLIBRARY */


#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stropts.h>

#include "lp.h"
#include "msgs.h"

extern int	Lp_prio_msg;
extern char	AuthCode[];

#if	defined(__STDC__)
static int	read3_2 ( MESG * md, char *msgbuf, int size );
#else
static int	read3_2();
#endif

/*
**	Function:	int mread( MESG *, char *, int)
**	Args:		message descriptor
**			message buffer (var)
**			buffer size
**	Return:		The size of the message in message buffer.
**			or -1 on error.  Possible errnos are:
**		EINVAL	Bad value for md or msgbuf.
**		E2BIG	Not enough space for message.
**		EPIPE	Far end dropped the connection.
**		ENOMSG	No valid message available on fifo.
**
**	mread examines message descriptor and either calls read3_2
**	to read 3.2 HPI messages or getmsg(2) to read 4.0 HPI messages.
**	If a message is read, it is returned in message buffer.
*/

#if	defined(__STDC__)
int mread ( MESG * md, char * msgbuf, int size )
#else
int mread ( md, msgbuf, size )
MESG	*md;
char	*msgbuf;
int	size;
#endif
{
    struct strbuf	dat;
    struct strbuf	ctl;
    int			flag = 0;

    if (md == NULL || msgbuf == NULL)
    {
	errno = EINVAL;
	return(-1);
    }

    switch(md->type)
    {
      case MD_CHILD:
      case MD_STREAM:
      case MD_BOUND:
	if (size <= 0)
	{
	    errno = E2BIG;
	    return(-1);
	}
	dat.buf = msgbuf;
	dat.maxlen = size;
	dat.len = 0;
	ctl.buf = NULL;
	ctl.maxlen = 0;
	ctl.len = 0;
	flag = Lp_prio_msg;
	Lp_prio_msg = 0;	/* clean this up so there are no surprises */
	
	if (getmsg(md->readfd, &ctl, &dat, &flag) < 0)
	{
	    if (errno == EBADF)
		errno = EPIPE;
	    return(-1);
	}

	if (dat.len == 0)
	{
	    (void) Close(md->readfd);
	    return(0);
	}
	break;

      case MD_USR_FIFO:
      case MD_SYS_FIFO:
	if (size < CONTROL_LEN)
	{
	    errno = E2BIG;
	    return(-1);
	}

	if (read3_2(md, msgbuf, size) < 0)
	    return(-1);
	break;
    }

    return((int)msize(msgbuf));
}

/*
**	Function:	static in read3_2( MESG *, char *, int)
**	Args:		message descriptor
**			message buffer (var)
**			buffer size
**	Return:		0 for sucess, -1 for failure
**
**	This performs a 3.2 HPI style read_fifo on the pipe referanced
**	in the message descriptor.  If a message is found, it is returned
**	in message buffer.
*/
#if	defined(__STDC__)
static int read3_2 ( MESG * md, char *msgbuf, int size )
#else
static int read3_2 ( md, msgbuf, size )
MESG	*md;
char	*msgbuf;
int	size;
#endif
{
    short	type;

    if (md->type == MD_USR_FIFO)
	(void) Close (Open(md->file, O_RDONLY, 0));

    do
    {
	switch (read_fifo(md->readfd, msgbuf, size))
	{
	  case -1:
	    return (-1);

	  case 0:
	    /*
	     ** The fifo was empty and we have O_NDELAY set,
	     ** or the Spooler closed our FIFO.
	     ** We don't set O_NDELAY in the user process,
	     ** so that should never happen. But be warned
	     ** that we can't tell the difference in some versions
	     ** of the UNIX op. sys.!!
	     **
	     */
	    errno = EPIPE;
	    return (-1);
	}

	if ((type = stoh(msgbuf + HEAD_TYPE)) < 0 || LAST_MESSAGE < type)
	{
	    errno = ENOMSG;
	    return (-1);
	}
    }
    while (type == I_QUEUE_CHK);

    (void)memcpy (AuthCode, msgbuf + HEAD_AUTHCODE, HEAD_AUTHCODE_LEN);

    /*
    **	Get the size from the 3.2 HPI message
    **	minus the size of the control data
    **	Copy the actual message
    **	Reset the message size.
    */
    size = stoh(msgbuf + HEAD_SIZE) - EXCESS_3_2_LEN;
    memmove(msgbuf, msgbuf + HEAD_SIZE, size);
    (void) htos(msgbuf + MESG_SIZE, size);
    return(0);
}
