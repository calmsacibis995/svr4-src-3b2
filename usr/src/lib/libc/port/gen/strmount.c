/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/strmount.c	1.1"
/*
 * Attach a file descriptor to a name in the file system by
 * mounting it onto the name space.
 */
#ifdef __STDC__
	#pragma weak strmount = _strmount
#endif
#include "synonyms.h"
#include <sys/errno.h>
#include <stdio.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/vnode.h>
#include <sys/fs/fifonode.h>

int
strmount(path, fildes)
	char *path;
	int fildes;
{
	struct fifodata fifodatap;
	char fsname[] = "FIFOFS";

	fifodatap.data_fd = fildes;
	return (fmount((char *)NULL, path, MS_DATA, fsname, 
			(char *)&fifodatap, 
			    sizeof(struct fifodata)));
}
	
