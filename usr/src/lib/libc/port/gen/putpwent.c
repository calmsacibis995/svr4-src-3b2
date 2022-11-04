/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/putpwent.c	1.9"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
/*
 * format a password file entry
 */
#ifdef __STDC__
	#pragma weak putpwent = _putpwent
#endif
#include "synonyms.h"
#include <stdio.h>
#include <pwd.h>

#ifdef __STDC__
	int	_putpwbuf(const struct passwd *,
			int (*)(void *, char *, size_t), void *);
#else
	int	_putpwbuf();
#endif

static int
#ifdef __STDC__
fls(void *f, char *buf, size_t n)
#else
fls(f, buf, n)
	char	*f;
	char	*buf;
	size_t	n;
#endif
{
	fwrite(buf, n, 1, (FILE *)f);
	return ferror(((FILE *)f));
}


int
putpwent(p, f)
register const struct passwd *p;
register FILE *f;
{
	int	err;

	err = _putpwbuf(p, fls, (char *)f);
	(void)fflush(f);
	return err | ferror(f);
}
