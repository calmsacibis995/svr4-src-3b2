/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/rename.c	1.9"
/*LINTLIBRARY*/
#include "synonyms.h"
#include <sys/types.h>
#include <sys/stat.h>

extern int link(), unlink(), uname(), _rename();

int
remove(filename)
const char *filename;
{
	struct stat	statb;

	/* If filename is not a directory, call unlink(filename)
	 * Otherwise, call rmdir(filename) */

	if (stat(filename, &statb) != 0)
		return(-1);
	if ((statb.st_mode & S_IFMT) != S_IFDIR)
		return(unlink(filename));
	return(rmdir(filename));
}

int
rename(old, new)
const char *old;
const char *new;
{
#ifndef _STYPES
	return(_rename(old, new));
#else
	if (link(old, new) < 0)
		return(-1);
	if (unlink(old) < 0) {
		(void)unlink(new);
		return(-1);
	}
	return(0);
#endif
}
