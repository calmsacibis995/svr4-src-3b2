/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getgrent.c	1.14"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
#ifdef __STDC__
	#pragma weak getgrent = _getgrent
	#pragma weak fgetgrent = _fgetgrent
	#pragma weak endgrent = _endgrent
	#pragma weak setgrent = _setgrent
#endif
#include "synonyms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <grp.h>

#ifdef __STDC__
	struct group	*_getgrbuf(struct group *, char *, void *(*)(size_t));
#else
	struct group	*_getgrbuf();
	char		*malloc();
	char		*realloc();
#endif

static const char GROUP[] = "/etc/group";
static FILE *grf;
static char *line, *gr_mem;
static struct group grp;
static size_t size, gr_size;

#ifdef __STDC__
static void *
alloc(size_t size)
#else
static char *
alloc(size)
	size_t	size;
#endif
{
	if (gr_mem != 0)
	{
		if (gr_size >= size)
			return gr_mem;
		free(gr_mem);
	}
	gr_size = size;
	return gr_mem = malloc(size);
}

void
setgrent()
{
	if (grf == NULL)
		grf = fopen(GROUP, "r");
	else
		rewind(grf);
}

void
endgrent()
{
	if (grf != NULL)
	{
		(void) fclose(grf);
		grf = NULL;
	}
}

static void
cleanup()
{
	if (line != NULL)
	{
		free(line);
		line = NULL;
	}
	if (gr_mem != NULL)
	{
		free (gr_mem);
		gr_mem = NULL;
	}
	(void) endgrent();
}

struct group *
getgrent()
{
	extern struct group *fgetgrent();

	if (grf == NULL && (grf = fopen(GROUP, "r")) == NULL)
		return(NULL);
	return (fgetgrent(grf));
}

struct group *
fgetgrent(f)
FILE *f;
{
	char *p;
	int len;
	long offset;
	char done;

	if (line == NULL)
	{
		size = BUFSIZ+1;
		if ((line = malloc(size)) == NULL)
		{
			(void) cleanup();
			return(NULL);
		}
	}
	done = 0;
	while (!done)
	{
		offset = ftell(f);
		if ((p = fgets(line, size, f)) == NULL)
			return(NULL);
		len = strlen(p);
		if ((len <= size) && (p[len-1] == '\n'))
			done = 1;
		else
		{
			size *= 32;
			if ((line = realloc(line, size)) == NULL)
			{
				(void) cleanup();
				return(NULL);
			}
			fseek(f, offset, 0);
		}
	}
	return _getgrbuf(&grp, line, alloc);
}
