/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)osadmin:csub/fopenck.c	1.1"
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	
	Perform an fopen(3) and report and exit on failure.
*/

#include	<stdio.h>


FILE *
fopenck( name, type )
register char	*name;
register char	*type;
{
	FILE		*fopen();
	register FILE	*fp;

	if( (fp = fopen( name, type )) == NULL ) {
		pgmname();
		perror( name );
		exit(1);
	}
	return  fp;
}
