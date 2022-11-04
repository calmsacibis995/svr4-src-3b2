/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getgrbuf.c	1.1"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
#include "synonyms.h"
#include <stdlib.h>
#include <string.h>
#include <grp.h>


static char *
grskip(p, c)
char *p;
int c;
{
	while (*p != '\0' && *p != c)
		++p;
	if (*p != '\0')
	 	*p++ = '\0';
	return(p);
}

struct group *
#ifdef __STDC__
_getgrbuf(struct group *grp, char *line, void *(*alloc)(size_t))
#else
_getgrbuf(grp, line, alloc)
	struct group	*grp;
	char		*line;
	char		(*alloc)();
#endif
{
	char *p = line, **q;

	grp->gr_name = p;
	grp->gr_passwd = p = grskip(p, ':');
	grp->gr_gid = atol(p = grskip(p, ':'));
	p = grskip(p, ':');
	(void) grskip(p, '\n');
	{
		register char	*x = p;
		register size_t	count = 2;	/* last + null */
		while (*x != '\0')
			if (*x++ == ',')
				++count;
		if ((q = (char **)(*alloc)(count * sizeof(char *))) == 0)
			return 0;
	}
	grp->gr_mem = q;
	while (*p != '\0')
	{
		*q++ = p;
		p = grskip(p, ',');
	}
	*q = 0;
	return grp;
}
