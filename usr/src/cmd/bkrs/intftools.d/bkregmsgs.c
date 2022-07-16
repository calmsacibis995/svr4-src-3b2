/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:intftools.d/bkregmsgs.c	1.2"

char *errmsgs[] = {
	"Option \"%c\" is invalid.\n",
	"Tag %s does not exist in table %s (return code %d).\n",
	"Could not allocate memory for a table entry.\n",
	"Could not read table entry number %d.\n",
	"Could not open temporary file %s.\n",
	"Cannot open table %s (%d).\n",
};
int	lasterrmsg = sizeof( errmsgs )/sizeof( char * );
