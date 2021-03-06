/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)osadmin:csub/errbefore.c	1.1"
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	
	Routine called before error message has been printed.
	Command and library version.
*/

#include	"cmderr.h"


void
errbefore( severity, format, ErrArgList )
int	severity;
char	*format;
int	ErrArgList;
{
	switch( severity ) {
	case CHALT:
	case CERROR:
		errverb( "tag" );
		break;
	case CWARN:
	case CINFO:
		break;
	}
	return;
}
