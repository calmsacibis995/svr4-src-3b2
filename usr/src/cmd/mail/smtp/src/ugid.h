/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:smtp/src/ugid.h	1.3"
/* ident "@(#)ugid.h	1.2 'attmail mail(1) command'" */
/* structure to represent uid, gid; used by mboxowner() */

struct ugid {
	int uid;
	int gid;
};
