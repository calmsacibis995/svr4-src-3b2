/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libns:rtoken.c	1.3.4.1"
#include <stdio.h>
#include <tiuser.h>
#include <nsaddr.h>
#include "stdns.h"
#include "nsdb.h"
#include "nserve.h"
#include "sys/types.h"
#include "sys/nserve.h"
#include "sys/rf_sys.h"
#include "nslog.h"

static char	sepstr[2] = { SEPARATOR, '\0' };

/*
 * For left to right syntax of domain names,
 * rtoken is identical to strtok.
 * For right to left syntax, it does the equivalent
 * of strtok, but starts at the right side of the string
 * and goes to the left.
 */
#ifdef RIGHT_TO_LEFT
char	*
rtoken(name)
char	*name;
{
	static	char	*pptr;
	static	char	buffer[BUFSIZ];

	LOG2(L_TRACE, "(%5d) enter: rtoken\n", Logstamp);
	if (name) {	/* initialize */
		strncpy(buffer,name,BUFSIZ);
		pptr = &buffer[strlen(buffer)];
	}
	else if (pptr == NULL) {
		fprintf(stderr,"rtoken: no current name\n");
		LOG2(L_TRACE, "(%5d) leave: rtoken\n", Logstamp);
		return(NULL);
	}

	*pptr = '\0';

	if (buffer == pptr) {
		LOG2(L_TRACE, "(%5d) leave: rtoken\n", Logstamp);
		return(NULL);
	}

	while (buffer < pptr && *pptr != SEPARATOR)
		pptr--;

	if (*pptr == SEPARATOR) {
		LOG2(L_TRACE, "(%5d) leave: rtoken\n", Logstamp);
		return(pptr+1);
	}

	LOG2(L_TRACE, "(%5d) leave: rtoken\n", Logstamp);
	return(pptr);
}
#else
char	*
rtoken(name)
char	*name;
{
	char	*strtok();
	return(strtok(name,sepstr));
}
#endif
/**************************************************************
 *
 *	dompart returns the domain part of a domain name.
 *
 *	e.g., if RIGHT_TO_LEFT
 *		dompart("a.b.c.d") would return "b.c.d"
 *		dompart("a") would return "" (ptr to null string)
 *	      if LEFT_TO_RIGHT
 *		dompart("a.b.c.d") would return "a.b.c"
 *		dompart("a") would return "" (ptr to null string)
 *
 *	dompart returns a pointer to a static buffer that contains
 *	the result.  An error will cause dompart to return NULL.
 *
 *************************************************************/
char *
dompart(name)
register char	*name;
{
#ifdef RIGHT_TO_LEFT
	static char	sname[MAXDNAME+1];

	LOG2(L_TRACE, "(%5d) enter: dompart\n", Logstamp);
	if (name == NULL) {
		LOG2(L_TRACE, "(%5d) leave: dompart\n", Logstamp);
		return(NULL);
	}

	while (*name != SEPARATOR && *name != '\0')
		name++;
	if (*name == SEPARATOR)
		name++;	/* skip over separator	*/

	strcpy(sname,name);
	LOG2(L_TRACE, "(%5d) leave: dompart\n", Logstamp);
	return(sname);
#else
	static char	sname[MAXDNAME+1];
	register char	*pt=sname;
	char	*save=sname;;

	LOG2(L_TRACE, "(%5d) enter: dompart\n", Logstamp);
	if (name == NULL) {
		LOG2(L_TRACE, "(%5d) leave: dompart\n", Logstamp);
		return(NULL);
	}

	while(*pt++ = *name++)
		if (*name == SEPARATOR)
			save = pt;

	*save = NULL;
	LOG2(L_TRACE, "(%5d) leave: dompart\n", Logstamp);
	return(sname);
#endif
}
/**************************************************************
 *
 *	namepart returns the name part of a domain name.
 *
 *	e.g., if RIGHT_TO_LEFT
 *		namepart("a.b.c.d") would return "a"
 *		namepart("a") would return "a"
 *		namepart("") would return "" (ptr to null string)
 *	      if LEFT_TO_RIGHT
 *		namepart("a.b.c.d") would return "d"
 *		namepart("a") would return "a"
 *		namepart("") would return "" (ptr to null string)
 *
 *	namepart returns a pointer into a static buffer
 *	so new calls to namepart overwrite previous results.
 *	An error will cause namepart to return NULL.
 *
 *************************************************************/
char *
namepart(name)
register char	*name;
{
#ifdef RIGHT_TO_LEFT
	static char	sname[MAXDNAME+1];
	register char	*p;

	LOG2(L_TRACE, "(%5d) enter: namepart\n", Logstamp);
	if (name == NULL) {
		LOG2(L_TRACE, "(%5d) leave: namepart\n", Logstamp);
		return(NULL);
	}

	p=sname;

	while (*name !=SEPARATOR && *name != '\0' && p < &sname[NAMSIZ+1])
		*p++ = *name++;

	*p = '\0';
	LOG2(L_TRACE, "(%5d) leave: namepart\n", Logstamp);
	return(sname);
#else
	static char	sname[MAXDNAME+1];
	register char	*pt=name;
	register char	*npt=sname;

	LOG2(L_TRACE, "(%5d) enter: namepart\n", Logstamp);
	while (*name++)
		if (*name == SEPARATOR)
			pt = name+1;
	while (*npt++ = *pt++)
		;
	LOG2(L_TRACE, "(%5d) leave: namepart\n", Logstamp);
	return(sname);
#endif
}
