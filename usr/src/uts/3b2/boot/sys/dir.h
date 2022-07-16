/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/sys/dir.h	11.2"
#ifndef	DIRSIZ
#define	DIRSIZ	14
#endif
struct	direct
{
	o_ino_t	d_ino;
	char	d_name[DIRSIZ];
};
