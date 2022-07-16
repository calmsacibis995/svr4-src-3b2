/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:cmd/lpsched/lpsched/putjob.c	1.1"

# include	<unistd.h>
# include	<stdlib.h>
# include	<limits.h>
# include	<string.h>

# include	"lpsched.h"

static char	*rfile;
static char	*src;
static char	*dst;
static char	*rsrc;

#if	defined(__STDC__)
void putjobfiles ( RSTATUS * prs )
#else
void putjobfiles ( prs )
RSTATUS *prs;
#endif
{
    char	**flist = NULL;
    char	**listp;
    char	*tmp1;
    char	*tmp2;
    char	*srcp;
    int		count = 0;
    int		filtered = 0;
    int		len3;
    int		len4;
    int		len5;
    char	*p;
    RSTATUS	rs;
    REQUEST	rtmp;
    SECURE	stmp;
    
    rs = *(prs);
    rtmp = *(prs->request);
    rs.request = &rtmp;
    stmp = *(prs->secure);
    rs.secure = &stmp;
    
    tmp1 = getreqno(rs.secure->req_id);

    if (rs.request->outcome & RS_FILTERED)
    {
	filtered++;
	if (!(tmp2 = makestr("F", tmp1, "-", MOST_FILES_S, (char *)0)))
	    mallocfail();
	if (!(src = makepath(Lp_Tmp, rs.secure->system, tmp2, (char *)0)))
	    mallocfail();
	len3 = strlen(src) - STRSIZE(MOST_FILES_S);
	free (tmp2);
    }
    
    if (!(tmp2 = makestr(tmp1, "-", MOST_FILES_S, (char *)0)))
	mallocfail();
    if (!(dst = makepath(Lp_NetTmp, "tmp", rs.secure->system, tmp2, (char *)0)))
	mallocfail();
    if (!(rsrc = makepath(Lp_Tmp, rs.secure->system, tmp2, (char *)0)))
	mallocfail();

    len4 = strlen(dst) - STRSIZE(MOST_FILES_S);
    len5 = strlen(rsrc) - STRSIZE(MOST_FILES_S);

    for (listp = rs.request->file_list; *listp; listp++)
    {
	(void) sprintf (dst + len4, "%d", count + 1);
	(void) sprintf (rsrc + len5, "%d", count + 1);
	if (filtered)
	{
	    (void) sprintf (src + len3, "%d", count + 1);
	    srcp = src;
	    count++;
	}
	else
	    srcp = *listp;
	
		    
	Unlink (dst);
	if (Link(srcp, dst))
	    if (Symlink(srcp, dst))
		fail ("symlink(%s,%s) failed (%s)\n", srcp, dst, PERROR);


	if (appendlist(&flist, rsrc))
	    mallocfail();
	
	count++;
    }

    /*	Don't free file_list, the pointer is shared with "request" */
    rs.request->file_list = flist;

    if (rs.request->alert)
	rs.request->alert = NULL;
    rs.request->actions &= ~(ACT_WRITE|ACT_MAIL);
    rs.request->actions |= ACT_NOTIFY;
    
    if (!(tmp2 = makestr(tmp1, "-0", (char *)0)))
	mallocfail();
    if (!(rfile = makepath(Lp_NetTmp, "tmp", rs.secure->system, tmp2, (char *)0)))
	mallocfail();

    if (!(p = makestr(Local_System, BANG_S, rs.request->user, (char *)0)))
	mallocfail();

    rs.request->user = p;

    rs.request->destination = strdup(rs.printer->remote_name);
    
    if (putrequest(rfile, rs.request) || putsecure(rs.req_file, rs.secure))
	fail ("putrequest(%s,...) or putsecure(%s,...) failed (%s)\n", rfile, rs.req_file, PERROR);

    if (!(src = makepath(Lp_Requests, rs.req_file, (char *)0)))
	mallocfail();
    if (!(tmp2 = makestr(tmp1, "-0", (char *)0)))
	mallocfail();
    if (!(rfile = makepath(Lp_NetTmp, "requests", rs.secure->system, tmp2, (char *)0)))
	mallocfail();

    Unlink (rfile);
    if (Link(src, rfile))
	if (Symlink(src, rfile))
	    fail ("symlink(%s,%s) failed (%s)\n", src, rfile, PERROR);

    free(p);
    freelist(flist);
}
