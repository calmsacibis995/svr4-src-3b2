/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stitam:track.c	1.1"
#include "cvttam.h"
#include "track.h"

int
TAMtrack (w)
short w;
{
  if (w != TamWin2int(CurrentWin)) {
    return TERR_SYS;
  }
  return TAMwgetc (w);
}
