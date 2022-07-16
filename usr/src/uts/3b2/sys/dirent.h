/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H

#ident	"@(#)head.sys:sys/dirent.h	11.11"

/*
 * File-system independent directory entry.
 */
struct dirent {
	ino_t		d_ino;		/* "inode number" of entry */
	off_t		d_off;		/* offset of disk directory entry */
	unsigned short	d_reclen;	/* length of this record */
	char		d_name[1];	/* name of file */
};

typedef	struct	dirent	dirent_t;

#if defined(__STDC__) && !defined(_KERNEL)
int getdents(int, struct dirent *, unsigned);
#else
int getdents( );
#endif

#endif	/* _SYS_DIRENT_H */
