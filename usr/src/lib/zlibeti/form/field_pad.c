/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sti:form/field_pad.c	1.1"

#include "utility.h"

	/******************
	*  set_field_pad  *
	******************/

int set_field_pad (f, pad)
FIELD * f;
int pad;
{
	if (! (isascii (pad) && isprint (pad)))
		return E_BAD_ARGUMENT;

	f = Field (f);

	if (Pad (f) != pad)
	{
		Pad (f) = pad;
		return _sync_attrs (f);
	}
	return E_OK;
}

int field_pad (f)
FIELD * f;
{
	return Pad (Field (f));
}

