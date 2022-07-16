/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:cmd/lpsched/lpsched/getstatus.c	1.6"

# include	<stdlib.h>
# include	<unistd.h>

# include	"lpsched.h"

static SUSPENDED	*Suspend_List = NULL;

#if	defined(__STDC__)
static char * mesgdup ( char * m )
#else
static char * mesgdup ( m )
char	*m;
#endif
{
    char		*p;
    unsigned long	size = msize(m);

    if ((p = (char *)malloc(size)) == NULL)
	mallocfail();

    (void) memcpy(p, m, size);
    return(p);
}



#if	defined(__STDC__)
void mesgadd ( SSTATUS * ssp, char * mesg )
#else
void mesgadd ( ssp, mesg )
SSTATUS	*ssp;
char	*mesg;
#endif
{
    size_t	len;
    char	**p;
    
    if (ssp->tmp_pmlist)
    {
	len = lenlist(ssp->tmp_pmlist);
	
	p = (char **) realloc(&(ssp->tmp_pmlist), (len + 2) * sizeof(char *));
	if (p)
	{
	    p[len] = mesgdup(mesg);
	    p[len + 1] = NULL;
	    ssp->tmp_pmlist = p;
	}
    }
    else
    {
	if (ssp->tmp_pmlist = (char **) calloc(2, sizeof(char *)))
	{
	    ssp->tmp_pmlist[0] = mesgdup(mesg);
	    ssp->tmp_pmlist[1] = NULL;
	}
	else
	    mallocfail();
    }
}

	      
#if	defined(__STDC__)
void askforstatus ( SSTATUS * ssp, MESG * md )	/* funcdecl */
#else
void askforstatus ( ssp, md )
SSTATUS	*ssp;
MESG	*md;
#endif
{
    WAITING	*w;
    time_t	now;
    
    /*
    **	If wait is -1, the user has been through all of this once already
    **	and should not be kept waiting.  This remedies the situation where
    **	the <md> is waiting for 2 or more systems and the response from
    **	one system comes more than USER_STATUS_EXPIRED seconds after another
    **	has reported back.  (i.e., while waiting for one system, the other
    **	expired again.)  Without this check, the <md> could deadlock always
    **	waiting for the status from one more system.
    */
    if (md->wait == -1)
    {
	schedlog("Already waited for status once, don't wait again.\n");
	return;
    }
    
    now = time(NULL);

    if ((now - ssp->laststat) > USER_STATUS_EXPIRED)
    {
	if ((w = (WAITING *) malloc(sizeof(WAITING))) == NULL)
	    mallocfail();
	w->md = md;
	w->next = ssp->waiting;
	ssp->waiting = w;
	md->wait++;
	schedlog("Sending GETSTATUS(%s) to lpexec\n", ssp->system->name);
	mkreq(ssp->system->name, S_GETSTATUS);
	schedule(EV_SYSTEM, ssp);
    }
    else
	schedlog("Timer has not expired yet.\n");
}

#if	defined(__STDC__)
int waitforstatus ( char * m, MESG * md )	/* funcdecl */
#else
int waitforstatus ( m, md )
char	*m;
MESG	*md;
#endif
{
    SUSPENDED	*s;
    
    if (md->wait <= 0)
    {
	md->wait = 0;
	schedlog("No requests to wait for.\n");
	return(0);
    }

    if ((s = (SUSPENDED *) malloc(sizeof(SUSPENDED))) == NULL)
	mallocfail();

    if ((s->message = mesgdup(m)) == NULL)
	mallocfail();

    s->md = md;

    s->next = Suspend_List;
    Suspend_List = s;

    schedlog("Suspend %lu for status\n", md);
    return(1);
}


