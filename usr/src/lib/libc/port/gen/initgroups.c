/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/initgroups.c	1.4"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
/*
 * initgroups
 */
#ifdef __STDC__
	#pragma weak initgroups = _initgroups
#endif
#include "synonyms.h"
#include "shlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <grp.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

initgroups(uname, agroup)
	const char *uname;
	gid_t agroup;
{
	gid_t *groups;
	register struct group *grp;
	register int i;
	long ngroups_max;
	int ngroups = 0;
	int errsave, retsave;
 
	if ((ngroups_max = sysconf(_SC_NGROUPS_MAX)) <= 0)
		return ngroups_max;

	groups = (gid_t *)malloc(sizeof(gid_t) * ngroups_max);
	if (agroup >= 0)
		groups[ngroups++] = agroup;

	setgrent();

	while (grp = getgrent()) {
		if (grp->gr_gid == agroup)
			continue;
		for (i = 0; grp->gr_mem[i]; i++) {
			if (strcmp(grp->gr_mem[i], uname))
				continue;
			if (ngroups == ngroups_max)
				goto toomany;
			groups[ngroups++] = grp->gr_gid;
		}
	}

toomany:
	endgrent();

	retsave = setgroups(ngroups, groups);
	errsave = errno;

	free(groups);

	errno = errsave;
	return retsave;
}
