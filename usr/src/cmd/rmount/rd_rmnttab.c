/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
	rd_mnttab - read the rmount table
		a block is allocated that is large enough to hold
		the table plus room for one more entry.
	return:
		0 - stat and open success
		1 - rmnttab not existing
		2 - stat or open error
*/

#ident	"@(#)rmount:rd_rmnttab.c	1.1.2.1"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <sys/mnttab.h>

#define RMNTTAB	"/etc/rfs/rmnttab"

extern char *cmd;
extern FILE *rp;
extern struct stat stbuf;

int
rd_rmnttab()
{

	if (stat(RMNTTAB, &stbuf) < 0) {
		if (errno == ENOENT)	 	/* rmnttab does not exist */
			return 1;		/* pretend it's empty */
		else {
			fprintf(stderr, "%s: cannot stat %s\n", cmd, RMNTTAB);
			return 2;
		}
	}
	else if ((rp = fopen(RMNTTAB, "r")) == NULL) {
		fprintf(stderr, "%s: cannot open %s\n", cmd, RMNTTAB);
		return 2;
	}
	return 0;
}
