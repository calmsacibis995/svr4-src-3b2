/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:abspath.c	1.3"
#ident "@(#)abspath.c	1.4 'attmail mail(1) command'"
/*
    NAME
	abspath - expand a path relative to some `.'

    SYNOPSIS
	string *abspath(char *path, char *dot, string *to)

    DESCRIPTION
	If "path" is relative (does not start with `.'), the
	the value of "dot" will be prepended and the result
	returned in "to". Otherwise, the value of "path" is
	returned in "to".
*/
#include "mail.h"

extern string *
abspath(path, dot, to)
	char *path;
	char *dot;
	string *to;
{
	if (*path == '/') {
		to = s_append(to, path);
	} else {
		to = s_append(to, dot);
		to = s_append(to, path);
	}
	return to;
}
