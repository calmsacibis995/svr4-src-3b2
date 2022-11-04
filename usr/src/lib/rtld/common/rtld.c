/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:common/rtld.c	1.7"

/* main run-time linking routines 
 *
 * _rtld may be called either from _rt_setup, to get
 * the process going at startup, or from _dlopen, to
 * load a shared object into memory during execution.
 *
 * We read the interface structure: if we have been passed the path
 * name of some file, we load that file into memory and continue
 * further processing beginning with that file; else we begin processing
 * with the current end of the rt_private_map list.
 *
 * For each shared object listed in the first object's needed list,
 * we open that object and load it into memory.  We then run down
 * the list of loaded objects and perform any needed relocations on each.
 * Finally, we adjust all memory segment protections, invoke the _init
 * routine for any loaded object that has one, and return a pointer
 * to the map structure for the first object we loaded
 *
 * If the environment flag LD_TRACE_LOADED_OBJECTS is set, we load
 * all objects, as above, print out the path name of each, and then exit.
 * If LD_WARN is also set, we also perform relocations, printing out a
 * diagnostic for any unresolved symbol.
 */

#include <fcntl.h>
#include "rtinc.h"

static Elf32_Dyn lreturn[2];  /* structure used to return values to caller */


int _rtld(interface, rt_ret)
Elf32_Dyn *interface;
Elf32_Dyn **rt_ret;
{
	Elf32_Dyn *inttmp[DT_MAXNEGTAGS];
	Elf32_Dyn *lneed;
	register struct rt_private_map *lm, *first_loaded;
	struct rt_private_map *lm2, *lm3;
	char  *pname, *pathname, *name;
	void (*itmp)();
	register int i; 
	int fd;
	unsigned int mode;

	DPRINTF(LIST,(2, "rtld: _rtld(0x%x, 0x%x)\n",(unsigned long)interface, 
		(unsigned long)rt_ret));

	/* create array of pointers to the interface structures
	 * array[tag] points to interface with d_tag == tag
	 */
	for (i= 0; i < DT_MAXNEGTAGS; i++)
		inttmp[i] = (Elf32_Dyn *)0;
	while (interface->d_tag != DT_NULL)  {
		if (interface->d_tag > 0) {
			_rt_lasterr("ld.so: %s: internal interface error",_proc_name);
			return(1);
		}
		inttmp[-interface->d_tag] = interface++;
	}

	/* inform debuggers that we are adding to the rt_private_map */
	_r_debug.r_state = RT_ADD;
	_r_debug_state();

	/* open /dev/zero if not open, for reading anonymous memory */
	if (_devzero_fd == -1) {
		if ((_devzero_fd = _open(DEV_ZERO, O_RDONLY)) == -1) {
			_rt_lasterr("ld.so: %s: can't open %s",_proc_name,(CONST char *)DEV_ZERO);
			return(1);
		}
	}

	/* if interface contains pathname, map in that file */
	if (inttmp[- DT_FPATH]) {
		pname = (char *)(inttmp[- DT_FPATH]->d_un.d_ptr);

		/* null pointer an invalid file name */
		if (pname == (char *)0) {
			_rt_lasterr("ld.so: %s: internal interface error: null pathname specified",_proc_name);
			return(1);
		}

		/* determine if this file is already loaded */
		if ((lm = _so_loaded(pname)) == (struct rt_private_map *)0) {
		/* if loaded, just exit
		 * all neededs should be loaded as well and we 
		 * don't want to relocate 2x */

			/* file not already loaded 
			 * - is it the run-time linker?
			 * if so, add rtld to end of link map
			 */
			if (!_rtstrcmp(pname, NAME(_rtld_map))) {
				first_loaded = _rtld_map;
				PREV(_rtld_map) = (struct link_map *)_ld_tail;
				NEXT(_ld_tail) = (struct link_map *)_rtld_map;
				_ld_tail = _rtld_map;
			}
			else { /* not run-time linker - find and open file */
				if ((fd = _so_find(pname, &pathname)) == -1) {

					return(1);
				}

				/* map in object and add to end of map list */
				if ((first_loaded = _map_so(fd, pathname)) == (struct rt_private_map *)0) 
					return(1);
				NEXT(_ld_tail) = (struct link_map *)first_loaded;
				PREV(first_loaded) = (struct link_map *)_ld_tail;
				_ld_tail = first_loaded;
				/* ??? check again for rtld ??? */
			}
		}
		else  { /* file already loaded */
			/* set up return value  - 
			 * rt_private_map of loaded object
			 */
			lreturn[0].d_tag = DT_MAP;
			lreturn[0].d_un.d_ptr = (Elf32_Addr)lm;
			lreturn[1].d_tag = DT_NULL;
			*rt_ret = &(lreturn[0]);
			return(0);
		}
	}
	else
		first_loaded = _ld_tail;
	DENY(first_loaded) = 0;
	
	/* map in all shared objects needed; start with needed
	 * section of first_loaded and map all those in;
	 * then go through needed sections of all objects just mapped,
	 * etc. result is a breadth first ordering of all needed objects
	 */

	for (lm = first_loaded; lm; lm = (struct rt_private_map *)NEXT(lm)) {
		/* process each shared object on needed list */
		for (lneed = (Elf32_Dyn *)DYN(lm);
			lneed->d_tag != DT_NULL; lneed++) {
			if (lneed->d_tag != DT_NEEDED) {
				continue;
			}
			name = (char *)STRTAB(lm) + lneed->d_un.d_val;
			/* is file already loaded? */
			if ((lm3 = _so_loaded(name)) == (struct rt_private_map *)0) {
				/* file not already loaded 
				 * - is it the run-time linker?
				 * if so, add rtld to end of link map
				 */
				if (!_rtstrcmp(name,NAME(_rtld_map))) {
					NEXT(_ld_tail) = (struct link_map *)_rtld_map;
					PREV(_rtld_map) = (struct link_map *)_ld_tail;
					_ld_tail = _rtld_map;
				}
				else { /* not run-time linker 
					* find and open file
					*/
					if ((fd = _so_find(name, &pname)) == -1) 
						return(1);
	
					/* map in object and add to end of map list */
					if ((lm2 = _map_so(fd, pname)) == (struct rt_private_map *)0) 
						return(1);
					NEXT(_ld_tail) = (struct link_map *)lm2;
					PREV(lm2) = (struct link_map *)_ld_tail;
					_ld_tail = lm2;
				}
				DENY(_ld_tail) = 0;			
			} /* end file not loaded */
			else {
				DENY(lm3) = 0;
			}
		} /* end for needed */
	} 
	/* if tracing print out names of all objects just added - skip a.out 
	 * we get here only on initial startup - not from dlopen
	 */
	if (_rt_tracing) {
		for (lm = (struct rt_private_map *)NEXT(_ld_loaded); lm; lm = (struct rt_private_map *)NEXT(lm)) {
			_rtfprintf(1, "ld.so: %s: file loaded: %s\n",_proc_name,NAME(lm));
		}
		/* if _rt_warn not set, exit */
		if (!_rt_warn)
			_exit(0);
	}

	/* for each object just added (except ld.so), call relocate 
	 * to relocate symbols
	 */

	/* determine binding mode  - default is lazy binding */
	if (inttmp[- DT_MODE])
		mode = inttmp[- DT_MODE]->d_un.d_val;
	else 
		mode = RTLD_LAZY;

	for (lm = first_loaded; lm; lm = (struct rt_private_map *)NEXT(lm)) {
		if (lm != _rtld_map)
			if (!_relocate(lm, mode))
				if (!_rt_warn)
					return(1);
		
	}

	/* perform special copy type relocations */
	for ( ; _rt_copy_entries; _rt_copy_entries = _rt_copy_entries->r_next)
		_rt_memcpy(_rt_copy_entries->r_to, _rt_copy_entries->r_from, _rt_copy_entries->r_size);
		

	/* close /dev/zero */
	(void)_close(_devzero_fd);
	_devzero_fd = -1;

	/* if tracing, exit */
	if (_rt_tracing)
		_exit(0);

	/* set correct protections for each segment mapped */
	for (lm = first_loaded; lm; lm = (struct rt_private_map *)NEXT(lm)) {
		if (!TEXTREL(lm) || lm == _rtld_map)
			continue;
		if (!_set_protect(lm, 0))
			return(1);
	}
	
	/* invoke _init of each object loaded (except a.out) */
	for (lm = first_loaded; lm; lm = (struct rt_private_map *)NEXT(lm)) {
		if (NAME(lm)) {
			itmp =  INIT(lm);
			if (itmp)
				(*itmp)();
		}
	}
	/* tell debuggers that the rt_private_map is now in a consistent state */
	_r_debug.r_state = RT_CONSISTENT;
	_r_debug_state();

	/* set up return value  - pointer to first object loaded */
	lreturn[0].d_tag = DT_MAP;
	lreturn[0].d_un.d_ptr = (Elf32_Addr)first_loaded;
	lreturn[1].d_tag = DT_NULL;
	*rt_ret = &(lreturn[0]);
	
	return(0);
}

