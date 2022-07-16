/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)osadmin:csub/Write.c	1.1"
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	
	write(2) with error checking
*/

#include	<cmderr.h>
#include	<stdio.h>


Write( fd, buf, count )
int	fd;	/* file descriptor */
char	*buf;
int	count;
{
	register int	countout;

	if( (countout = write( fd, buf, count )) != count ) {
		cmderr( CERROR, "file descriptor %d, tried %d, wrote %d",
			fd, count, countout );
	}
}
