/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)curses:screen/_slk_init.c	1.1"

#define		NOMACROS
#include	"curses_inc.h"

slk_init(f)
int	f;
{
    return (slk_start((f == 0) ? 3 : 2, NULL));
}
