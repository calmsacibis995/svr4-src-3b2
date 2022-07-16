/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cscope:common/getwd.c	1.1"
/*
 *	char *getwd(dir)
 *
 *	Get the full pathname of the current directory.
 *
 *	returns: a pointer to dir on success.
 *		 NULL on failure.
 *
 *	Current directory upon return from getwd:
 *
 *		success: same as on entry
 *		failure: If a failure is noted during processing getwd
 *			 backs out of it's traversal and restores the
 *			 current directory to what it was upon entry.
 *			 Of course there are pathological corruptions
 *			 to the file system which can totally confuse
 *			 getwd.
 *
 *	Note: This function is based on curdir in -lPW, but is two orders of
 *	magnitude faster.  (curdir was already several orders of magnitude
 *      faster than getcwd(3).)
 */

#include <sys/types.h>	/* needed by stat.h */
#include <sys/stat.h>	/* stat */
#include <stdio.h>	/* NULL */
#include <string.h>	/* string functions */
#include "ndir.h"	/* direct and directory access macros */

/* System V changes this symbol's name and moves it to a different header
   file every release, but its value is the same for System V, BSD, and V9 */
#define ROOTINO	2

#define YES	1
#define NO 	0

char *
getwd(dir)
register char	*dir; 	/* destination buffer for name so far */
{
	register DIR *dirfile;		/* directory file descriptor */
	char	namebuf[256+1];		/* buffer for recursivly building path */
	char	cofp[MAXNAMLEN + 5]; 	/* child of parent name */
	char	*child;			/* child name */
	char	*pwd;			/* PWD environment variable value */
	struct	stat	d_sb;  		/* current directory status */
	struct	stat	dd_sb; 		/* parent directory status */
	struct	stat	tmp_sb; 	/* temporary stat buffer */
	int	atmount = NO;		/* at root of a mounted file system */
	struct	direct	*p;
	char	*getenv();

	/* get the current directory's status */
	if (stat(".", &d_sb) < 0) {
		return(NULL);
	}
	/* use $PWD if it matches this directory */
	if ((pwd = getenv("PWD")) != NULL && *pwd != '\0' && 
	    stat(pwd, &tmp_sb) == 0 &&
	    d_sb.st_ino == tmp_sb.st_ino && d_sb.st_dev == tmp_sb.st_dev) {
		(void) strcpy(dir, pwd);
		return(pwd);
	}
	/* open the parent directory */
	if ((dirfile = opendir("..")) == NULL) {
		return(NULL);
	}
	/* get the current and parent directories status */
	if (statdir(dirfile, &dd_sb) < 0) {
		closedir(dirfile);
		return(NULL);
	}
	/*
	 * Determine if at a mount point or at the root of the root 
	 * file system.
	 */
        if (d_sb.st_ino == ROOTINO ||
	   (d_sb.st_ino == dd_sb.st_ino && d_sb.st_dev == dd_sb.st_dev)) {
		atmount = YES;
		if (d_sb.st_ino == dd_sb.st_ino && d_sb.st_dev == dd_sb.st_dev) {
			(void) strcpy(dir, "/"); /* start out right */
			closedir(dirfile);
			return(dir);  		/* start to unwind */
		}
	}
	/*  find the name of "."  in ".." */
	(void) strcpy(cofp, "../");
	child = cofp + 3;
	for (;;) {
		if ((p = readdir(dirfile)) == NULL) {
			closedir(dirfile);
			return(NULL);
		}
		if (p->d_ino == 0)
			continue;
		if (p->d_ino == d_sb.st_ino || atmount == YES) {
			(void) strcpy(child, p->d_name);
			/* 
			 * Must stat all the files in the parent 
			 * directory if the current directory is a mount
			 * point because stat returns a different inode
			 * number than is in the parent's directory.
			 */
			if (stat(cofp, &tmp_sb) < 0)  {
				closedir(dirfile);
				return(NULL);
			}
			if ((tmp_sb.st_ino == d_sb.st_ino)
			    && (tmp_sb.st_dev == d_sb.st_dev))
				break;  /* found it */
		}
	}
	closedir(dirfile);
	if (chdir("..") < 0)
		return(NULL);

	/* namebuf used as destination for invoking child */
	if (getwd(namebuf) == NULL)  {

		/* failure - backout by changing to child directory */
		(void) chdir(child);
		return(NULL);
	}
	else {
		/* 
		 * As the tail recursion unwinds add the current
		 * directoriy's name to the end of 'namebuf',
		 * and copy 'namebuf' to 'dir', while 
		 * descending the tree to the directory of 
		 * invocation.
		 */
		(void) strcpy(dir, namebuf);
		if (*(namebuf + strlen(namebuf) - 1) != '/') {
			/* previous call didn't find root */
			(void) strcat(dir, "/");
		}
		(void) strcat(dir, child);

		/* return to directory of invocation */
		(void) chdir(child);
		return(dir);
	}
}
