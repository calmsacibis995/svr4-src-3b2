/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/fatal.c	1.7"

#include	<stdio.h>
#include	<string.h>
#include	<varargs.h>
#include	"defs.h"

#define	FNAMSIZ	14	/* Maximum file name size.	*/

static char FileName[FNAMSIZ+1];	/* Current file name.	*/
static char FNsave[FNAMSIZ+1];		/* Save area for file name.	*/


	void
fatalinit(file)			/* Initialize fatal message.	*/
char *file;			/* Pointer to file name.	*/

{
 char *nl;			/* Points to NEWLINE, if it exists.	*/

 (void) strncpy(FileName,file,sizeof(FileName));	/* Move it in.	*/
 FileName[FNAMSIZ] = EOS;			/* Be sure it stops.	*/
 if((nl = strchr(FileName,NEWLINE)) != NULL)	/* If line ends with newline, */
	*nl = EOS;				/* remove it.	*/
 return;
}

	char *
fatalsave()			/* saves current file name in 'file'. 	*/
{
 return(strncpy(FNsave,FileName,sizeof(FileName)));
}

/* VARARGS */
	void
fatal(va_alist)
va_dcl
{
	va_list args;
	char *fmt;
	extern void exit();
	va_start(args);
	(void) fflush(stdout);
	(void) fprintf(stderr, "Optimizing file %s: ",FileName);
	fmt=va_arg(args,char *);
	(void) vfprintf(stderr, fmt, args);
	(void) fflush(stderr);
	va_end(args);
	exit(2);
}
