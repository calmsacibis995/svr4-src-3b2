/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:smtp/src/s5sysname.c	1.3"
#ident "@(#)s5sysname.c	1.3 'attmail mail(1) command'"
/* get the system's name -- System V */

#include <sys/utsname.h>

extern char *
sysname_read()
{
	static struct utsname s;

	if (uname(&s)<0)
		return "kremvax";
	return s.nodename;
}
