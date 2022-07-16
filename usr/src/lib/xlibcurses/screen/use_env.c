/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/use_env.c	1.3"

extern	char	_use_env;	/* in curses.c */

void
use_env(bf)
char	bf;
{
	_use_env = bf;
}
