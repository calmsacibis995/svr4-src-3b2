/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:intftools.d/validdays.c	1.3"

#define TRUE	1
#define FALSE	0

main( argc, argv )
int argc;
char *argv[];
{
	unsigned char *ptr;
	unsigned char *p_dayrange();

	int ok = FALSE;
	int begin;
	int end;

	void exit();

	if( argc != 2 )
		exit( 1 );

	ptr = (unsigned char *) argv[1];
	while( ptr = p_dayrange( ptr, &begin, &end ) ) {
		if( !*ptr ) {
			ok = TRUE;
			break;
		}
		ptr++;
	}

	exit( !ok );
}
