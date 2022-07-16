/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/raise.c	1.4"
/*LINTLIBRARY*/
#include <sys/types.h>
#include "synonyms.h"
#include <signal.h>
#include <unistd.h>


int
raise(sig)
int sig;
{
	return( kill(getpid(), sig) );
}
