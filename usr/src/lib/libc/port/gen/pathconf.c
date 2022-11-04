/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/pathconf.c	1.2"
/*
** pathconf(3c) - returns system configuration information based
** on path. This code provides a POSIX pathconf function.
**
*/

#ifdef __STDC__
	#pragma weak pathconf = _pathconf
	#pragma weak fpathconf = _fpathconf
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <errno.h>
#include <stddef.h>
#include <limits.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/sysconfig.h>


pathconf(path, name)
char *path;
int name;
{
struct statvfs vfsbuf;
	
	switch(name) {

	default:
		errno = EINVAL;
		return(-1);

	case _PC_LINK_MAX:
		return(LINK_MAX);	/* defined in limits.h */

	case _PC_MAX_CANON:
		return(MAX_CANON);	/* defined in termios.h */

	case _PC_MAX_INPUT:
		return(MAX_INPUT);	/* defined in termios.h */

	case _PC_PATH_MAX:
		return(PATH_MAX);	/* defined in limits.h */

	case _PC_PIPE_BUF:
		return(PIPE_BUF);	/* defined in limits.h */

	case _PC_NO_TRUNC:
		if (path != NULL) {
			if (statvfs(path, &vfsbuf)) 
				return(-1);	/* errno set by statvfs */

			if (vfsbuf.f_flag & ST_NOTRUNC)
				return (1);
			else {
				errno = EINVAL;
				return (-1);
			}
		} 
		errno = ENOENT;
		return(-1); 
		
	case _PC_VDISABLE:
		return(_POSIX_VDISABLE);	/* defined in termios.h */

	case _PC_NAME_MAX:
		if (path != NULL) {
			if (statvfs(path, &vfsbuf)) 
				return(-1);	/* errno set by statvfs */

			return(vfsbuf.f_namemax);
		}
		errno = ENOENT;
		return(-1); 

	case _PC_CHOWN_RESTRICTED:
		{
		int ret;
			if ((ret = _sysconfig(_CONFIG_CHOWN_RST)) == 0) {
				errno = EINVAL;
				return(-1);
			} else
				return(ret);
		}
			


	}
}

fpathconf(fd, name)
int fd;
int name;
{
struct statvfs vfsbuf;
	
	if (name == _PC_NAME_MAX) {
		if (fstatvfs(fd, &vfsbuf)) 
			return(-1);	/* errno set by fstatvfs */
		return(vfsbuf.f_namemax);
	}
	
	if (name == _PC_NO_TRUNC) {
		if (fstatvfs(fd, &vfsbuf)) 
			return(-1);	/* errno set by statvfs */

		if (vfsbuf.f_flag & ST_NOTRUNC)
	
			return (1);
		else {
			errno = EINVAL;
			return (-1);
		}

	} 

	return(pathconf(NULL, name));
}
