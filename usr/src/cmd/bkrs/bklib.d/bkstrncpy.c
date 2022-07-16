/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:bklib.d/bkstrncpy.c	1.3"

#include <sys/types.h>
#include <string.h>

void
bkstrncpy( to, tosz, from, fromsz )
char *to, *from;
int tosz, fromsz;
{
	if( fromsz < tosz )
		(void) strcpy( to, from );
	else {
		(void) strncpy( to, from, tosz - 1 );
		to[ tosz - 1 ] = '\0';
	}
}

