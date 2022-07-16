/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:basename.c	1.3"
#ident "@(#)basename.c	1.4 'attmail mail(1) command'"
/*
    NAME
	basename - return base from pathname

    SYNOPSIS
	char *basename(char *path)

    DESCRIPTION
	basename() returns a pointer to the base
	component of a pathname.
*/
#include "mail.h"

char *
basename(path)
	char *path;
{
	char *cp;

	cp = strrchr(path, '/');
	return cp==NULL ? path : cp+1;
}
