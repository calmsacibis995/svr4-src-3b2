/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:mail/doopen.c	1.4"
#ident "@(#)doopen.c	2.6 'attmail mail(1) command'"
#include "mail.h"
/*
	Generic open routine.
	Exits on error with passed error value.
	Returns file pointer on success.

	Note: This should be used only for critical files
	as it will terminate mail(1) on failure.
*/
FILE *
doopen(file, type, errnum)
char	*type, *file;
{
	static char pn[] = "doopen";
	FILE *fptr;

	if ((fptr = fopen(file, type)) == NULL) {
		fprintf(stderr,
			"%s: Can't open '%s' type: %s\n",program,file,type);
		error = errnum;
		Dout(pn, 0, "can't open '%s' type: %s\n",program,file,type);
		Dout(pn, 0, "error set to %d\n", error);
		done(0);
	}
	return(fptr);
}
