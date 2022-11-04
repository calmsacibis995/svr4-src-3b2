/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)libpkg:devtype.c	1.3"
#include <stdio.h>
#include <string.h>
#include <pkgdev.h>

extern char	*devattr();
extern void	logerr(),
		free();
extern int	isdir();

int
devtype(alias, devp)
char	*alias;
struct pkgdev *devp;
{
	devp->mntflg = 0;
	devp->name = alias;
	devp->norewind = devp->dirname = devp->mount = NULL;
	devp->fstyp = devp->cdevice = devp->bdevice = NULL;
	devp->rdonly = 0;

	/* see if alias represents an existing directory */
	if((alias[0] == '/') && !isdir(alias)) {
		devp->dirname = devp->name;
		return(0); /* directory */
	}

	/* see if alias represents a mountable device (e.g., a floppy) */
	if((devp->mount=devattr(alias, "mountpt")) && devp->mount[0]) {
		devp->bdevice = devattr(alias, "bdevice");
		if(!devp->bdevice || !devp->bdevice[0]) {
			if(devp->bdevice) {
				free(devp->bdevice);
				devp->bdevice = NULL;
			}
			return(-1);
		}
		devp->dirname = devp->mount;
		return(0);
	} else if(devp->mount) {
		free(devp->mount);
		devp->mount = NULL;
	}

	devp->cdevice = devattr(alias, "cdevice");
	if(!devp->cdevice || !devp->cdevice[0]) {
		if(devp->cdevice) {
			free(devp->cdevice);
			devp->cdevice = NULL;
		}
		/*
		 * if it is not a raw device, it must be a directory
		 */
		devp->dirname = devattr(alias, "pathname");
		if(!devp->dirname || !devp->dirname[0] || 
		    isdir(devp->dirname)) {
			if(devp->dirname) {
				free(devp->dirname);
				devp->dirname = NULL;
			}
			return(-1);
		}
		return(0); /* directory */
	}

	/* 
	 * device is not a directory and is not mountable
	 * so it must be able to support a datastream
	 */
	if((devp->norewind=devattr(alias,"norewind")) && devp->norewind[0]){
		devp->dirname = NULL;
		return(0); /* tempdir */
	} else if(devp->norewind) {
		free(devp->norewind);
		devp->norewind = NULL;
	}
	return(-1);
}
