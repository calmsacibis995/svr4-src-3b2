/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)osadmin:csub/Fstat.c	1.1"
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	
	fstat(2) with error checking.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "cmderr.h"


Fstat( fildes, buf )
int	fildes;
struct stat	*buf;
{
	int	i;

	if( (i = fstat( fildes, buf ) ) == -1 ) 
		cmderr(CERROR, CERRNO);

	return  i;
}
