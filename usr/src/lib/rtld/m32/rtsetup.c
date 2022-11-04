/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:m32/rtsetup.c	1.12"

/* 3B2 specific setup routine - 
 * relocate ld.so's symbols, setup its environment,
 * map in loadable sections of a.out;
 *
 * takes base address ld.so was loaded at, address of ld.so's
 * dynamic structure, address of process environment pointers,
 * address of auxiliary vector and
 * argv[0] (process name);
 * if errors occur, send process SIGKILL - otherwise
 * return a.out's entry point to the bootstrap routine
 */

#include <signal.h>
#ifdef __STDC__
#include <stdlib.h>
#else
extern int atexit();
#endif
#include <fcntl.h>
#include <sys/auxv.h>
#include "rtinc.h"
#include <sys/elf_M32.h>
#include <unistd.h>

static struct rt_private_map ld_map;  /* link map for ld.so */

unsigned long _rt_setup(ld_base, ld_dyn, pname, envp, auxv)
unsigned long ld_base;
Elf32_Dyn *ld_dyn;
char *pname, **envp;
auxv_t *auxv;
{
	Elf32_Dyn *dyn[DT_MAXPOSTAGS], *ltmp;
	Elf32_Dyn interface[2];
	Elf32_Sym *sym;
	register unsigned long off, reladdr, rend;
	CONST char *envdirs = 0, *rundirs = 0;
	int i, bmode = RTLD_LAZY;
	struct rt_private_map *lm;
	struct rt_private_map *save_lm;
	char *p;
	union {
		unsigned long l;
		char c[4];
	} symval;
	Elf32_Phdr *phdr;
	int phsize;
	int phnum;
	unsigned long entry;
	int flags;
	int pagsz;

	int fd = -1;

	/* search the aux. vector for the information passed by exec */
	for (; auxv->a_type != AT_NULL; auxv++) {
		switch(auxv->a_type) {

		case AT_EXECFD:
			/* this is the old exec that passes a file descriptor */
			fd = auxv->a_un.a_val;
			break;

		case AT_FLAGS:
			/* processor flags (MAU available, etc) */
			flags = auxv->a_un.a_val;
			break;

		case AT_PAGESZ:
			/* system page size */
			pagsz = auxv->a_un.a_val;
			break;

		case AT_PHDR:
			/* address of the segment table */
			phdr = (Elf32_Phdr *) auxv->a_un.a_ptr;
			break;

		case AT_PHENT:
			/* size of each segment header */
			phsize = auxv->a_un.a_val;
			break;

		case AT_PHNUM:
			/* number of program headers */
			phnum = auxv->a_un.a_val;
			break;

		case AT_BASE:
			/* ld.so base address */
			ld_base = auxv->a_un.a_val;
			break;

		case AT_ENTRY:
			/* entry point for the a.out */
			entry = auxv->a_un.a_val;
			break;

		}
	}

	/* store pointers to each item in ld.so's dynamic structure 
	 * dyn[tag] points to the dynamic section entry with d_tag
	 * == tag
	 */
	for (i = 0; i < DT_MAXPOSTAGS; i++)
		dyn[i] = (Elf32_Dyn *)0;
	ld_dyn = (Elf32_Dyn *)((char *)ld_dyn + ld_base);
	while (ld_dyn->d_tag != DT_NULL) 
		dyn[ld_dyn->d_tag] = ld_dyn++;
	
	/* relocate all symbols in ld.so */
	reladdr = ((unsigned long)(dyn[DT_REL]->d_un.d_ptr) + ld_base);

	rend = reladdr + dyn[DT_RELSZ]->d_un.d_val;
	for ( ; reladdr < rend; reladdr += sizeof(Elf32_Rel)) {
		off = (unsigned long)((Elf32_Rel *)reladdr)->r_offset + ld_base;
		/* insert value calculated at reference point
		 * we should only have 1 kind of relocation
		 * in ld.so because of the way it was linked:
		 * R_M32_RELATIVE - we simply add the base address
		 */
		if (ELF32_R_TYPE(((Elf32_Rel *)reladdr)->r_info) 
			!= R_M32_RELATIVE) {
			_rtfprintf(2, "ld.so: internal error: invalid relocation type %d at 0x%x\n",ELF32_R_TYPE(((Elf32_Rel *)reladdr)->r_info),off);
			(void)_kill(_getpid(), SIGKILL);
		}
		symval.c[0] = ((char *)off)[0];
		symval.c[1] = ((char *)off)[1];
		symval.c[2] = ((char *)off)[2];
		symval.c[3] = ((char *)off)[3];
		symval.l += ld_base;
		((char *)off)[0] = symval.c[0];
		((char *)off)[1] = symval.c[1];
		((char *)off)[2] = symval.c[2];
		((char *)off)[3] = symval.c[3];
	}

	/* set global to process name for error messages */
	_proc_name = pname;

	/* look for environment strings */
	envdirs = _readenv( envp, &bmode );

	DPRINTF(LIST,(2, "rtld: _rt_setup(0x%x, 0x%x, %s, 0x%x, 0x%x)\n",ld_base,(unsigned long)ld_dyn,pname,(unsigned long)envp, auxv));

	/* open /dev/zero to use for mapping anonymous memory */
	if ((_devzero_fd = _open(DEV_ZERO, O_RDONLY)) == -1) {
		_rtfprintf(2, "ld.so: %s: can't open %s\n",_proc_name,(CONST char *)DEV_ZERO);
		(void)_kill(_getpid(), SIGKILL);
	}

	/* map in the file, if exec has not already done so.
	 * If it has, just create a new link map structure for the a.out
	 */
	if (fd != -1) {
		/* this is the old exec that doesn't pass as much
		 * information on the stack, so we have to go
		 * through system calls to get it
		 */

		/* set system page size */
		_syspagsz = _sysconf(_SC_PAGESIZE);

		/* set processor specific flags */
		(void)_sys3b(S3BFPHW, &_flags);

		if ((_ld_loaded = _map_so(fd, 0)) == 0) {
			_rtfprintf(2, "%s\n",_dlerror());
			(void)_kill(_getpid(), SIGKILL);
		}
	}
	else {
		Elf32_Phdr *pptr;
		Elf32_Phdr *firstptr = 0;
		Elf32_Phdr *lastptr;
		Elf32_Dyn *mld;

		_flags = flags;
		_syspagsz = pagsz;

		/* extract the needed information from the segment headers */
		for (i = 0, pptr = phdr; i < phnum; i++) {
			if (pptr->p_type == PT_LOAD) {
				if (!firstptr)
					firstptr = pptr;
				lastptr = pptr;
			}
			else if (pptr->p_type == PT_DYNAMIC)
				mld = (Elf32_Dyn *)(pptr->p_vaddr);
			pptr = (Elf32_Phdr *)((unsigned long)pptr + phsize);
		}
		if ((_ld_loaded = _new_lm(0, mld, firstptr->p_vaddr,
			(lastptr->p_vaddr + lastptr->p_memsz) - firstptr->p_vaddr,
			entry, phdr, phnum, phsize)) == 0) {
				_rtfprintf(2, "%s\n",_dlerror());
				(void)_kill(_getpid(), SIGKILL);
		}
		if (TEXTREL(_ld_loaded))
			if (_set_protect( _ld_loaded, PROT_WRITE ) == 0) {
				_rtfprintf(2, "%s\n",_dlerror());
				(void)_kill(_getpid(), SIGKILL);
			}
	}

	/* _ld_loaded and _ld_tail point to head and tail of rt_private_map list
	 */
	_ld_tail = _ld_loaded;

	/* initialize debugger information structure 
	 * some parts of this structure were initialized
	 * statically
	 */
	_r_debug.r_map = (struct link_map *)_ld_loaded;
	_r_debug.r_ldbase = ld_base;

	/* create a rt_private_map structure for ld.so */
	_rtld_map = &ld_map;
	DYN(_rtld_map) = ld_dyn;
	ADDR(_rtld_map) = ld_base;
	SYMTAB(_rtld_map) = (VOID *)((unsigned long)dyn[DT_SYMTAB]->d_un.d_ptr
		+ ld_base);
	STRTAB(_rtld_map) = (VOID *)((unsigned long)dyn[DT_STRTAB]->d_un.d_ptr 
		+ ld_base);
	HASH(_rtld_map) = (unsigned long *)(dyn[DT_HASH]->d_un.d_ptr 
		+ ld_base);

	/* we copy the name here rather than just setting a pointer
	 * to it so that it will appear in the data segment and
	 * thus in any core file
	 */
	p = (char *)STRTAB(_rtld_map) + dyn[DT_SONAME]->d_un.d_val;
	if ((NAME(_rtld_map) = _rtmalloc(_rtstrlen(p) + 1)) == 0) {
		_rtfprintf(2, "%s\n",_dlerror());
		(void)_kill(_getpid(), SIGKILL);
	}
	(void)_rtstrcpy(NAME(_rtld_map), p);
	SYMENT(_rtld_map) = dyn[DT_SYMENT]->d_un.d_val;
	PERMIT(_rtld_map) = 1;
	NODELETE(_rtld_map) = 1;


	/* are any directories specified in the a.out's dynamic? */
	rundirs = RPATH(_ld_loaded);

	/* set up directory search path */
	if (!_rt_setpath(envdirs, rundirs)) {
		_rtfprintf(2, "%s\n",_dlerror());
		(void)_kill(_getpid(), SIGKILL);
	}

 	/* _nd is used by sbrk  - set its value to
	 * the a.out's notion of the program break
	 */
 	sym = _lookup("_end", _ld_loaded, 0, &lm);
	if (sym)
		_nd = sym->st_value;

	/* setup for call to _rtld */
	interface[0].d_tag = DT_MODE;
	interface[0].d_un.d_val = bmode;
	interface[1].d_tag = DT_NULL;

	if (_rtld(interface, &ltmp)) {
		_rtfprintf(2, "%s\n",_dlerror());
		(void)_kill(_getpid(), SIGKILL);
	}

	save_lm = 0;
	/* set refpermit bit of all objects mapped */
	for (lm = _ld_loaded; lm; lm = (struct rt_private_map *)NEXT(lm)) {
		PERMIT(lm) = 1;
		if (NEXT(lm) == (struct link_map *)_rtld_map)
			save_lm = lm;
	}

	/* unlink the link map for rtld, so it isn't searched unnecessaily
	 * At this point, _rtld_map should be on the end of the list
	 * (see _rtld())
	 */
	if (save_lm)
		NEXT(save_lm) = 0;

	/* set values of various symbols */
	_rt_setaddr();

	/* reconnect the list */
	if (save_lm)
		NEXT(save_lm) = (struct link_map *)_rtld_map;

	/* set up to invoke _fini routines on exit */
	(void)atexit(_rt_do_exit);

	return(ENTRY(_ld_loaded));
}


/* verify machine specific flags in ELF header - if the
 * flags indicate an error condition, return 1; else return 0
 */
int _flag_error(eflags, pathname)
unsigned long eflags;
CONST char *pathname;
{
	if ((eflags == EF_M32_MAU) && !_flags) {
		_rt_lasterr("ld.so: MAU required for file %s", pathname);
		return 1;
	}
	else
		return 0;
}
