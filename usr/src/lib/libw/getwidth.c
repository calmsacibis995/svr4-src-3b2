/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libw:getwidth.c	1.3"
#include "libw.h"
#include <ctype.h>
#define CSWIDTH	514

void getwidth(eucstruct)
eucwidth_t *eucstruct;
{
	unsigned char *cswidth = &__ctype[CSWIDTH];
	eucstruct->_eucw1 = cswidth[0];
	eucstruct->_eucw2 = cswidth[1];
	eucstruct->_eucw3 = cswidth[2];
	eucstruct->_multibyte = (cswidth[6] > 1);
	if (cswidth[0] > sizeof(unsigned short) 
	|| cswidth[1] > sizeof(unsigned short) 
	|| cswidth[2] > sizeof(unsigned short))
		eucstruct->_pcw = sizeof(unsigned long);
	else
		eucstruct->_pcw = sizeof(unsigned short);
	eucstruct->_scrw1 = cswidth[3];
	eucstruct->_scrw2 = cswidth[4];
	eucstruct->_scrw3 = cswidth[5];
}
