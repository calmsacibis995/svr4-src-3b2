/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/putpwbuf.c	1.1"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
/*
 * format a password file entry
 */

#include "synonyms.h"
#include <stdio.h>	/* BUFSIZ only! nothing else */
#include <string.h>
#include <pwd.h>


typedef struct
{
	char	*buf;
	char	*ptr;
	size_t	cnt;
#ifdef __STDC__
	int	(*fls)(void *, char *, size_t);
	void	*iop;
#else
	int	(*fls)();
	char	*iop;
#endif
} Buf;

#ifdef __STDC__
	#undef	const
#else
	#undef	const
	#define	const
#endif

static const char	colon[] = ":",
			comma[] = ",",
			newline[] = "\n";

static int
copy(b, p)
	register
	Buf		*b;
	register
	const char	*p;
{
	register size_t	len, left;
	int		err = 0;

	if (p == 0)
	{
		if (b->cnt == BUFSIZ)
			return 0;
		return (*b->fls)(b->iop, b->buf, (size_t)(BUFSIZ - b->cnt));
	}
	len = strlen(p);
	while (len != 0)
	{
		if (b->cnt == 0)
		{
			err |= (*b->fls)(b->iop, b->buf, (size_t)BUFSIZ);
			b->ptr = b->buf;
			b->cnt = BUFSIZ;
		}
		left = b->cnt;
		if (len < left)
			left = len;
		(void)memcpy(b->ptr, p, left);
		b->ptr += left;
		b->cnt -= left;
		p += left;
		len -= left;
	}
	return err;
}


static size_t
utoa(val, buf, buflen)
	unsigned	val;
	char		*buf;
	size_t		buflen;
{
	static
	const char	digs[] = "0123456789";
	size_t		len = 0;
	unsigned	d;

	d = val % 10;
	val /= 10;
	if (val != 0 && buflen > 2)
		len = utoa(val, buf, buflen - 1);
	buf[len] = digs[d];
	buf[++len] = '\0';
	return len;
}


int
#ifdef __STDC__
_putpwbuf(register const struct passwd *p,
	int (*fls)(void *, char *, size_t), void *iop)
#else
_putpwbuf(p, fls, iop)
	register const struct passwd *p;
	register int (*fls)();
	char *iop;
#endif
{
	char	line[BUFSIZ];
	char	num[100];
	Buf	b;
	int	err = 0;

	b.buf = line;
	b.ptr = line;
	b.cnt = BUFSIZ;
	b.fls = fls;
	b.iop = iop;

	err |= copy(&b, p->pw_name);
	err |= copy(&b, colon);
	err |= copy(&b, p->pw_passwd);
	if((*p->pw_age) != '\0')
	{
		err |= copy(&b, comma);
		err |= copy(&b, p->pw_age);
	}
	err |= copy(&b, colon);
	(void)utoa((unsigned)p->pw_uid, num, sizeof(num));
	err |= copy(&b, num);
	err |= copy(&b, colon);
	utoa((unsigned)p->pw_gid, num, sizeof(num));
	err |= copy(&b, num);
	err |= copy(&b, colon);
	err |= copy(&b, p->pw_gecos);
	err |= copy(&b, colon);
	err |= copy(&b, p->pw_dir);
	err |= copy(&b, colon);
	err |= copy(&b, p->pw_shell);
	err |= copy(&b, newline);
	return err | copy(&b, (char *)0);
}
