/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#ident	"@(#)head.sys:sys/stat.h	11.22"

#include "sys/time.h"

#define ST_FSTYPSZ 16		/* array size for file system type name */

/*
 * stat structure, used by stat(2) and fstat(2)
 */

#if defined(_KERNEL)

	/* SVID stat struct */
struct	stat {
	o_dev_t	st_dev;
	o_ino_t	st_ino;
	o_mode_t st_mode;
	o_nlink_t st_nlink;
	o_uid_t st_uid;
	o_gid_t st_gid;
	o_dev_t	st_rdev;
	off_t	st_size;
	time_t	st_atime;
	time_t	st_mtime;
	time_t	st_ctime;
};
	/* Expanded stat structure */ 

struct	xstat {
	dev_t	st_dev;
	long	st_pad1[3];	/* reserve for dev expansion, sysid definition */
	ino_t	st_ino;
	mode_t	st_mode;
	nlink_t st_nlink;
	uid_t 	st_uid;
	gid_t 	st_gid;
	dev_t	st_rdev;
	long	st_pad2[2];
	off_t	st_size;
	long	st_pad3;	/* reserve pad for future off_t expansion */
	timestruc_t st_atime;
	timestruc_t st_mtime;
	timestruc_t st_ctime;
	long	st_blksize;
	long	st_blocks;
	char	st_fstype[ST_FSTYPSZ];
	long	st_pad4[8];	/* expansion area */
};

#elif !defined(_STYPES)	/* user level 4.0 stat struct */

/* maps to kernel struct xstat */
struct	stat {
	dev_t	st_dev;
	long	st_pad1[3];	/* reserved for network id */
	ino_t	st_ino;
	mode_t	st_mode;
	nlink_t st_nlink;
	uid_t 	st_uid;
	gid_t 	st_gid;
	dev_t	st_rdev;
	long	st_pad2[2];
	off_t	st_size;
	long	st_pad3;	/* future off_t expansion */
	timestruc_t st_atim;	
	timestruc_t st_mtim;	
	timestruc_t st_ctim;	
	long	st_blksize;
	long	st_blocks;
	char	st_fstype[ST_FSTYPSZ];
	long	st_pad4[8];	/* expansion area */

};
#define st_atime	st_atim.tv_sec
#define st_mtime	st_mtim.tv_sec
#define st_ctime	st_ctim.tv_sec

#else	/*  SVID Issue 2 stat */

struct	stat {
	o_dev_t	st_dev;
	o_ino_t	st_ino;
	o_mode_t st_mode;
	o_nlink_t st_nlink;
	o_uid_t	st_uid;
	o_gid_t	st_gid;
	o_dev_t	st_rdev;
	off_t	st_size;
	time_t	st_atime;
	time_t	st_mtime;
	time_t	st_ctime;
};
#endif	/* end defined(_KERNEL) */


/* MODE MASKS */

#define	S_IFMT	0xF000		/* type of file */
#define S_IAMB	0x1FF		/* access mode bits */
#define		S_IFIFO	0x1000	/* fifo */
#define		S_IFCHR	0x2000	/* character special */
#define		S_IFDIR	0x4000	/* directory */
#define		S_IFNAM 0x5000  /* XENIX special named file */
#define			S_INSEM 0x1	/* XENIX semaphore subtype of IFNAM */
#define			S_INSHD 0x2	/* XENIX shared data subtype of IFNAM */
#define		S_IFBLK	0x6000	/* block special */
#define		S_IFREG	0x8000	/* regular */
#define		S_IFLNK	0xA000	/* symbolic link */
#define	S_ISUID	04000		/* set user id on execution */
#define	S_ISGID	02000		/* set group id on execution */
#define	S_ISVTX	01000		/* save swapped text even after use */
#define	S_IREAD	00400		/* read permission, owner */
#define	S_IWRITE 00200		/* write permission, owner */
#define	S_IEXEC	00100		/* execute/search permission, owner */
#define	S_ENFMT	S_ISGID		/* record locking enforcement flag */
#define	S_IRWXU	00700		/* read, write, execute: owner */
#define	S_IRUSR	00400		/* read permission: owner */
#define	S_IWUSR	00200		/* write permission: owner */
#define	S_IXUSR	00100		/* execute permission: owner */
#define	S_IRWXG	00070		/* read, write, execute: group */
#define	S_IRGRP	00040		/* read permission: group */
#define	S_IWGRP	00020		/* write permission: group */
#define	S_IXGRP	00010		/* execute permission: group */
#define	S_IRWXO	00007		/* read, write, execute: other */
#define	S_IROTH	00004		/* read permission: other */
#define	S_IWOTH	00002		/* write permission: other */
#define	S_IXOTH	00001		/* execute permission: other */

