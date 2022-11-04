/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sti:form/field_opts.c	1.2"

#include "utility.h"

	/*******************
	*  set_field_opts  *
	*******************/

int set_field_opts (f, opts)
FIELD * f;
OPTIONS opts;
{
	return _sync_opts (Field (f), opts);
}

OPTIONS field_opts (f)
FIELD * f;
{
	return Field (f) -> opts;
}

int field_opts_on (f, opts)
FIELD * f;
OPTIONS opts;
{
	FIELD * x = Field (f);
	return _sync_opts (x, x -> opts | opts);
}


int field_opts_off (f, opts)
FIELD * f;
OPTIONS opts;
{
	FIELD * x = Field (f);
	return _sync_opts (x, x -> opts & ~opts);
}
