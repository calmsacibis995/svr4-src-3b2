/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libelf:common/kind.c	1.4"


#ifdef __STDC__
	#pragma weak	elf_kind = _elf_kind
#endif


#include "syn.h"
#include "libelf.h"
#include "decl.h"


Elf_Kind
elf_kind(elf)
	Elf	*elf;
{
	if (elf == 0)
		return ELF_K_NONE;
	return elf->ed_kind;
}
