/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stitam:wgetsel.c	1.2"
#include "cvttam.h"

int
TAMwgetsel()
{
  if (CurrentWin) {
    return (TamWin2int(CurrentWin));
  }
  return (ERR);
}
