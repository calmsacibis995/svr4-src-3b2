/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:libbrmeth.d/brgetvolume.c	1.4"

#include <bkrs.h>

extern int brtype;

extern int bkgetvolume();

int
brgetvolume( volume, override, automated, result )
char *volume, *result;
int override, automated;
{
	switch( brtype ) {
	case BACKUP_T:
		return( bkgetvolume( volume, override, automated, result ) );
		/*NOTREACHED*/
		break;
	
	case RESTORE_T:
		break;

	default:
		return( BRNOTINITIALIZED );
	}

	return( BRSUCCESS );
}
