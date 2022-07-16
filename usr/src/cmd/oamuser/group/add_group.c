/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:group/add_group.c	1.3"

#include	<sys/types.h>
#include	<stdio.h>
#include	<unistd.h>
#include	<userdefs.h>

extern int lockf();

int
add_group(group, gid)
char *group;	/* name of group to add */
gid_t gid;		/* gid of group to add */
{
	FILE *etcgrp;		/* /etc/group file */

	if( (etcgrp = fopen( GROUP, "a" )) == NULL
		|| lockf( fileno(etcgrp), F_LOCK, 0 ) != 0 ) {
		return( EX_UPDATE );
	}

	(void) fprintf( etcgrp, "%s::%ld:\n", group, gid );

	(void) lockf( fileno(etcgrp), F_ULOCK, 0 );

	(void) fclose(etcgrp);

	return( EX_SUCCESS );
}
