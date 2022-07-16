/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/vidputs.c	1.10"
#include	"curses_inc.h"

vidputs(a,b)
chtype	a;
#ifdef __STDC__
int	(*b)(char);
#else
int	(*b)();
#endif
{
    vidupdate(a,cur_term->sgr_mode,b);
    return (OK);
}
