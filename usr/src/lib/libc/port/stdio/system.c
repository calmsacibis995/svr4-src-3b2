/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/system.c	1.18"
/*	3.0 SID #	1.4	*/
/*LINTLIBRARY*/
#include "synonyms.h"
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <osfcn.h>

#define BIN_SH "/bin/sh"

int
system(s)
const char *s;
{
	int	status, pid, w;
	void (*istat)(), (*qstat)(), (*cstat)();
	struct stat	buf;

	if (s == NULL) {
		if (stat(BIN_SH, &buf) != 0) {
			return(0);
		} else if (getuid() == buf.st_uid) {
			if ((buf.st_mode & 0100) == 0)
				return(0);
		} else if (getgid() == buf.st_gid) {
			if ((buf.st_mode & 0010) == 0)
				return(0);
		} else if ((buf.st_mode & 0001) == 0) {
			return(0);
		}
		return(1);
	}
		
	if((pid = fork()) == 0) {
		(void) execl(BIN_SH, "sh", (const char *)"-c", s, (char *)0);
		_exit(127);
	}
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	cstat = signal(SIGCLD, SIG_DFL);
	while((w = wait(&status)) != pid && w != -1)
		;
	(void) signal(SIGINT, istat);
	(void) signal(SIGQUIT, qstat);
	(void) signal(SIGCLD, cstat);
	return((w == -1)? w: status);
}