#if	defined(__STDC__)
void load_bsd_stat ( SSTATUS * ssp, PSTATUS * psp )
#else
void load_bsd_stat ( ssp, psp )
SSTATUS	*ssp;
PSTATUS	*psp;
#endif
{
    FILE		*fpi;
    char		buf[BUFSIZ];
    char		*file;
    char		*rmesg = NULL;
    char		*dmesg = NULL;
    char		*req = "";
    RSTATUS		*rp;
    time_t		now;
    short		status = 0;
    char		mbuf[MSGMAX];
    unsigned long	size;
    char		*cp;
    char		*cp2;
    short		rank;

    file = psp->alert->msgfile;
    
    if ((fpi = open_lpfile(file, "r", MODE_NOREAD)) == NULL)
	return;

    Unlink(file);
    
    while (fgets(buf, BUFSIZ, fpi))
    {
	buf[strlen(buf) - 1] = '\0';
	
	schedlog(">>>%s\n", buf);

	switch(*buf)
	{
	  case '%':
	    /* *!* add code to fetch old status and restore it */
	    break;
	    
	  case '-':
	    if (strstr(buf + 2, "queue") != NULL)
	    {
		schedlog("Added to reject reason\n");
		status |= PS_REJECTED;
		addstring(&rmesg, buf + 2);
		addstring(&rmesg, "\n");
	    }
	    else
	    {
		schedlog("Added to disable reason\n");
		status |= PS_DISABLED;
		addstring(&dmesg, buf + 2);
		addstring(&dmesg, "\n");
	    }
	    break;
	    
	  default:
	    if ((cp = strchr(buf, ':')) == NULL)
		break;
	    cp++;
	    if ((cp2 = strchr(cp, ':')) == NULL)
		break;
	    *cp2++ = '\0';
	    /*
	    **	<cp> now points at the null terminated rank
	    */
	    rank = atoi(cp);
	    if ((cp = strchr(cp2, ':')) == NULL)
		break;
	    *cp = '\0';
	    while (*cp2 == '0')
		cp2++;
	    if (*cp2 == '\0')
		cp2--;
	    
	    /*
	    **	<cp2> now points at the null terminated jobid
	    */
	    if ((rp = request_by_jobid(psp->printer->name, cp2)) == NULL)
	    {
		schedlog("Could not find request for jobid (%s)\n", cp2);
		break;
	    }
	    schedlog("Saving a rank of %d\n", rank);
	    rp->status |= (RSS_KLUDGE|RSS_RANK);
	    if ((rp->rank = rank) == 0)
	    {
		status |= PS_BUSY;
		req = rp->secure->req_id;
	    }
	}
    }

    schedlog("Cleaning up old requests\n");
    for (rp = Request_List; rp; rp = rp->next)
	if (rp->printer == psp)
	{
	    if ((rp->request->outcome & RS_SENT) == 0)
		continue;

	    if (rp->status & RSS_KLUDGE)
	    {
		rp->status &= ~RSS_KLUDGE;
		continue;
	    }
	    schedlog("Completed \"%s\"\n", rp->secure->req_id);
	    rp->request->outcome &= ~RS_ACTIVE;
	    rp->request->outcome |= (RS_PRINTED|RS_NOTIFY);
	    check_request(rp);
	}

    now = time(NULL);
    schedlog("Saving printer status\n");
    size = putmessage(mbuf, R_INQUIRE_REMOTE_PRINTER, MOKMORE,
		      psp->printer->name, "", "", dmesg, rmesg,
		      status, req, (long) now, (long) now);

    mesgadd(ssp, mbuf);
    
    if (dmesg)
	free(dmesg);

    if (rmesg)
	free(rmesg);

    close_lpfile(fpi);
}


#if	defined(__STDC__)
void update_req ( char * req_id, long rank )
#else
void update_req ( req_id, rank )
char	*req_id;
long	rank;
#endif
{
    RSTATUS		*rp;

    if ((rp = request_by_id(req_id)) == NULL)
	return;
    
    rp->status |= RSS_RANK;
    rp->rank = rank;
}

#if	defined(__STDC__)
int md_wakeup ( SSTATUS * ssp )
#else
int md_wakeup (ssp)
    SSTATUS		*ssp;
#endif
{
    WAITING		*w;
    int			wakeup = 0;
    SUSPENDED		*susp;
    SUSPENDED		*newlist = NULL;

    while(ssp->waiting)
    {
	w = ssp->waiting;
	ssp->waiting = w->next;
	if (--(w->md->wait) <= 0)
	    wakeup = 1;
	free(w);
    }

    if (wakeup)
    {
	while (Suspend_List)
	{
	    susp = Suspend_List;
	    Suspend_List = susp->next;
	    if (susp->md->wait <= 0)
	    {
		susp->md->wait = -1;
		(void) dispatch(mtype(susp->message), susp->message, susp->md);
		free(susp->message);
		free(susp);
	    }
	    else
	    {
		susp->next = newlist;
		newlist = susp;
	    }
	}
	Suspend_List = newlist;
    }
}
