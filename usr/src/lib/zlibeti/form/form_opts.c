/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sti:form/form_opts.c	1.2"

#include "utility.h"

	/******************
	*  set_form_opts  *
	******************/

int set_form_opts (f, opts)
FORM * f;
OPTIONS opts;
{
	Form (f) -> opts = opts;
	return E_OK;
}

OPTIONS form_opts (f)
FORM * f;
{
	return Form (f) -> opts;
}

int form_opts_on (f, opts)
FORM * f;
OPTIONS opts;
{
	Form (f) -> opts |= opts;
	return E_OK;
}

int form_opts_off (f, opts)
FORM * f;
OPTIONS opts;
{
	Form (f) -> opts &= ~opts;
	return E_OK;
}
