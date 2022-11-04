/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stitam:werase.c	1.1"
#include "cvttam.h"

int
TAMwerase (wn)
short wn;
{
  TAMWIN *tw;

  if (tw = _validwindow (wn)) {
    (void)werase (Scroll(tw));
    return (OK);
  }
  return (ERR);
}
