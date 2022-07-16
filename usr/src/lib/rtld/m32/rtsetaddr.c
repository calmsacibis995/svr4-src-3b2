/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:m32/rtsetaddr.c	1.1"
#include "rtinc.h"

/* look up values of several special symbols -
 * if no defintion found, do nothing, else
 * set GOT entry for that symbol
 * -symbols all have definitions in ld.so - definitions
 * from the a.out or other libraries may override
 */
void _rt_setaddr()
{
	Elf32_Sym *sym;
	struct link_map *lm;
	unsigned long l;

	sym = _lookup("errno", LO_ALL, _ld_loaded, &lm);
	if (sym)
		l = sym->st_value + (NAME(lm) ? ADDR(lm) : 0);
	sym = _lookup("_fp_hw", LO_ALL, _ld_loaded, &lm);
	if (sym)
		l = sym->st_value + (NAME(lm) ? ADDR(lm) : 0);
	sym = _lookup("_asr", LO_ALL, _ld_loaded, &lm);
	if (sym)
		l = sym->st_value + (NAME(lm) ? ADDR(lm) : 0);
	sym = _lookup("_end", LO_SINGLE, _ld_loaded, &lm);
	if (sym)
		_nd = sym->st_value;
}
