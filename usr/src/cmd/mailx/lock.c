/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:lock.c	1.5.2.1"
#ident "@(#)lock.c	1.3 'attmail mail(1) command'"
#ident	"@(#)mailx:lock.c	1.5.1.1"
/*
 * lock a file pointer
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>

lock(fp, mode, blk)
FILE	*fp;
char	*mode;
int	blk;
{
	struct flock	l;

	l.l_type = !strcmp(mode, "r") ? F_RDLCK : F_WRLCK;
	l.l_whence = 0;
	l.l_start = l.l_len = 0L;
	return fcntl(fileno(fp), blk ? F_SETLKW : F_SETLK, &l);
}
