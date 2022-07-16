/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_S5DIR_H
#define _FS_S5DIR_H

#ident	"@(#)head.sys:sys/fs/s5dir.h	11.4"
#ifndef	DIRSIZ
#define	DIRSIZ	14
#endif
struct	direct
{
	o_ino_t	d_ino;		/* s5 inode type */
	char	d_name[DIRSIZ];
};

#define	SDSIZ	(sizeof(struct direct))

#endif	/* _FS_S5DIR_H */
