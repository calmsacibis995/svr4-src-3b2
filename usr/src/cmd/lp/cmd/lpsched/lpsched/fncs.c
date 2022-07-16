/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlp:cmd/lpsched/fncs.c	1.9.1.1"
/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"

#include "lpsched.h"

extern void		free();

/**
 ** walk_ptable() - WALK PRINTER TABLE, RETURNING ACTIVE ENTRIES
 ** walk_ftable() - WALK FORMS TABLE, RETURNING ACTIVE ENTRIES
 ** walk_ctable() - WALK CLASS TABLE, RETURNING ACTIVE ENTRIES
 ** walk_pwtable() - WALK PRINT WHEEL TABLE, RETURNING ACTIVE ENTRIES
 **/


PSTATUS *
#if	defined(__STDC__)
walk_ptable (
	int			start
)
#else
walk_ptable (start)
	int			start;
#endif
{
	static PSTATUS		*psend,
				*ps = 0;

	if (start || !ps) {
		ps = PStatus;
		psend = PStatus + PT_Size;
	}

	while (ps < psend && !ps->printer->name)
		ps++;

	if (ps >= psend)
		return (ps = 0);
	else
		return (ps++);
}

FSTATUS *
#if	defined(__STDC__)
walk_ftable (
	int			start
)
#else
walk_ftable (start)
	int			start;
#endif
{
	static FSTATUS		*psend,
				*ps = 0;

	if (start || !ps) {
		ps = FStatus;
		psend = FStatus + FT_Size;
	}

	while (ps < psend && !ps->form->name)
		ps++;

	if (ps >= psend)
		return (ps = 0);
	else
		return (ps++);
}

CSTATUS *
#if	defined(__STDC__)
walk_ctable (
	int			start
)
#else
walk_ctable (start)
	int			start;
#endif
{
	static CSTATUS		*psend,
				*ps = 0;

	if (start || !ps) {
		ps = CStatus;
		psend = CStatus + CT_Size;
	}

	while (ps < psend && !ps->class->name)
		ps++;

	if (ps >= psend)
		return (ps = 0);
	else
		return (ps++);
}

PWSTATUS *
#if	defined(__STDC__)
walk_pwtable (
	int			start
)
#else
walk_pwtable (start)
	int			start;
#endif
{
	static PWSTATUS		*psend,
				*ps = 0;

	if (start || !ps) {
		ps = PWStatus;
		psend = PWStatus + PWT_Size;
	}

	while (ps < psend && !ps->pwheel->name)
		ps++;

	if (ps >= psend)
		return (ps = 0);
	else
		return (ps++);
}

/**
 ** walk_req_by_printer() - WALK REQUEST LIST, GET ALL FOR ONE PRINTER
 ** walk_req_by_form() - WALK REQUEST LIST, GET ALL FOR ONE FORM
 ** walk_req_by_pwheel() - WALK REQUEST LIST, GET ALL FOR ONE PRINT WHEEL
 ** walk_req_by_dest() - WALK REQUEST LIST, GET ALL FOR ONE DESTINATION
 **/

RSTATUS *
#if	defined(__STDC__)
walk_req_by_printer (
	register RSTATUS **	pnext,
	register PSTATUS *	pps
)
#else
walk_req_by_printer (pnext, pps) 
	register RSTATUS	**pnext;
	register PSTATUS	*pps;
#endif
{ 
	register RSTATUS	*prs;

	if (!(prs = *pnext))
		prs = *pnext = Request_List; 

	while (prs && prs->printer != pps)
		prs = prs->next;

	if (prs)
		*pnext = prs->next;

	return (prs); 
}

RSTATUS *
#if	defined(__STDC__)
walk_req_by_form (
	register RSTATUS **	pnext,
	register FSTATUS *	pfs
)
#else
walk_req_by_form (pnext, pfs) 
	register RSTATUS	**pnext;
	register FSTATUS	*pfs;
#endif
{ 
	register RSTATUS	*prs;

	if (!(prs = *pnext))
		prs = *pnext = Request_List; 

	while (prs && prs->form != pfs)
		prs = prs->next;

	if (prs)
		*pnext = prs->next;

	return (prs); 
}

RSTATUS *
#if	defined(__STDC__)
walk_req_by_pwheel (
	register RSTATUS **	pnext,
	register char *		pwheel_name
)
#else
walk_req_by_pwheel (pnext, pwheel_name) 
	register RSTATUS	**pnext;
	register char		*pwheel_name;
#endif
{ 
	register RSTATUS	*prs;

	if (!(prs = *pnext))
		prs = *pnext = Request_List; 

	while (
		prs
	     && (
			!prs->pwheel_name
		     || !STREQU(prs->pwheel_name, pwheel_name)
		)
	)
		prs = prs->next;

	if (prs)
		*pnext = prs->next;

	return (prs); 
}

RSTATUS *
#if	defined(__STDC__)
walk_req_by_dest (
	register RSTATUS **	pnext,
	register char *		destination
)
#else
walk_req_by_dest (pnext, destination) 
	register RSTATUS	**pnext;
	register char		*destination;
