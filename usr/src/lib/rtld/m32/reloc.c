/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:m32/reloc.c	1.10"

/* 3B2 specific routines for performing relocations */

#include "rtinc.h"
#include <sys/elf_M32.h>
#include <sgs.h>

/* read and process the relocations for one link object 
 * we assume all relocation sections for loadable segments are
 * stored contiguously in the file
 *
 * accept a pointer to the rt_private_map structure for a given object and
 * the binding mode: if RTLD_LAZY, then don't relocate procedure
 * linkage table entries; if RTLD_NOW, do.
 */

static int do_reloc ARGS((struct rt_private_map *, Elf32_Rel *, unsigned long));


int _relocate(lm, mode)
struct rt_private_map *lm;
int mode;
{
	register unsigned long *got_addr;
	register unsigned long base_addr;
	register Elf32_Rel *rel;
	register Elf32_Rel *rend;

	DPRINTF((LIST|DRELOC),(2, "rtld: _relocate(%s, %d)\n",
		(CONST char *)((NAME(lm) ? NAME(lm) : "a.out")), mode));
	
	/* if lazy binding, initialize first procedure linkage
	 * table entry to go to _rtbinder
	 */
	if (PLTGOT(lm)) {
		if (mode == RTLD_LAZY) {
			/* fix up the first few GOT entries
			 *	GOT[GOT_XLINKMAP] = the address of the link map
			 *	GOT[GOT_XRTLD] = the address of rtbinder
			 */
			got_addr = PLTGOT(lm) + GOT_XLINKMAP;
			*got_addr = (unsigned long)lm;
			got_addr = PLTGOT(lm) + GOT_XRTLD;
			*got_addr = (unsigned long)_rtbinder;

			/* if this is a shared object, we have to step
			 * through the plt entries and add the base address
			 * to the corresponding got entry
			 */
			if (NAME(lm)) {
				base_addr = ADDR(lm);
				rel = JMPREL(lm);
				rend = (Elf32_Rel *)((unsigned long)rel + PLTRELSZ(lm));

				for ( ; rel < rend; ++rel) {
					got_addr = (unsigned long *)((char *)rel->r_offset + base_addr);	
					*got_addr += base_addr;
				}
			}
		}
		else
		{
			if (do_reloc(lm, JMPREL(lm), PLTRELSZ(lm)) == 0)
				return 0;
		}
	}

	if (RELSZ(lm) && REL(lm))
		return do_reloc(lm, REL(lm), RELSZ(lm));
	return 1;
}


