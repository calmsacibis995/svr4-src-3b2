/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:cunix/elf_func.c	1.6"

#include <sys/types.h>
#include <sys/localtypes.h>
#include <stdio.h>
#include <a.out.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/dir.h>
#include <dirent.h>
#include <sys/sys3b.h>
#include <sys/sbd.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/boothdr.h>
#include <ctype.h>
#include <sys/error.h>
#include <sys/ledt.h>
#include <sys/sym.h>
#include <sys/dproc.h>
#include <sys/machdep.h>
#include <sys/gen.h>
#include <sys/off.h>
#include <sys/fcntl.h>
#include <sys/cunix.h>
#include <libelf.h>
#include <sys/elf_M32.h>
#include <sys/elf_func.h>
#include <sys/dwarf.h>

void elf_proc_debug();
void static end_elf_debug();

/*
 * This can be expanded to be a more powerfull debugging option.
 * Hope to look into it for 4.0.
 */

 void
elf_debug_info(kp)
struct kernel *kp;
{

	if (kp->otype == O_ELF)
		elf_proc_debug(kp->name);

}
			


void
elf_proc_debug(name)
char *name;
{
	int fd;
	Elf *elf;
	Elf_Data *edata;
	Elf32_Ehdr *ehdr;
	char *strtab;
	register Elf32_Shdr *eshdr;
	register Elf_Scn  *escn;
	register char *sh_name;

	if ((fd = open(name, O_RDONLY)) == -1) {
		error(ER80, name);
		return;
	}

	if ((elf = elf_begin(fd,ELF_C_READ, NULL)) == NULL){
		(void)close(fd);
		error(ER106, name, elf_errmsg(-1));
		return;
	}

	if (!is_elf_reloc(elf)) {
		error(ER58, name);
		end_elf_debug(elf, fd);
		return;
	}

	if ((ehdr = elf32_getehdr(elf)) == NULL) {
		end_elf_debug(elf, fd);
		return;
	}
	
	/* get section header string table */
	if ((escn = elf_getscn(elf, ehdr->e_shstrndx)) == NULL) {
		error(ER115, name);
		end_elf_debug(elf, fd);
		return;
	}

	if ((edata = elf_getdata(escn, NULL)) == NULL || edata->d_buf == NULL) {
		error(ER115, name);
		end_elf_debug(elf, fd);
		return;
	}

	strtab = (char *)edata->d_buf;
	
	escn = (Elf_Scn *)0; 
	while( (escn = elf_nextscn(elf, escn)) != NULL){
		if ((eshdr = elf32_getshdr(escn)) == NULL ||
			((sh_name = (strtab + eshdr->sh_name)) == NULL)) {
			end_elf_debug(elf, fd);
			return;
		}
		if (strcmp(sh_name,DEBUG_NAME) == 0)
			break;
	}

	if (escn == NULL) {
		end_elf_debug(elf, fd);
		return;
	}

	if ((edata = elf_getdata(escn, NULL)) == NULL || edata->d_buf == NULL) {
		end_elf_debug(elf, fd);
		return;
	}
	
	elf_tag_member(edata->d_buf, edata->d_size);

	end_elf_debug(elf, fd);

	return;
}

static void
end_elf_debug(elf, fd)
Elf *elf;
int fd;
{
	elf_end(elf);

	(void)close(fd);

	return;
}


elf_tag_member(buf, size)
char *buf;
int size;
{

	SYMBOL *sp;
	char *cp, name[32], *start;
	int length, location, c;
	short tag, at;  

	cp = buf;

	name[sizeof(name)-1]='\0';
	while (cp < (buf + size)){
		start = cp;
		length = dwarf4(cp);
		tag = dwarf2(cp + 4);
		if (tag == TAG_member){
			c = 0;
			cp += 6;
			while (cp < (start + length)){
				at = dwarf2(cp);
				if (at == AT_location){
					location = dwarf4(cp + 5);
					c++;
				}
				if (at == AT_name){
					strncpy(name, cp + 2, sizeof(name)-1);
					c++;
				}
				if (c == 2)
					break;
				bump_ptr(at, &cp);
			}
			sp = Xsym_name(name);
			if (!(sp->flag & DEFINED)){
				/* MOU and MOS entries */
				sp->flag |= (PASSTHRU | DEFINED);  
				sp->value = location;
			}
		}
		cp = start + length;
	}
}


bump_ptr(at, cp)
short at;
char **cp;
{

	int l4;
	short l2;

	
	switch(at & FORM_MASK){
	case FORM_ADDR:
	case FORM_REF:
	case FORM_DATA4:
			*cp += (4 + 2);
			break;
	case FORM_DATA2:
			*cp += (2 + 2);
			break;
	case FORM_DATA8:
			*cp += (8 + 2);
			break;
	case FORM_BLOCK2:
			l2 = dwarf2(*cp + 2);
			*cp += (l2 + 2 + 2);
			break;
	case FORM_BLOCK4:
			l4 = dwarf4(*cp + 2);
			*cp += (l4 + 4 + 2);
			break;
	case FORM_STRING:
			l4 = strlen(*cp + 2) + 1;
			*cp += (l4 + 2);
			break;
	}
}

is_elf_reloc(elf)
Elf *elf;
{
	register Elf32_Ehdr *ehdr;

	if ((elf_kind(elf)) != ELF_K_ELF) {
		return (0);
	}

	if ((ehdr = elf32_getehdr(elf)) == NULL) {
		return (0);
	}

	if (ehdr->e_type != ET_REL) {
		return (0);
	}

	return (1);
}

is_elf_exec(elf)
Elf *elf;
{
	register Elf32_Ehdr *ehdr;

	if ((elf_kind(elf)) != ELF_K_ELF) {
		return (0);
	}

	if ((ehdr = elf32_getehdr(elf)) == NULL) {
		return (0);
	}

	if (ehdr->e_type != ET_EXEC) {
		return (0);
	}

	return (1);
}