#endif
{ 
	register RSTATUS	*prs;

	if (!(prs = *pnext))
		prs = *pnext = Request_List; 

	while (prs && !STREQU(prs->request->destination, destination))
		prs = prs->next;

	if (prs)
		*pnext = prs->next;

	return (prs); 
}

/**
 ** search_ptable() - SEARCH PRINTER TABLE
 ** search_ftable() - SEARCH FORMS TABLE
 ** search_ctable() - SEARCH CLASS TABLE
 ** search_pwtable() - SEARCH PRINT WHEEL TABLE
 **/

PSTATUS *
#if	defined(__STDC__)
search_ptable (
	register char *		name
)
#else
search_ptable (name) 
	register char		*name; 
#endif
{ 
	register PSTATUS	*ps,
				*psend; 

	for ( 
		ps = & PStatus[0], psend = & PStatus[PT_Size]; 
		ps < psend && !SAME(ps->printer->name, name); 
		ps++ 
	)
		; 

	if (ps >= psend) 
		ps = 0; 

	return (ps); 
}

FSTATUS *
#if	defined(__STDC__)
search_ftable (
	register char *		name
)
#else
search_ftable (name) 
	register char		*name; 
#endif
{ 
	register FSTATUS	*ps,
				*psend; 

	for ( 
		ps = & FStatus[0], psend = & FStatus[FT_Size]; 
		ps < psend && !SAME(ps->form->name, name); 
		ps++ 
	)
		; 

	if (ps >= psend) 
		ps = 0; 

	return (ps); 
}

CSTATUS *
#if	defined(__STDC__)
search_ctable (
	register char *		name
)
#else
search_ctable (name) 
	register char		*name; 
#endif
{ 
	register CSTATUS	*ps,
				*psend; 

	for ( 
		ps = & CStatus[0], psend = & CStatus[CT_Size]; 
		ps < psend && !SAME(ps->class->name, name); 
		ps++ 
	)
		; 

	if (ps >= psend) 
		ps = 0; 

	return (ps); 
}

PWSTATUS *
#if	defined(__STDC__)
search_pwtable (
	register char *		name
)
#else
search_pwtable (name) 
	register char		*name; 
#endif
{ 
	register PWSTATUS	*ps,
				*psend; 

	for ( 
		ps = & PWStatus[0], psend = & PWStatus[PWT_Size]; 
		ps < psend && !SAME(ps->pwheel->name, name); 
		ps++ 
	)
		; 

	if (ps >= psend) 
		ps = 0; 

	return (ps); 
}

SSTATUS *
#if	defined(__STDC__)
search_stable (
	char *			name
)
#else
search_stable (name)
	char			*name;
#endif
{
	int			i;

	if (!SStatus)
		return (0);
    
	for (
		i = 0;
		SStatus[i] && !SAME(SStatus[i]->system->name, name);
		i++
	)
		;

	return (SStatus[i]);
}

/**
 ** load_str() - LOAD STRING WHERE ALLOC'D STRING MAY BE
 ** unload_str() - REMOVE POSSIBLE ALLOC'D STRING
 **/

void
#if	defined(__STDC__)
load_str (
	char **			pdst,
	char *			src
)
#else
load_str (pdst, src)
	char			**pdst,
				*src;
#endif
{
	if (*pdst)
		free (*pdst);
	*pdst = strdup(src);
	return;
}

void
#if	defined(__STDC__)
unload_str (
	char **			pdst
)
#else
unload_str (pdst)
	char			**pdst;
#endif
{
	if (*pdst)
		free (*pdst);
	*pdst = 0;
	return;
}

/**
 ** unload_list() - REMOVE POSSIBLE ALLOC'D LIST
 **/

void
#if	defined(__STDC__)
unload_list (
	char ***		plist
)
#else
unload_list (plist)
	char			***plist;
#endif
{
	if (*plist)
		freelist (*plist);
	*plist = 0;
	return;
}

/**
 ** load_sdn() - LOAD STRING WITH ASCII VERSION OF SCALED DECIMAL NUMBER
 **/

void
#if	defined(__STDC__)
load_sdn (
	char **			p,
	SCALED			sdn
)
#else
load_sdn (p, sdn)
	char			**p;
	SCALED			sdn;
#endif
{
	if (!p)
		return;

	if (*p)
		free (*p);
	*p = 0;

	if (sdn.val <= 0 || 999999 < sdn.val)
		return;

	*p = malloc(sizeof("999999.999x"));
	if (*p)
		sprintf (
			*p,
			"%.3f%s",
			sdn.val,
			(sdn.sc == 'c'? "c" : (sdn.sc == 'i'? "i" : ""))
		);

	return;
}

/**
 ** Getform() - EASIER INTERFACE TO "getform()"
 **/

_FORM *
#if	defined(__STDC__)
Getform (
	char *			form
)
#else
Getform (form)
	char			*form;
