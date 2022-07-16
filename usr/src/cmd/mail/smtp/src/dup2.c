/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:smtp/src/dup2.c	1.3"
#ident "@(#)dup2.c	1.2 'attmail mail(1) command'"
#include <fcntl.h>

int
dup2(a,b)
	int a;
	int b;
{
	if (a==b)
		return 0;
	close(b);
	return fcntl(a, F_DUPFD, b);
}