/* the following macros are for POSIX conformance */

#define S_ISFIFO(mode)	(mode & S_IFIFO)
#define S_ISCHR(mode)	(mode & S_IFCHR)
#define S_ISDIR(mode)	(mode & S_IFDIR)
#define S_ISBLK(mode)	(mode & S_IFBLK)
#define S_ISREG(mode)	(mode & S_IFREG) 


/* a version number is part of the new stat function interface */


#define R3_MKNOD_VER 1		/* SVR3.0 mknod */
#define MKNOD_VER 2		/* current version of mknod */
#define R3_STAT_VER 1		/* SVR3.0 stat */
#define STAT_VER 2		/* current version of stat */

#if !defined(_KERNEL)
#if defined(__STDC__)

#if !defined(_STYPES)
static int fstat(int, struct stat *);
static int stat(const char *, struct stat *);
static int lstat(const char *, struct stat *);
static int mknod(const char *, mode_t, dev_t);
#else
int fstat(int, struct stat *);
int stat(const char *, struct stat *);
int lstat(const char *, struct stat *);
int mknod(const char *, mode_t, dev_t);
#endif

int _fxstat(const int, int, struct stat *);
int _xstat(const int, const char *, struct stat *);
int _lxstat(const int, const char *, struct stat *);
int _xmknod(const int, const char *, mode_t, dev_t);
extern int chmod(const char *, mode_t);
extern int mkdir(const char *, mode_t);
extern int mkfifo(const char *, mode_t);
extern mode_t umask(mode_t);

#else	/* !__STDC__ */

#if !defined(_STYPES)
static int fstat(), stat(), lstat();
static int mknod();
#else
int fstat(), stat(), lstat();
int mknod();
#endif

int _fxstat(), _xstat(), _lxstat();
int _xmknod();
extern int chmod();
extern int mkdir();
extern int mkfifo();
extern mode_t umask();

#endif /* defined(__STDC__) */
#endif /* !defined(_KERNEL) */

/*
 * NOTE: Application software should NOT program 
 * to the _xstat interface.
 */

#if !defined(_STYPES) && !defined(_KERNEL)
static int
stat(path, buf)
const char *path;
struct stat *buf;
{
int ret;
	ret = _xstat(STAT_VER, path, buf);
	return ret; 
}

static int
lstat(path, buf)
const char *path;
struct stat *buf;
{
int ret;
	ret = _lxstat(STAT_VER, path, buf);
	return ret;
}

static int
fstat(fd, buf)
int fd;
struct stat *buf;
{
int ret;
	ret = _fxstat(STAT_VER, fd, buf);
	return ret;
}

static int
mknod(path, mode, dev)
const char *path;
mode_t mode;
dev_t dev;
{
int ret;
	ret = _xmknod(MKNOD_VER, path, mode, dev);
	return ret;
}

#endif
/*			Function prototypes
 *			___________________
 *
 * fstat()/stat() used for NON-EFT case - functions defined in libc.
 * fxstat/xstat/lxstat are called indirectly from fstat/stat/lstat when EFT is 
 * enabled.
 */



#endif	/* _SYS_STAT_H */
