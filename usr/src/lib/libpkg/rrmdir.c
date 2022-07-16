/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)libpkg:rrmdir.c	1.4"
#include <limits.h>

extern int	sprintf(),
		system();

int
rrmdir(path)
char *path;
{
	char cmd[PATH_MAX+13];

	(void) sprintf(cmd, "/bin/rm -rf %s", path);
	return(system(cmd) ? 1 : 0);
}
