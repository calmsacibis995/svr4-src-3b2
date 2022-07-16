/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)filemgmt:bin/sysfs.c	1.1"
#include <sys/fstyp.h>

extern int errno;

main()
{
	register int nfstyp = sysfs(GETNFSTYP);
	register int ifstyp;
	char fsname[FSTYPSZ];

	if (nfstyp < 1) {
		exit(errno);
	}
	for (ifstyp = 1; ifstyp <= nfstyp; ifstyp++) {
		if (sysfs(GETFSTYP, ifstyp, fsname) < 0) {
			exit(errno);
		}
		printf("%s\n", fsname );	
	}
	exit(0);
}
