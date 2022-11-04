/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/sysconf.c	1.3"

/* sysconf(3C) - returns system configuration information
*/

#ifdef __STDC__
	#pragma weak sysconf = _sysconf
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysconfig.h>
#include <sys/errno.h>
#include <limits.h>
#include <time.h>

extern int errno;

sysconf(name)
int name;
{

	switch(name) {
		default:
			errno = EINVAL;
			return(-1);

		case _SC_ARG_MAX:
			return(ARG_MAX);

		case _SC_CLK_TCK:
			return(CLOCKS_PER_SEC);

		case _SC_JOB_CONTROL:
			return(_POSIX_JOB_CONTROL);

		case _SC_SAVED_IDS:
			return(_POSIX_SAVED_IDS);

		case _SC_CHILD_MAX:
			return(_sysconfig(_CONFIG_CHILD_MAX));

		case _SC_NGROUPS_MAX:
			return(_sysconfig(_CONFIG_NGROUPS));

		case _SC_OPEN_MAX:
			return(_sysconfig(_CONFIG_OPEN_FILES));

		case _SC_VERSION:
			return(_sysconfig(_CONFIG_POSIX_VER));

		case _SC_PAGESIZE:
			return(_sysconfig(_CONFIG_PAGESIZE));
	}
}

