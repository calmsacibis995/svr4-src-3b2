/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:bkdaemon.d/file.c	1.2"

#include <sys/types.h>
#include <sys/stat.h>

int
f_exists( fname )
char *fname;
{
	struct stat buf;
	return( !stat( fname, &buf ) );
}
