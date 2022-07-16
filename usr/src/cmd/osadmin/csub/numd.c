/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)osadmin:csub/numd.c	1.1"
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	
	true if string is numeric, with or without decimal point
*/

#include	<ctype.h>


numd( string )
register char *string;
{
	register int	seen_a_digit = 0;

	if( !string  ||  !*string )
		return  0;	/* null pointer or null string */
	while( *string != '.' && *string != '\0' )
		if( isdigit( *(string++) ) )
			seen_a_digit = 1;
		else
			return  0;
	if( *string == '.' ) {	/* have found decimal */
		string++;	/* go to char. after decimal */
		while( *string )
			if( isdigit( *(string++) ) )
				seen_a_digit = 1;
			else
				return  0;
	}
	return  seen_a_digit;
}
