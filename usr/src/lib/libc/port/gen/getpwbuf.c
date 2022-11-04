/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getpwbuf.c	1.1"
/*LINTLIBRARY*/
#include "synonyms.h"
#include <sys/types.h>
#include <sys/param.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <errno.h>


static char *
pwskip(p)
register char *p;
{
	while(*p && *p != ':' && *p != '\n')
		++p;
	if(*p == '\n')
		*p = '\0';
	else if(*p)
		*p++ = '\0';
	return(p);
}


struct passwd *
_getpwbuf(pwd, line)
	register
	struct passwd	*pwd;
	char		*line;
{
	register char *p = line;
	char *end;
	long	x;

	pwd->pw_name = p;
	p = pwskip(p);
	pwd->pw_passwd = p;
	p = pwskip(p);
	if (p == NULL || *p == ':') {
		/* check for non-null uid */
		errno = EINVAL;
		return (NULL);
	}
	x = strtol(p, &end, 10);	
	if (end != memchr(p, ':', strlen(p))){
		/* check for numeric value */
		errno = EINVAL;
		return (NULL);
	}
	p = pwskip(p);
	pwd->pw_uid = (x < 0 || x > MAXUID)? (UID_NOBODY): x;
	if (p == NULL || *p == ':') {
		/* check for non-null uid */
		errno = EINVAL;
		return (NULL);
	}
	x = strtol(p, &end, 10);	
	if (end != memchr(p, ':', strlen(p))) {
		/* check for numeric value */
		errno = EINVAL;
		return (NULL);
	}
	p = pwskip(p);
	pwd->pw_gid = (x < 0 || x > MAXUID)? (UID_NOBODY): x;
	pwd->pw_comment = p;
	pwd->pw_gecos = p;
	p = pwskip(p);
	pwd->pw_dir = p;
	p = pwskip(p);
	pwd->pw_shell = p;
	p = pwskip(p);

	p = pwd->pw_passwd;
	while(*p && *p != ',')
		p++;
	if(*p)
		*p++ = '\0';
	pwd->pw_age = p;
	return pwd;
}
