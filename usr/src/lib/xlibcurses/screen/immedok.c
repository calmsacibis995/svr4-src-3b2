/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/immedok.c	1.4"
#include	"curses_inc.h"

extern	int	_quick_echo();

void
immedok(win, bf)
WINDOW	*win;
bool	bf;
{
    if (bf)
    {
	win->_immed = TRUE;
	_quick_ptr = _quick_echo;
    }
    else
	win->_immed = FALSE;
}
