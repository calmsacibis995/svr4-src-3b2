/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libelf:common/fill.c	1.2"


#ifdef __STDC__
	#pragma weak	elf_fill = _elf_fill
#endif


#include "syn.h"


extern int	_elf_byte;


void
elf_fill(fill)
	int	fill;
{
	_elf_byte = fill;
	return;
}
