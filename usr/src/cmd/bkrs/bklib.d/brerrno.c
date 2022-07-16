/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:bklib.d/brerrno.c	1.3"

#include	<errno.h>

extern char *sys_errlist[];
extern int sys_nerr;
extern int sprintf();

char *
brerrno( L_errno )
int L_errno;
{
	static char buffer[ 30 ];
	if( L_errno < sys_nerr ) return( sys_errlist[ L_errno ] );
	(void) sprintf( buffer, "Unknown errno %d", L_errno );
	return( buffer );
}

