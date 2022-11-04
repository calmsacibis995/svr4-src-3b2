/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stitam:wpostwait.c	1.1"
#include "cvttam.h"

int
TAMwpostwait ()
{
  reset_prog_mode ();
  (void)wclear (stdscr);
  (void)wnoutrefresh (stdscr);
  (void)TAMwrefresh (-1);
  return (OK);
}