static int
do_reloc(lm, reladdr, relsz)
	struct rt_private_map *lm;
	register Elf32_Rel *reladdr;
	unsigned long relsz;
{
	unsigned long baseaddr, stndx;
	register unsigned long off;
	register unsigned int rtype;
	register Elf32_Rel *rend;
	long value;
	Elf32_Sym *symref, *symdef;
	char *name;
	struct rt_private_map *def_lm, *first_lm, *list_lm;
	union {
		unsigned long l;
		char c[4];
	} symval;

	DPRINTF((LIST|DRELOC),(2, "rtld: do_reloc(%s, 0x%x, 0x%x)\n",
		(CONST char *)(NAME(lm) ? NAME(lm) : "a.out"), reladdr, relsz));
	
	baseaddr = ADDR(lm);
	rend = (Elf32_Rel *)((unsigned long)reladdr + relsz);

	/* loop through relocations */
	for ( ; reladdr < rend; ++reladdr) { 
		rtype = ELF32_R_TYPE(reladdr->r_info);
		off = (unsigned long)reladdr->r_offset;
		stndx = ELF32_R_SYM(reladdr->r_info);

		if (rtype == R_M32_NONE)
			continue;

		/* if not a.out, add base address to offset */
		if (NAME(lm))
			off += baseaddr;


		/* if R_M32_RELATIVE, simply add base addr 
		 * to reloc location 
		 */

		if (rtype == R_M32_RELATIVE || rtype == R_M32_RELATIVE_S)
			value = baseaddr;

		/* get symbol table entry - if symbol is local
		 * value is base address of this object
		 */
		 else {
			symref = (Elf32_Sym *)((unsigned long)SYMTAB(lm) + (stndx * SYMENT(lm)));
	
			/* if local symbol, just add base address 
			 * we should have no local relocations in the
			 * a.out
			 */
			if (ELF32_ST_BIND(symref->st_info) == STB_LOCAL) {
				value = baseaddr;
			}
			else {	/* global or weak 
				 * lookup symbol definition - error 
				 * if name not found and reference was 
				 * not to a weak symbol - weak 
				 * references may be unresolved
				 */
		
				name = (char *)(STRTAB(lm) + symref->st_name);

			DPRINTF(DRELOC,(2, "rtld: relocating %s\n",name));

				first_lm = 0;
				if (rtype == R_M32_COPY) {
					/* don't look in the a.out */
					list_lm = (struct rt_private_map *)NEXT(_ld_loaded);
				} else {
					list_lm = _ld_loaded;
					/* look in the current object first */
					if (SYMBOLIC(lm))
						first_lm = lm;
				}
					
				if (((symdef = _lookup(name, first_lm, list_lm, &def_lm))
					== (Elf32_Sym *)0)
					&& (ELF32_ST_BIND(symref->st_info)
					!= STB_WEAK)) {
					if (_rt_warn) {
						_rtfprintf(2, "ld.so: %s: relocation error: symbol not found: %s\n", _proc_name,name);
						continue;
					}
					else {
						_rt_lasterr("ld.so: %s: relocation error: symbol not found: %s", _proc_name, name);
						return(0);
					}
				}
				else { /* symbol found  - relocate */
					if (symdef == (Elf32_Sym *)0)
						/* undefined weak global */
						continue;  
					/* calculate location of definition 
					 * - symbol value plus base address of
					 * containing shared object
					 */
					value = symdef->st_value;
					if (NAME(def_lm) && 
						(symdef->st_shndx != SHN_ABS))
						value += ADDR(def_lm);
		
		
					/* for R_M32_COPY, just make an entry 
					 * in the rt_copy_entries array
					 */
					if (rtype == R_M32_COPY) {
						struct rel_copy *rtmp;
		
						if ((rtmp = (struct rel_copy *) _rtmalloc(sizeof(struct rel_copy))) == 0) {
							if (!_rt_warn)
								return(0);
							else continue;
						}
						else {
							rtmp->r_to = (char *)off;
							rtmp->r_size = symdef->st_size;
							rtmp->r_from = (char *) value;
							if (!_rt_copy_entries) {
								/* 1st entry */
								_rt_copy_entries = rtmp;
								_rt_copy_last = rtmp;
							}
							else {
								_rt_copy_last-> r_next = rtmp;
								_rt_copy_last = rtmp;
							}
						}
						continue;
					} /* end R_M32_COPY */
					
					/* calculate final value - 
					 * if PC-relative, subtract ref addr
					 */
					if (PCRELATIVE(rtype))
						value -= off;
					
				} /* end else symbol found */
			} /* end global or weak */
		} /* end not R_M32_RELATIVE */
		/* insert value calculated at reference point
		 * 3 cases - normal byte order aligned, normal byte
		 * order unaligned, and byte swapped
		 * for the swapped and unaligned cases we insert value 
		 * a byte at a time
		 */
		symval.l = value;
		switch(rtype) {
		case R_M32_GLOB_DAT:  /* word aligned */
			DPRINTF(DRELOC,(2,"rtld: sym value is 0x%x, offset is 0x%x\n",value, off));
			value += *(unsigned long *)off;
			*(unsigned long *)off = value;
			break;
		case R_M32_32:	/* unaligned - normal byte order */
		case R_M32_RELATIVE:
			symval.c[0] = ((char *)off)[0];
			symval.c[1] = ((char *)off)[1];
			symval.c[2] = ((char *)off)[2];
			symval.c[3] = ((char *)off)[3];
			symval.l += value;
			((char *)off)[0] = symval.c[0];
			((char *)off)[1] = symval.c[1];
			((char *)off)[2] = symval.c[2];
			((char *)off)[3] = symval.c[3];
			break;
		case R_M32_32_S:	/* byte swapped */
		case R_M32_PC32_S:
		case R_M32_RELATIVE_S: 
			symval.c[0] = ((char *)off)[3];
			symval.c[1] = ((char *)off)[2];
			symval.c[2] = ((char *)off)[1];
			symval.c[3] = ((char *)off)[0];
			symval.l += value;
			((char *)off)[3] = symval.c[0];
			((char *)off)[2] = symval.c[1];
			((char *)off)[1] = symval.c[2];
			((char *)off)[0] = symval.c[3];
			break;
		case R_M32_JMP_SLOT: 
			/* for plt-got do not add ref contents */
			((char *)off)[0] = symval.c[0];
			((char *)off)[1] = symval.c[1];
			((char *)off)[2] = symval.c[2];
			((char *)off)[3] = symval.c[3];
			break;
		default:
			if (_rt_warn)
				_rtfprintf(2, "ld.so: %s: invalid relocation type %d at 0x%x\n",_proc_name,rtype,off);
			else {
				_rt_lasterr("ld.so: %s: invalid relocation type %d at 0x%x",_proc_name,rtype,off);
				return(0);
			}
			break;
		}

	} /* end of while loop */
	return(1);
}