#endif
{
	static _FORM		_formbuf;

	FORM			formbuf;

	FALERT			alertbuf;

	int				ret;


	/*
	 * "getform()" stores a name only if we get "all". Thus, if we're
	 * getting a single form, we have to copy the name.
	 */
	register int		name_okay	= STREQU(form, NAME_ALL);


	while (
		(ret = getform(form, &formbuf, &alertbuf, (FILE **)0)) == -1
	     && errno == EINTR
	)
		;
	if (ret == -1)
		return (0);

	_formbuf.plen = formbuf.plen;
	_formbuf.pwid = formbuf.pwid;
	_formbuf.lpi = formbuf.lpi;
	_formbuf.cpi = formbuf.cpi;
	_formbuf.np = formbuf.np;
	_formbuf.chset = formbuf.chset;
	_formbuf.mandatory = formbuf.mandatory;
	_formbuf.rcolor = formbuf.rcolor;
	_formbuf.comment = formbuf.comment;
	_formbuf.conttype = formbuf.conttype;

	if (name_okay)
		_formbuf.name = formbuf.name;
	else
		_formbuf.name = strdup(form);

	if ((_formbuf.alert.shcmd = alertbuf.shcmd)) {
		_formbuf.alert.Q = alertbuf.Q;
		_formbuf.alert.W = alertbuf.W;
	} else {
		_formbuf.alert.Q = 0;
		_formbuf.alert.W = 0;
	}

	return (&_formbuf);
}

/**
 ** Getprinter()
 ** Getrequest()
 ** Getuser()
 ** Getclass()
 ** Getpwheel()
 ** Getsecure()
 ** Getsystem()
 ** Loadfilters()
 **/

PRINTER *
#if	defined(__STDC__)
Getprinter (
	char *			name
)
#else
Getprinter (name)
	char			*name;
#endif
{
	register PRINTER	*ret;

	TRACE_ON ("Getprinter");
	while (!(ret = getprinter(name)) && errno == EINTR)
		;
	TRACE_OFF ("Getprinter");
	return (ret);
}

REQUEST *
#if	defined(__STDC__)
Getrequest (
	char *			file
)
#else
Getrequest (file)
	char			*file;
#endif
{
	register REQUEST	*ret;

	while (!(ret = getrequest(file)) && errno == EINTR)
		;
	return (ret);
}

USER *
#if	defined(__STDC__)
Getuser (
	char *			name
)
#else
Getuser (name)
	char			*name;
#endif
{
	register USER		*ret;

	while (!(ret = getuser(name)) && errno == EINTR)
		;
	return (ret);
}

CLASS *
#if	defined(__STDC__)
Getclass (
	char *			name
)
#else
Getclass (name)
	char			*name;
#endif
{
	register CLASS		*ret;

	while (!(ret = getclass(name)) && errno == EINTR)
		;
	return (ret);
}

PWHEEL *
#if	defined(__STDC__)
Getpwheel (
	char *			name
)
#else
Getpwheel (name)
	char			*name;
#endif
{
	register PWHEEL		*ret;

	while (!(ret = getpwheel(name)) && errno == EINTR)
		;
	return (ret);
}

SECURE *
#if	defined(__STDC__)
Getsecure (
	char *			file
)
#else
Getsecure (file)
	char			*file;
#endif
{
	register SECURE		*ret;

	while (!(ret = getsecure(file)) && errno == EINTR)
		;
	return (ret);
}

SYSTEM *
#if	defined(__STDC__)
Getsystem (
	char *			file
)
#else
Getsystem (file)
	char			*file;
#endif
{
	SYSTEM		*ret;

	while (!(ret = getsystem(file)) && errno == EINTR)
		;
	return (ret);
}

int
#if	defined(__STDC__)
Loadfilters (
	char *			file
)
#else
Loadfilters (file)
	char			*file;
#endif
{
	register int		ret;

	while ((ret = loadfilters(file)) == -1 && errno == EINTR)
		;
	return (ret);
}

/**
 ** free_form() - FREE MEMORY ALLOCATED FOR _FORM STRUCTURE
 **/

void
#if	defined(__STDC__)
free_form (
	register _FORM *	pf
)
#else
free_form (pf)
	register _FORM		*pf;
#endif
{
	if (!pf)
		return;
	if (pf->chset)
		free (pf->chset);
	if (pf->rcolor)
		free (pf->rcolor);
	if (pf->comment)
		free (pf->comment);
	if (pf->conttype)
		free (pf->conttype);
	if (pf->name)
		free (pf->name);
	pf->name = 0;
	if (pf->alert.shcmd)
		free (pf->alert.shcmd);
	return;
}

/**
 ** getreqno() - GET NUMBER PART OF REQUEST ID
 **/

char *
#if	defined(__STDC__)
getreqno (
	char *			req_id
)
#else
getreqno (req_id)
	char			*req_id;
#endif
{
	register char		*cp;

	if (!(cp = strchr(req_id, '-')))
		cp = req_id;
	else
		cp++;
	return (cp);
}
