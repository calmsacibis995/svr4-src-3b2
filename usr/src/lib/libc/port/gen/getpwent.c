/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getpwent.c	1.18.2.1"
/*LINTLIBRARY*/
#ifdef __STDC__
	#pragma weak endpwent = _endpwent
	#pragma weak fgetpwent = _fgetpwent
	#pragma weak getpwent = _getpwent
	#pragma weak setpwent = _setpwent
#endif
#include "synonyms.h"
#include "shlib.h"
#include <sys/param.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <errno.h>

struct passwd	*_getpwbuf();

static const char *PASSWD = "/etc/passwd";
static FILE *pwf = NULL;
static char *line;
static struct passwd passwd;

void
setpwent()
{
	if(pwf == NULL)
		pwf = fopen(PASSWD, "r");
	else
		rewind(pwf);
}

void
endpwent()
{
	if(pwf != NULL) {
		(void) fclose(pwf);
		pwf = NULL;
	}
}

struct passwd *
getpwent()
{
	extern struct passwd *fgetpwent();

	if(pwf == NULL) {
		if((pwf = fopen(PASSWD, "r")) == NULL)
			return(NULL);
	}
	return (fgetpwent(pwf));
}

struct passwd *
fgetpwent(f)
FILE *f;
{
	if (line == 0 && (line = malloc(BUFSIZ + 1)) == 0)
		return NULL;
	if (fgets(line, BUFSIZ, f) == NULL)
		return NULL;
	return _getpwbuf(&passwd, line);
}
