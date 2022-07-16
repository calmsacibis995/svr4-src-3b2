/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head:dirent.h	1.6.1.3"

#ifndef _DIRENT_H
#define _DIRENT_H

#define MAXNAMLEN	512		/* maximum filename length */
#define DIRBUF		1048		/* buffer size for fs-indep. dirs */

#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

typedef struct
	{
	int		dd_fd;		/* file descriptor */
	int		dd_loc;		/* offset in block */
	int		dd_size;	/* amount of valid data */
	char		*dd_buf;	/* directory block */
	}	DIR;			/* stream data from opendir() */

#if defined(__STDC__)

extern DIR		*opendir( const char * );
extern struct dirent	*readdir( DIR * );
extern long		telldir( DIR * );
extern void		seekdir( DIR *, long );
extern void		rewinddir( DIR * );
extern int		closedir( DIR * );

#else

extern DIR		*opendir();
extern struct dirent	*readdir();
extern long		telldir();
extern void		seekdir();
extern void		rewinddir();
extern int		closedir();

#endif

#define rewinddir( dirp )	seekdir( dirp, 0L )

#ifndef _SYS_DIRENT_H
#include <sys/dirent.h>
#endif

#endif	/* _DIRENT_H */
