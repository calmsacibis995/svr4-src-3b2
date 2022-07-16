/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/tcgetattr.c	1.1"

#ifdef __STDC__
	#pragma weak tcgetattr = _tcgetattr
#endif
#include "synonyms.h"
#include <sys/termios.h>

/* 
 * get parameters associated with fildes and store them in termios
 */

int tcgetattr (fildes, termios_p)
int fildes;
struct termios *termios_p;
{
	return(ioctl(fildes,TCGETS,termios_p));
}
