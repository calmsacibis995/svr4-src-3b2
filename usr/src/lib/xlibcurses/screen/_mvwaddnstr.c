/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/_mvwaddnstr.c	1.1"

#define	NOMACROS

#include	"curses_inc.h"

mvwaddnstr(win, y, x, s, n)
WINDOW	*win;
int	y, x;
char	*s;
int	n;
{
    return (wmove(win, y, x)==ERR?ERR:waddnstr(win, s, n));
}
