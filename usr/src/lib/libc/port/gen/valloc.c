/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/valloc.c	1.1"

#ifdef __STDC__
	#pragma weak valloc = _valloc
#endif

#include "synonyms.h"
#include <stdlib.h>
#include <malloc.h>

extern	unsigned getpagesize();
extern VOID * memalign();

VOID *
valloc(size)
	size_t size;
{
	static unsigned pagesize;
	if (!pagesize)
		pagesize = getpagesize();
	return memalign(pagesize, size);
}
