/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_INFO_H
#define _SYS_INFO_H

#ident	"@(#)head.sys:sys/info.h	1.2"
/*
 *   Header that will contain system info. Version and release number. 
 *
 */

struct sysinfo {
	short release;
	short version;
};

#endif	/* _SYS_INFO_H */
