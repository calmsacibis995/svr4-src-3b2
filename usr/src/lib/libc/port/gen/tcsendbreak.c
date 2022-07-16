/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/tcsendbreak.c	1.2"

#ifdef __STDC__
	#pragma weak tcsendbreak = _tcsendbreak
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/termios.h>

/* 
 * send zeros for 0.25 seconds, if duration is 0
 */

int tcsendbreak (fildes, duration)
int fildes;
int duration;
{
	return(ioctl(fildes,TCSBRK,duration));
}
