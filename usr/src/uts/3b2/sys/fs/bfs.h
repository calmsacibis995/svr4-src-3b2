/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_BFS_H
#define _FS_BFS_H

#ident	"@(#)head.sys:sys/fs/bfs.h	1.8"
#define BFS_MAXFNLEN 14			/* Maximum file length */
#define BFS_MAXFNLENN (BFS_MAXFNLEN+1)  /* Used for NULL terminated copies */

struct bfsvattr {
	long		va_mask;	/* bit-mask of attributes */
	vtype_t		va_type;	/* vnode type (for create) */
	mode_t		va_mode;	/* file access mode */
	uid_t		va_uid;		/* owner user id */
	uid_t		va_gid;		/* owner group id */
	dev_t		va_fsid;	/* file system id (dev for now) */
	o_ino_t		va_nodeid;	/* node id */
	nlink_t		va_nlink;	/* number of references to file */
	ulong		va_size;	/* file size in bytes */
	time_t		va_atime;	/* time of last access */
	time_t		va_mtime;	/* time of last modification */
	time_t		va_ctime;	/* time file ``created'' */
	dev_t		va_rdev;	/* device the file represents */
	ulong		va_blksize;	/* fundamental block size */
	ulong		va_nblocks;	/* # of blocks allocated */
	long		va_filler[7];	/* padding */
};

/*
 * The bfs_dirent is the "inode" of BFS.  Always on disk, it is pointed
 * to (by disk offset) by the vnode and is referenced every time an
 * operation is done on the vnode.  It must be referenced every time,
 * as things can move around very quickly
 */
struct bfs_dirent
{
	o_ino_t d_ino;				/* inode */
	daddr_t d_sblock;			/* Start block */
	daddr_t d_eblock;			/* End block */
	daddr_t d_eoffset;			/* EOF disk offset (absolute) */
	struct bfsvattr d_fattr;		/* File attributes */
};


struct bfs_ldirs {
	o_ino_t l_ino;
	char   l_name[BFS_MAXFNLEN];
};

/*
 * We keep a linked list of all referenced BFS vnodes.  bfs_inactive will remove
 * them from the list, and bfs_fillvnode will add to and search through the list
 */
struct bfs_core_vnode
{
	struct vnode *core_vnode;
	struct bfs_core_vnode *core_next;
};

/*
 * The BFS superbuf contains all private data about a given BFS filesystem.
 * It is pointed to by the data field of the vfs structure and is thus passed
 * to every vfsop and vnodeops even if indirectly
 */
struct bsuper
{
	off_t bsup_start;		/* The filesystem data start offset */
	off_t bsup_end;			/* The filesystem data end offset */
	long bsup_freeblocks;		/* # of freeblocks (for statfs) */
	long bsup_freedrents;		/* # of free dir entries (for statfs) */
	struct vnode *bsup_devnode;	/* The device special vnode */
	struct vnode *bsup_root;	/* Root vnode */
	off_t bsup_lastfile;		/* Last file directory offset */

	/* Linked vnode list */

	struct bfs_core_vnode *bsup_incore_vlist;	

	/*
	 * bsup_ioinprog is the count of the number of io operations is 
	 * in progress.  Compaction routines sleep on this being zero
	 */
	ushort bsup_ioinprog;
	struct vnode *bsup_writelock;	/* The file which is open for write */

	/* Booleans */

	unsigned char bsup_fslocked;	/* Fs is locked when compacting */
	unsigned char bsup_compacted;	/* Fs compacted, no removes done */
};

/* The disk superbuff */
struct bdsuper
{
	long bdsup_bfsmagic;		/* Magic number */
	off_t bdsup_start;		/* Filesystem data start offset */
	off_t bdsup_end;		/* Filesystem data end offset */

	/*
	 * The next four words are used to promote sanity in compaction.  Used
	 * correctly, a crash at any point during compaction is recoverable
	 */
	daddr_t bdcp_fromblock;		/* "From" block of current transfer */
	daddr_t bdcp_toblock;		/* "To" block of current transfer */
	daddr_t bdcpb_fromblock;	/* Backup of "from" block */
	daddr_t bdcpb_toblock;		/* Backup of "to" block */
};

/* Used to overlay the kernel struct fid */
struct bfs_fid_overlay
{
	ushort o_len;
	long o_offset;
};

/* Used to overlay the kernel struct dirent */
struct bfs_drent_overlay
{
	ino_t d_ino;
	off_t d_off;
	unsigned short d_reclen;
	char d_name[BFS_MAXFNLENN];
};


#define BFS_MAGIC	016234671456
#define BFS_SUPEROFF	0
#define BFS_DIRSTART	(BFS_SUPEROFF + sizeof(struct bdsuper))
#define BFS_ROOTOFF 	(&(((struct bdsuper *)BFS_SUPEROFF)->bdsup_rootattrs))
#define BFS_BSIZE	512
#define BFS_ULT		(ulong)5000
#define BFS_DEVNODE(vfsp) ((struct bsuper *)vfsp->vfs_data)->bsup_devnode
#define BFS_YES		(char)1
#define BFS_NO		(char)0
#define CHUNKSIZE	4096
#define BIGFILE		500
#define SMALLFILE	10
#define BFSROOTINO	2
#define DIRBUFSIZE	1024	/* Typical size in bytes of directory 	  */

#define BFS_OFF2INO(offset) \
	((offset - BFS_DIRSTART) / sizeof(struct bfs_dirent)) + BFSROOTINO

#define BFS_INO2OFF(inode) \
	((inode - BFSROOTINO) * sizeof(struct bfs_dirent)) + BFS_DIRSTART

#define BFS_GETINODE(bvp, offset, buf, cr) \
	vn_rdwr(UIO_READ, bvp, (caddr_t)buf, sizeof(struct bfs_dirent), \
				offset, UIO_SYSSPACE, 0, 0, cr, 0)

#define BFS_PUTINODE(bvp, offset, buf, cr) \
	vn_rdwr(UIO_WRITE, bvp, (caddr_t)buf, sizeof(struct bfs_dirent), \
			offset, UIO_SYSSPACE, IO_SYNC, BFS_ULT, cr, (int *)0)

#define BFS_GETDIRLIST(bvp, offset, buf, len, cr) \
	vn_rdwr(UIO_READ, bvp, buf, len, offset, UIO_SYSSPACE, 0, 0, cr, 0)

#define CHECK_LOCK(bs) \
	if (bs->bsup_fslocked) \
		while (bs->bsup_fslocked) \
			sleep((caddr_t)&bs->bsup_fslocked, PINOD)


#define BFS_LOCK(bs) bs->bsup_fslocked = BFS_YES


#define BFS_IOBEGIN(bs) bs->bsup_ioinprog++

#define BFS_IOEND(bs) if (!(--bs->bsup_ioinprog)) \
			wakeup((caddr_t)&bs->bsup_ioinprog)

#endif	/* _FS_BFS_H */
