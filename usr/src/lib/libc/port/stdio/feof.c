/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/feof.c	1.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>

#undef feof

int
feof(iop)
	FILE *iop;
{
	return iop->_flag & _IOEOF;
}
