/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head.sys:sys/fcntl.h	11.33"
#ifndef _SYS_FCNTL_H
#define _SYS_FCNTL_H

#ifndef _SYS_TYPES_H
#include "sys/types.h"
#endif

/*
 * Flag values accessible to open(2) and fcntl(2)
 * (the first three can only be set by open).
 */
#define	O_RDONLY	0
#define	O_WRONLY	1
#define	O_RDWR		2
#define	O_NDELAY	0x04	/* non-blocking I/O */
#define	O_APPEND	0x08	/* append (writes guaranteed at the end) */
#define	O_SYNC		0x10	/* synchronous write option */
#define	O_NONBLOCK	0x80	/* non-blocking I/O (POSIX) */

/*
 * Flag values accessible only to open(2).
 */
#define	O_CREAT		0x100	/* open with file create (uses third open arg) */
#define	O_TRUNC		0x200	/* open with truncation */
#define	O_EXCL		0x400	/* exclusive open */
#define	O_NOCTTY	0x800	/* don't allocate controlling tty (POSIX) */

/* fcntl(2) requests */
#define	F_DUPFD		0	/* Duplicate fildes */
#define	F_GETFD		1	/* Get fildes flags */
#define	F_SETFD		2	/* Set fildes flags */
#define	F_GETFL		3	/* Get file flags */
#define	F_SETFL		4	/* Set file flags */
#define	F_SETLK		6	/* Set file lock */
#define	F_SETLKW	7	/* Set file lock and wait */

/*
 * Applications that read /dev/mem must be built like the kernel.  A
 * new symbol "_KMEMUSER" is defined for this purpose.
 */
#if defined(_KERNEL) || defined(_KMEMUSER)
#define	F_GETLK		14	/* Get file lock */
#define	F_O_GETLK	5	/* SVR3 Get file lock */

#else	/* user definition */

#if defined(_STYPES)	/* SVR3 definition */
#define	F_GETLK		5	/* Get file lock */
#else
#define	F_GETLK		14	/* Get file lock */
#endif	/* defined(_STYPES) */

#endif	/* defined(_KERNEL) */

#define	F_SETLK		6	/* Set file lock */
#define	F_SETLKW	7	/* Set file lock and wait */


#define	F_CHKFL		8	/* Unused */
#define	F_ALLOCSP	10	/* Reserved */
#define	F_FREESP	11	/* Free file space */
#define F_BLOCKS	18	/* Get number of BLKSIZE blocks allocated */
#define F_BLKSIZE	19	/* Get optimal I/O block size */ 

#define F_RSETLK	20	/* Remote SETLK for NFS */
#define F_RGETLK	21	/* Remote GETLK for NFS */
#define F_RSETLKW	22	/* Remote SETLKW for NFS */

#define	F_GETOWN	23	/* Get owner */
#define	F_SETOWN	24	/* Set owner */

#define F_CNVT		25	/* For NFS Lock Manager */

#define LOCKMGR		26	/* Indicates lock manager lock */

/*
 * File segment locking set data type - information passed to system by user.
 */
#if defined(_KERNEL) || defined(_KMEMUSER)
	/* EFT definition */
typedef struct flock {
	short	l_type;
	short	l_whence;
	off_t	l_start;
	off_t	l_len;		/* len == 0 means until end of file */
        long	l_sysid;
        pid_t	l_pid;
	long	pad[4];		/* reserve area */
} flock_t;

typedef struct o_flock {
	short	l_type;
	short	l_whence;
	long	l_start;
	long	l_len;		/* len == 0 means until end of file */
        short   l_sysid;
        o_pid_t l_pid;
} o_flock_t;

#else		/* user level definition */

#if defined(_STYPES)
	/* SVR3 definition */
typedef struct flock {
	short	l_type;
	short	l_whence;
	off_t	l_start;
	off_t	l_len;		/* len == 0 means until end of file */
	short	l_sysid;
        o_pid_t	l_pid;
} flock_t;


#else

typedef struct flock {
	short	l_type;
	short	l_whence;
	off_t	l_start;
	off_t	l_len;		/* len == 0 means until end of file */
	long	l_sysid;
        pid_t	l_pid;
	long	pad[4];		/* reserve area */
} flock_t;

#endif	/* defined(_STYPES) */

#endif	/* defined(_KERNEL) */

/*
 * File segment locking types.
 */
#define	F_RDLCK	01	/* Read lock */
#define	F_WRLCK	02	/* Write lock */
#define	F_UNLCK	03	/* Remove lock(s) */

/*
 * POSIX constants 
 */

#define	O_ACCMODE	03477	/* Mask for file access modes */
#define	FD_CLOEXEC	1	/* close on exec flag */

#endif	/* _SYS_FCNTL_H */
