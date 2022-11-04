/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sti:form/form_win.c	1.1"

#include "utility.h"

	/*****************
	*  set_form_win  *
	*****************/

int set_form_win (f, window)
FORM * f;
WINDOW * window;
{
	if (Status (f, POSTED))
		return E_POSTED;

	Form (f) -> win = window;
	return E_OK;
}

WINDOW * form_win (f)
FORM * f;
{
	return Form (f) -> win;
}