/* function that determines whether a file with name pathname
 * has already been loaded; if so, returns a pointer to its
 * link structure; else returns a NULL pointer
 */
struct rt_private_map *_so_loaded(pathname)
CONST char *pathname;
{
	register CONST char *p, *q;
	int slash = 0;
	register struct rt_private_map *lm;

	DPRINTF(LIST,(2, "rtld: _so_loaded(%s)\n",pathname));

	/* does pathname contain any '/'s ? */
	p = pathname;
	while (*p)
		if (*p++ == '/') {
			slash = 1;
			break;
		}
	/* if pathname had any '/'s, compare it as is
	 * with names stored in each rt_private_map;
	 * else compare it with last component of
	 * each rt_private_map name
	 */
	for (lm = (struct rt_private_map *)NEXT(_ld_loaded); lm; lm = (struct rt_private_map *)NEXT(lm)) {
		p = NAME(lm);
		if (!slash) {
		/* find first character past last '/' */
			q = p;
			while (*q)
				if (*q++ == '/')
					p = q;
		}
		if (!_rtstrcmp(p, pathname))
			return(lm);
	}
	return((struct rt_private_map *)0);
}


/* symbol lookup routine - takes symbol name, pointer to a
 * rt_private_map to search first, a list of rt_private_maps to search if that fails.
 * If successful, returns pointer to symbol table entry
 * and to rt_private_map of enclosing object. Else returns a null
 * pointer.
 */

