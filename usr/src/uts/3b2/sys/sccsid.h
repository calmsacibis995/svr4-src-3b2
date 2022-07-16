/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_SCCSID_H
#define _SYS_SCCSID_H

#ident	"@(#)head.sys:sys/sccsid.h	11.4"
#ifdef lint
#define VERSION(x);
#define HVERSION(x);
#else
#define VERSION(x) static char sccsid[]="x";
#define HVERSION(n,x) static char n[]="x";
#endif

#endif	/* _SYS_SCCSID_H */
