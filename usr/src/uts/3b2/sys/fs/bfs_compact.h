/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_BFS_COMPACT_H
#define _FS_BFS_COMPACT_H

#ident	"@(#)head.sys:sys/fs/bfs_compact.h	1.5"
#ifdef _KERNEL

#define BFS_CCT_GETINODE(bvp, offset, buf,cr) \
	vn_rdwr(UIO_READ, bvp, (caddr_t)buf, sizeof(struct bfs_dirent), \
				offset, UIO_SYSSPACE, 0, 0, cr, 0)

#define BFS_CCT_READ(bvp, offset, len, buf,cr) \
	vn_rdwr(UIO_READ, bvp, (caddr_t)buf, len, offset, \
				UIO_SYSSPACE, 0, 0, cr, 0)

#define BFS_CCT_WRITE(bvp, offset, len, buf,cr) \
	vn_rdwr(UIO_WRITE, bvp, (caddr_t)buf, len, offset, \
				UIO_SYSSPACE, IO_SYNC, BFS_ULT, cr, (int *)0)

#define BFS_CCT_PUTINODE(bvp, offset, buf,cr) \
	vn_rdwr(UIO_WRITE, bvp, (caddr_t)buf, sizeof(struct bfs_dirent), offset, \
				UIO_SYSSPACE, IO_SYNC, BFS_ULT, cr, (int *)0)

#else

#define BFS_CCT_GETINODE(fd, offset, buf, cr) \
	seek_read(fd, offset, sizeof(struct bfs_dirent), buf);

#define BFS_CCT_READ(fd, offset, len, buf, cr) \
	seek_read(fd, offset, len, buf);

#define BFS_CCT_WRITE(fd, offset, len, buf, cr) \
	seek_write(fd, offset, len, buf);

#define BFS_CCT_PUTINODE(fd, offset, buf, cr) \
	seek_write(fd, offset, sizeof(struct bfs_dirent), buf);
#endif

#define BFS_COMPACTSTART (BFS_SUPEROFF + (sizeof(long)*3))

#endif	/* _FS_BFS_COMPACT_H */