Elf32_Sym *_lookup(name, first_lm, lm_list, lm)
CONST char *name;
struct rt_private_map *first_lm;
struct rt_private_map *lm_list;
struct rt_private_map **lm;
{
	register unsigned long hval = 0;
	unsigned long buckets;
	register unsigned long htmp, ndx;
	register struct rt_private_map *nlm;
	Elf32_Sym *sym;
	Elf32_Sym *symtabptr;
	char *strtabptr;
	unsigned long *hashtabptr;

	DPRINTF(LIST,(2, "rtld: _lookup(%s, 0x%x, 0x%x 0x%x)\n",name, 
		(unsigned long)first_lm,
		(unsigned long)lm_list, (unsigned long)lm));

	/* hash symbol name - use same hash function used by ELF access
	 * library 
	 * the form of the hash table is
	 * |--------------|
	 * | # of buckets |
	 * |--------------|
	 * | # of chains  |
	 * |--------------|
	 * | bucket[]	  |
	 * |   ...	  |
	 * |--------------|
	 * | chain[]	  |
	 * |   ...	  |
	 * |--------------|
	 */
	{
	register CONST char *p;
	register unsigned long g;

	p = name;
	hval = 0;
	while (*p) {
		hval = (hval << 4) + *p++;
		if ((g = (hval & 0xf0000000)) != 0)
			hval ^= g >> 24;
		hval &= ~g;
	}
	}

	if (first_lm) { /*  start search with specified rt_private_map */
		buckets = HASH(first_lm)[0];
		htmp = hval % buckets;
		/* get first symbol on hash chain */
		ndx = HASH(first_lm)[htmp + 2];

		hashtabptr = HASH(first_lm) + 2 + buckets;
		strtabptr = STRTAB(first_lm);
		symtabptr = SYMTAB(first_lm);

		while (ndx) {
			sym = symtabptr + ndx;
			if (_rtstrcmp(strtabptr + sym->st_name, name))  /* names do not match */
				ndx = hashtabptr[ndx];
			else if (sym->st_shndx != SHN_UNDEF) {
					*lm = first_lm;
					return(sym);
				}
			else /* local or undefined - fall
			      * through
			      */
				break; 
		} /* end while */
	} /* first_lm */

	/* go through each rt_private_map and look for symbol */
	for (nlm = lm_list; nlm; nlm = (struct rt_private_map *)NEXT(nlm)) {
		if (nlm != first_lm && !DENY(nlm)) {
			buckets = HASH(nlm)[0];
			htmp = hval % buckets;
			/* get first symbol on hash chain */
			ndx = HASH(nlm)[htmp + 2];

			hashtabptr = HASH(nlm) + 2 + buckets;
			strtabptr = STRTAB(nlm);
			symtabptr = SYMTAB(nlm);

			while (ndx) {
				sym = symtabptr + ndx;
				if (_rtstrcmp(strtabptr + sym->st_name, name))  /* names do not match */
					ndx = hashtabptr[ndx];
				else if (sym->st_shndx != SHN_UNDEF) {
					*lm = nlm;
					return(sym);
				}
				else /* local or undefined - fall
				      * through
				      */
					break; 
			} /* end while (ndx) */
		} /* end if !DENY(nlm) */
	} /* end for */
	/* if here -  no match */
	return((Elf32_Sym *)0);
}
