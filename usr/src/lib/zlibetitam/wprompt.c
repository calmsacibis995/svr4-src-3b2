/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stitam:wprompt.c	1.1"
#include "cvttam.h"
#include <string.h>

int
TAMwprompt (wn, c)
short wn;
char *c;
{
  TAMWIN *tw;

  if (tw = _validwindow (wn)) {
    if (Prompt(tw)) {
      free (Prompt(tw));
    }
    Prompt(tw) = NULL;
    if (c && c[0] != '\0') {
      if ((Prompt(tw) = malloc ((unsigned)(strlen (c)+1))) == NULL) {
	return (ERR);
      }
      (void)strcpy (Prompt(tw), c);
      _post (tw);
      (void)doupdate ();
    }
    return (OK);
  }
  return (ERR);
}
