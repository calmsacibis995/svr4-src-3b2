/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sti:menu/itemopts.c	1.4"
#include "private.h"

int
set_item_opts (i, opt)
register ITEM *i;
register OPTIONS opt;
{
  if (i) {
    if (Iopt(i) != opt) {
      Iopt(i) = opt;
      if ((opt & O_SELECTABLE) == 0) {
	/* If the item is being deactivated then unselect it */
	if (Value(i)) {
	  Value(i) = FALSE;
	}
      }
      if (Imenu(i) && Posted(Imenu(i))) {
	_move_post_item (Imenu(i), i);
	_show(Imenu(i));
      }
    }
  }
  else {
    Iopt(Dfl_Item) = opt;
  }
  return E_OK;
}

int
item_opts_off (i, opt)
register ITEM *i;
register OPTIONS opt;
{
  return set_item_opts (i, (Iopt(i ? i : Dfl_Item)) & ~opt);
}

int
item_opts_on (i, opt)
register ITEM *i;
register OPTIONS opt;
{
  return set_item_opts (i, (Iopt(i ? i : Dfl_Item)) | opt);
}

int
item_opts (i)
register ITEM *i;
{
  return Iopt(i ? i : Dfl_Item);
}
