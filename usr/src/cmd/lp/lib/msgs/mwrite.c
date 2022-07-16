/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:lib/msgs/mwrite.c	1.5"
/* LINTLIBRARY */

# include	<errno.h>
# include	<signal.h>
# include	<string.h>
# include	<stropts.h>

# include	"lp.h"
# include	"msgs.h"

#if	defined(__STDC__)
static int	write3_2 ( MESG *, char *, int );
static int	_mwrite ( MESG * md , char * msgbuf , int );
#else
static int	write3_2();
static int	_mwrite();
#endif

int	Lp_prio_msg = 0;

#if	defined(__STDC__)
int mwrite ( MESG * md, char * msgbuf )
#else
int mwrite (md, msgbuf)
    MESG	*md;
    char	*msgbuf;
#endif
{
    short		size;
    MQUE *		p;
    MQUE *		q;

    if (md == NULL)
    {
	errno = ENXIO;
	return(-1);
    }
    if (msgbuf == NULL)
    {
	errno = EINVAL;
	return(-1);
    }

    size = stoh(msgbuf);

    if (LAST_MESSAGE < stoh(msgbuf + MESG_TYPE))
    {
	errno = EINVAL;
	return (-1);
    }
    if (md->mque)
	goto queue;

    if (_mwrite(md, msgbuf, size) == 0)
	return(0);
    if (errno != EAGAIN)
	return(-1);

queue:
    if ((p = (MQUE *)malloc(sizeof(MQUE))) == NULL
        || (p->dat = (struct strbuf *)malloc(sizeof(struct strbuf))) == NULL
    	|| (p->dat->buf = (char *)malloc(size)) == NULL)
    {
	errno = ENOMEM;
	return(-1);
    }
    (void) memcpy(p->dat->buf, msgbuf, size);
    p->dat->len = size;
    p->next = 0;

    if ((q = md->mque))
    {
	while (q->next)
	    q = q->next;
	q->next = p;
    	while((p = md->mque))
    	{
		if(_mwrite(md, p->dat->buf, p->dat->len) != 0)
	    	return(errno == EAGAIN ? 0 : -1);
		md->mque = p->next;
		free(p->dat->buf);
		free(p->dat);
		free(p);
    	}
    }
    else
    	md->mque = p;

    return(0);
}

#if	defined(__STDC__)
int _mwrite ( MESG * md, char * msgbuf , int size )
#else
int _mwrite (md, msgbuf, size)
    MESG	*md;
    char	*msgbuf;
    int		size;
#endif
{
    int			flag = 0;
    struct strbuf	dat;
    struct strbuf	ctl;

    switch (md->type)
    {
        case MD_CHILD:
	case MD_STREAM:
	case MD_BOUND:
	    if (size <= 0 || size > MSGMAX)
	    {
		errno = EINVAL;
		return(-1);
	    }

	    dat.buf = msgbuf;
	    dat.len = size;
	    ctl.buf = "";
	    ctl.len = 0;
	    flag = Lp_prio_msg;
	    Lp_prio_msg = 0;	/* clean this up so there are no surprises */

	    if (putmsg(md->writefd, &ctl, &dat, flag) == 0)
		return(0);
	    if (errno == EAGAIN)
		break;
	    return(-1);

	case MD_SYS_FIFO:
	case MD_USR_FIFO:
	    switch (write3_2(md, msgbuf, size))
	    {
		case -1:
		    return(-1);
		case 0:
		    break;
		default:
		    return(0);
	    }
	    break;

	default:
	    errno = EINVAL;
	    return(-1);
    }

}

char		AuthCode[HEAD_AUTHCODE_LEN];
static void	(*callers_sigpipe_trap)() = SIG_DFL;

#if	defined(__STDC__)
static int write3_2 ( MESG * md, char * msgbuf, int size )
#else
static int write3_2 (md, msgbuf, size)
    MESG	*md;
    char	*msgbuf;
    int		size;
#endif
{
    char	*tmpbuf;
    int		rval;

    if ((tmpbuf = (char *) malloc(EXCESS_3_2_LEN + size)) == NULL)
    {
	errno = ENOMEM;
	return(-1);
    }

    (void) memmove(tmpbuf + HEAD_SIZE, msgbuf, size);
    (void) htos(tmpbuf + HEAD_SIZE, size + EXCESS_3_2_LEN);
    (void) memcpy (tmpbuf + HEAD_AUTHCODE, AuthCode, HEAD_AUTHCODE_LEN);

    callers_sigpipe_trap = signal(SIGPIPE, SIG_IGN);

    rval = write_fifo(md->writefd, tmpbuf, size + EXCESS_3_2_LEN);

    (void) signal(SIGPIPE, callers_sigpipe_trap);

    return (rval);
}
