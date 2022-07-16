/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cscope:common/ndir.h	1.3"
#ifdef BSD	/* build command requires #ifdef instead of #if */
#include <sys/dir.h>
#define	statdir(file, statp)	fstat(file->dd_fd, statp)
#else
#if apollo || hp9000s200
#define REALDIRSIZE	sizeof(_dirstruct)
#else
/* allow for a terminating null on the name, which must be the last structure 
   member */
#undef	DIRSIZ
#define DIRSIZ		15
/* can't use sizeof(_dirstruct) - 1 because it may be padded to a word boundry */
#define REALDIRSIZE	(sizeof(o_ino_t) + DIRSIZ - 1)
#endif
#include <sys/dir.h>		/* must be after DIRSIZ definition */
#define MAXNAMLEN	DIRSIZ - 1
#define	DIR		FILE
#define opendir(name)		fopen(name, "r")
#define closedir(file)		(void) fclose(file)
#define rewinddir(file)		rewind(file)
#define	statdir(file, statp)	fstat(fileno(file), statp)
#define readdir(file)		\
	(fread((char *) &_dirstruct, REALDIRSIZE, 1, file) == 1 ? \
	&_dirstruct : (struct direct *) NULL)
static	struct	direct	_dirstruct;
#endif
