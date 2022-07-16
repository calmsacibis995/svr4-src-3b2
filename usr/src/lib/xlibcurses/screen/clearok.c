/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/clearok.c	1.6"
#include	"curses_inc.h"

clearok(win,bf)
WINDOW	*win;
bool	bf;
{
    win->_clear = bf;
    return (OK);
}
