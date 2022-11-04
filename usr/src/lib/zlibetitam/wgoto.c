/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stitam:wgoto.c	1.1"
#include "cvttam.h"

int
TAMwgoto (wn, r, c)
short wn;
short r, c;
{
  TAMWIN *tw;

  if (tw = _validwindow (wn)) {
    (void)wmove (Scroll(tw), r, c);
    return (OK);
  }
  return (ERR);
}
