/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)osadmin:csub/errtext.c	1.1"
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	
	Routines to print and adjust options on error messages.
	Command and library version.
*/

#include	"cmderr.h"
#include	<stdio.h>
#include	<string.h>

int	errexit = 1;
static	char	*emsgs[] = {
		"",
		"WARNING:  ",
		"ERROR:  ",
		"HALT:  ",
		0
	};
char	**errmessages = emsgs;

static	char	*advice = 0;
static	int	bell = 0;
static	int	tag = 0;
static	int	tagnum = 0;
static	char	*tagstr = 0;
static	int	text = 1;
static	char	*tofix = 0;

char	*getenv();


void
errtext( severity, format, ErrArgList )
int	severity;
char	*format;
int	ErrArgList;
{
	errbefore( severity, format, ErrArgList );
	errverb( getenv( "ERRVERB" ) );
	if( bell )
		fputs( "\07", stderr );
	if( text ) {
		char	*s;

		if( s = getenv( "ERRPREFIX" ) )
			fputs( s, stderr );
		fputs( errmessages[ severity ], stderr );
	}
	if( (text  ||  tag)  &&  pgm_name )
		pgmname();
	if( text ) {
		if( (int)format == CERRNO )
			perror("");
		else {
			fprintf( stderr, format, ErrArgList );
			fputs( "\n", stderr );
		}
	}
	if( tag ) {
		if( tagstr )
			fprintf( stderr, "\t%s", tagstr );
		if( tagnum )
			fprintf( stderr, "%5d", tagnum );
		fputs( "\n", stderr );
	}
	if( (text  ||  tag)  &&  advice )
		fprintf( stderr, "\t%s\n", advice );
	if( (text  ||  tag)  &&  tofix )
		fprintf( stderr, "To Fix:\t%s\n", tofix );
	errafter( severity, format, ErrArgList );
	return;
}

void
erradvice( str )
char	*str;
{
	advice = str;
}

void
errtag( str, num )
char	*str;
int	num;
{
	tagstr = str;
	tagnum = num;
}

void
errtofix( str )
char	*str;
{
	tofix = str;
}


void
errverb( s )
register char	*s;
{
	char	buf[ BUFSIZ ];
	register
	 char	*token;
	static
	 char	space[] = ", \t\n";

	if( !s )
		return;
	strcpy( buf, s );
	token = strtok( buf, space );
	do {
		if(!strcmp(token, "bell"))
			bell = 1;
		else if(!strcmp(token, "nobell"))
			bell = 0;
		else if(!strcmp(token, "tag"))
			tag = 1;
		else if(!strcmp(token, "notag"))
			tag = 0;
		else if(!strcmp(token, "text"))
			text = 1;
		else if(!strcmp(token, "notext"))
			text = 0;
	} while( token = strtok( (char*)0, space ) );
}
