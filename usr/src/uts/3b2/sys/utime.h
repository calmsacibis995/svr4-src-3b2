/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head.sys:sys/utime.h	1.3"
/* utimbuf is used by utime(2) */

#ifndef _SYS_UTIME_H
#define _SYS_UTIME_H

#ifndef _SYS_TYPES_H
#include "sys/types.h"
#endif

struct utimbuf {
	time_t actime;		/* access time */
	time_t modtime;		/* modification time */
};

#endif	/* _SYS_UTIME_H */
