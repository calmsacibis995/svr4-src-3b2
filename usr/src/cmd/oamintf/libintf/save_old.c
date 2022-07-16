/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
#ident	"@(#)oamintf:libintf/save_old.c	1.2"
#include <stdio.h>
#include <string.h>
#include "intf.h"

extern char *getenv();

void
save_old(string, savfile)
char *string;		/* original string to save */
FILE *savfile;		/* file ptr that says where to save */
{
	(void) fputs(SAVHDR, savfile);
	(void) fputs(string, savfile);
}
