/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:cunix/off.c	1.16"

#include <string.h>
#include <sys/types.h>
#include <sys/localtypes.h>
#include <sys/param.h>
#include <time.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <a.out.h>
#include <sys/sysmacros.h>
#include <sys/sys3b.h>
#include <sys/mman.h>
#include <sys/ledt.h>
#include <sys/dproc.h>
#include <sys/gen.h>
#include <sys/off.h>
#include <sys/sym.h>
#include <sys/error.h>
#include <sys/cunix.h>
#include <libelf.h>
#include <sys/elf_M32.h>

REL_TAB data_rel, text_rel;

short donotload=0;

int data_symndx;
int text_symndx;

LOCCTR		 text_locctr;		/* .text */
LOCCTR		 data_locctr;		/* .data */
PCB_PATCH	 pcb_patch[256];
STRING_TAB	 string_tab;		/* string table */
NSYMENT 	 nsym;


/*
 * tie everything together and create an object file.
 */
conf(ofile)
char *ofile;
{
	FILHDR file_header;
	SCNHDR text_header,data_header,edt_header;
	address edt_size,text_size,data_size,drsize,trsize,string_size;
	register AUXENT *aux;
	register int i;
	int fd;
	FILE *fptr;


	text_size = SIZE(text_locctr);
	data_size = SIZE(data_locctr);
	edt_size = sizeof(struct s3bconf) + (sys3bconfig->count -1) * sizeof(struct s3bc);
	update_sym(text_size);
	drsize= data_rel.count * RELSZ;
	trsize = text_rel.count * RELSZ;
	string_size = string_tab.locctr - string_tab.origin;

	file_header.f_magic = WE32MAGIC;
	file_header.f_nscns = 3;
	file_header.f_symptr = FILHSZ + 3 * SCNHSZ + text_size + data_size + edt_size + drsize + trsize;
	file_header.f_nsyms = nsym.count;
	file_header.f_opthdr = 0;
	file_header.f_flags = F_LNNO | F_AR32W;
	
	aux = (AUXENT *)&nsym.nsyment[1];
	strcpy(aux->x_file.x_fname,ofile);
	
	strcpy(text_header.s_name,_TEXT);
	text_header.s_paddr = 0;
	text_header.s_vaddr = 0;
	text_header.s_size = text_size;
	text_header.s_scnptr = FILHSZ + 3 * SCNHSZ;
	text_header.s_relptr = text_header.s_scnptr + text_size + data_size + edt_size;
	text_header.s_lnnoptr = 0;
	text_header.s_nreloc = text_rel.count;
	text_header.s_nlnno = 0;
	text_header.s_flags = STYP_TEXT;

	aux = (AUXENT *)&nsym.nsyment[3];
	aux->x_scn.x_scnlen = text_size;
	aux->x_scn.x_nreloc = text_rel.count;
	aux->x_scn.x_nlinno = 0;

	strcpy(data_header.s_name,_DATA);
	data_header.s_paddr = text_size;
	data_header.s_vaddr = text_size;
	data_header.s_size = data_size;
	data_header.s_scnptr = text_header.s_scnptr + text_size;
	data_header.s_relptr = text_header.s_relptr + trsize;
	data_header.s_lnnoptr = 0;
	data_header.s_nreloc = data_rel.count;
	data_header.s_nlnno = 0;
	data_header.s_flags = STYP_DATA;

	aux = (AUXENT *)&nsym.nsyment[5];
	aux->x_scn.x_scnlen = data_size;
	aux->x_scn.x_nreloc = data_rel.count;
	aux->x_scn.x_nlinno = 0;
	
	strcpy(edt_header.s_name,"<EDT>");
	edt_header.s_paddr = 0;
	edt_header.s_vaddr = 0;
	edt_header.s_size = edt_size; 
	edt_header.s_scnptr = data_header.s_scnptr + data_size;
	edt_header.s_relptr = 0;
	edt_header.s_lnnoptr = 0;
	edt_header.s_nreloc = 0;
	edt_header.s_nlnno = 0;
	edt_header.s_flags = STYP_INFO;

	time(&file_header.f_timdat);
	if ((fd = open(ofile,O_CREAT | O_TRUNC | O_WRONLY,0644)) == -1){
		error(ER79,ofile);
		exit(1);
		}

	fptr = fdopen(fd,"w");

	fwrite_and_check(fptr,&file_header,FILHSZ);
	fwrite_and_check(fptr,&text_header,SCNHSZ);
	fwrite_and_check(fptr,&data_header,SCNHSZ);
	fwrite_and_check(fptr,&edt_header,SCNHSZ);

	fwrite_and_check(fptr,text_locctr.v_origin,text_size);
	fwrite_and_check(fptr,data_locctr.v_origin,data_size);

	fwrite_and_check(fptr,sys3bconfig,edt_size);

	for(i=0; i < text_rel.count; i++)
		fwrite_and_check(fptr,&text_rel.rp[i],RELSZ);
	for(i=0; i < data_rel.count; i++)
		fwrite_and_check(fptr,&data_rel.rp[i],RELSZ);

	for(i=0; i < nsym.count; i++)
		fwrite_and_check(fptr,&nsym.nsyment[i],SYMESZ);

	fwrite_and_check(fptr,&string_size,(address)4);
	fwrite_and_check(fptr,string_tab.origin + (address)4,string_size - (address)4);
	fclose(fptr);
	close(fd);
	}

/*
 * fake_sect
 *
 * Driver is missing .data or .bss section header (missing .text is not
 * allowed).  Fake up a header for a zero-length section.
 */

 void
fake_sect(shdr, name, flags)
	register SCNHDR *shdr;
	char *name;
	long flags;
	{

	static SCNHDR null_shdr = { 0 };

	*shdr = null_shdr;
	strcpy(shdr->s_name, name);
	shdr->s_flags = flags;
	}

 void
init_sym_space()
{

	int fd;
	int mult;

        /*
         * Allocate all the space for .o entry.
	 * Note: approximate sizes used so far.
	 */

	if ((fd = open("/dev/zero", O_RDWR)) == -1){
		error(ER80, "/dev/zero");
		exit(1);
	}

	mult=1;
	if (escapehatch) /* In case a really unusual system is being built */
		mult=2;

        text_locctr.v_origin = text_locctr.v_locctr = (address)mmap(0, MAXTSIZE * mult, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
	text_locctr.end = text_locctr.v_origin + MAXTSIZE * mult;

        data_locctr.v_origin = data_locctr.v_locctr = (address)mmap(0, MAXDSIZE * mult, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
	data_locctr.end = data_locctr.v_origin + MAXDSIZE * mult;

	data_rel.origin = (address)mmap(0, MAXDRELOC * mult, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
	data_rel.end = data_rel.origin + MAXDRELOC * mult;
	data_rel.rp = (struct reloc *) data_rel.origin;
	data_rel.count = 0;

	text_rel.origin = (address)mmap(0, MAXTRELOC * mult, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
	text_rel.end = text_rel.origin + MAXTRELOC * mult;
	text_rel.rp = (struct reloc *) text_rel.origin;
	text_rel.count = 0;

	nsym.origin = (address)mmap(0, MAXSYM * mult, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
	nsym.end = nsym.origin + MAXSYM * mult;
	nsym.nsyment = (SYMENT *) nsym.origin;
	nsym.count = 0;

	string_tab.origin = (address)mmap(0, MAXSTRING * mult, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
	string_tab.end = string_tab.origin + MAXSTRING * mult;
	string_tab.locctr = string_tab.origin + (address)4;


	if (string_tab.origin == -1 || text_rel.origin == -1 || data_rel.origin == -1 || data_locctr.v_origin == -1 || text_locctr.v_origin == -1){
		panic("unable to mmap space for conf.o\n");
		exit(1);
	}
	close(fd);

}


free_sym_space()
{

	(void)munmap(text_locctr.v_origin, (text_locctr.end - text_locctr.v_origin));
	(void)munmap(data_locctr.v_origin, (data_locctr.end - data_locctr.v_origin));
	(void)munmap(data_rel.origin, (data_rel.end - data_rel.origin));
	(void)munmap(text_rel.origin, (text_rel.end - text_rel.origin));
	(void)munmap(nsym.origin, (nsym.end - nsym.origin));
	(void)munmap(string_tab.origin, (string_tab.end - string_tab.origin));
}


/*
 * Create .file .data and .text entries.
 *
 */
init_sym()
{
	register int i;

	i = nsym.count;
	strcpy(nsym.nsyment[i].n_name,".file");
	nsym.nsyment[i].n_value = 0;
	nsym.nsyment[i].n_scnum = N_DEBUG;
	nsym.nsyment[i].n_type = T_NULL;
	nsym.nsyment[i].n_sclass = C_FILE;
	nsym.nsyment[i].n_numaux = 1;
	i+=2;   /* skip aux entries filled in later by conf */
	text_symndx = i;
	strcpy(nsym.nsyment[i].n_name,_TEXT);
	nsym.nsyment[i].n_value = text_locctr.v_origin;
	nsym.nsyment[i].n_scnum = 1;
	nsym.nsyment[i].n_type = T_NULL;
	nsym.nsyment[i].n_sclass = C_STAT;
	nsym.nsyment[i].n_numaux = 1;
	i+=2;
	data_symndx = i;
	strcpy(nsym.nsyment[i].n_name,_DATA);
	nsym.nsyment[i].n_value = data_locctr.v_origin;
	nsym.nsyment[i].n_scnum = 2;
	nsym.nsyment[i].n_type = T_NULL;
	nsym.nsyment[i].n_sclass = C_STAT;
	nsym.nsyment[i].n_numaux = 1;
	i+=2;
	nsym.count = i;
}
	

/*
 * Update symbol and relocation table with new offsets.
 */

update_sym(size)
address size;
{
	register int i;

	for(i=0;i< nsym.count;i++){
		if (nsym.nsyment[i].n_scnum == 1)
			nsym.nsyment[i].n_value -= text_locctr.v_origin;
		if (nsym.nsyment[i].n_scnum == 2)
			nsym.nsyment[i].n_value += size - data_locctr.v_origin;
	}

	for(i=0; i< data_rel.count; i++){
		if (data_rel.rp[i].r_symndx == data_symndx )
				*((long *) data_rel.rp[i].r_vaddr) += size - data_locctr.v_origin;
		if (data_rel.rp[i].r_symndx == text_symndx )
				*((long *) data_rel.rp[i].r_vaddr) -= text_locctr.v_origin;
		data_rel.rp[i].r_vaddr += size - data_locctr.v_origin;
	}

	for(i=0; i< text_rel.count;i++)
		text_rel.rp[i].r_vaddr -= text_locctr.v_origin;
}

/*
 * patch(), We need to change the paddr field in the section 
 * headers to tell boot where to load the sections. All sections 
 * are loaded immediately following boot text and boot data.
 * We also patch the interrupt vectors and the crash_sync variable.
 * Patching the Ivect table is very ugly but we don't have any choice.
 *
 */

patch()
{
	SYMENT *s;
	FILHDR fhdr;
	SCNHDR data,text,boot,gate,bss,scnhdr,*shdr;
	long paddr,faddr,physad,Sgate,gateSIZE,Stext,textSIZE,Sdata,dataSIZE,Sbss;
	struct tm *boot_time;
	long ivecstart,c_sync_addr,c_sync_time,btime;
	int i,index,fd;
	char *np;

	
	fd = open(unix_abs,O_RDWR);

	switch (object_file_type(fd)){
	case O_ELF:
		if ((elf_patch()) != 0) {
			error(ER120, unix_abs);
			(void)unlink(unix_abs);
			exit (1);
		}
		break;

	case O_COFF:
		if ((shdr= coff_section(unix_abs)) == NULL){
				error(ER60,unix_abs);
				(void)unlink(unix_abs);
				exit(1);
				}
		do {
			if (strcmp(".gate",shdr->s_name) == 0){
				gate = *shdr;
				continue;
				}
			if (strcmp("boot",shdr->s_name) == 0){
				boot = *shdr;
				continue;
				}
			if (strcmp(".text",shdr->s_name) == 0){
				text = *shdr;
				continue;
				}
			if (strcmp(".data",shdr->s_name) == 0){
				data = *shdr;
				continue;
				}
			if (strcmp(".bss",shdr->s_name) == 0){
				bss = *shdr;
				continue;
				}
		} while((shdr = coff_section((char *)NULL)) != NULL);
		
	/* mmimic calculations in ifile */
		paddr = btoc(boot.s_paddr) + btoc(boot.s_size);
		physad = ctob(paddr);
		Sgate = physad;
		gateSIZE = gate.s_size;
	
		paddr += btoc(gateSIZE);
	
		physad = ctob(paddr);
		Stext = physad;
		textSIZE = text.s_size;
		
		paddr += btoc(textSIZE);
	
		physad = ctob(paddr);
		Sdata = physad;
		dataSIZE = data.s_size;
		
		paddr += btoc(dataSIZE);
	
		physad = ctob(paddr);
		Sbss = physad;
		
		
		if ((s = coff_symbol(unix_abs,&index)) != NULL)
			do {
				if (s->n_zeroes)
					np = s->n_name;
				else
					np = s->n_name + s->n_offset;
	
					if (strcmp("crash_sync",np) == 0)
						c_sync_addr = s->n_value;
					if (strcmp("Ivect",np) == 0)
						ivecstart = s->n_value;
					if (strncmp("kpcb",np,4) == 0)
						for(i = 0;i<nkpcb;i++)
							if (strcmp(&np[4],pcb_patch[i].suffix) == 0)
								pcb_patch[i].value = s->n_value;
		} while ((s = coff_symbol((char *) NULL,&index)) != NULL);
	
	
		time(&btime);
		boot_time = gmtime(&btime);
		c_sync_time = boot_time->tm_sec;
		c_sync_time += boot_time->tm_min * 60;
		c_sync_time += boot_time->tm_hour * 60 * 60;
		c_sync_time += boot_time->tm_mday * 24 * 60 * 60;
	
		lseek(fd,(boot.s_scnptr + (c_sync_addr - boot.s_paddr )),0);
		write(fd,&c_sync_time,sizeof(long));
	
		/* patch Ivect table */
		for (i=0; i<nkpcb; i++){
			lseek(fd,(gate.s_scnptr + ivecstart + pcb_patch[i].vector * sizeof(long)),0);
			write(fd,&pcb_patch[i].value,sizeof(long));
		}
	
		gate.s_paddr = Sgate;
		text.s_paddr = Stext;
		data.s_paddr = Sdata;
		bss.s_paddr = Sbss;
	
		if ((lseek(fd, 0L, 0)) == -1L) {
			perror("patch: lseek");
		}
		read_and_check(fd,(char *)&fhdr,FILHSZ);
		lseek(fd,FILHSZ + fhdr.f_opthdr,0);
		faddr = FILHSZ + fhdr.f_opthdr;
		/* patch paddr fields */
		i = fhdr.f_nscns;
		while ( i-- != 0){
			read_and_check(fd,(char *)&scnhdr,SCNHSZ);
			if (strcmp(".gate",scnhdr.s_name) == 0)
				update_scn(fd,faddr,&gate);
			if (strcmp(".text",scnhdr.s_name) == 0)
				update_scn(fd,faddr,&text);
			if (strcmp(".data",scnhdr.s_name) == 0)
				update_scn(fd,faddr,&data);
			if (strcmp(".bss",scnhdr.s_name) == 0)
				update_scn(fd,faddr,&bss);
		faddr += SCNHSZ;
		}
	
		close(fd);
	
		if (!confdebug)
			unlink(conf_file);
		break;
	default:
		break;
	}

	/*
	**	patch the virtual address variables in unix_abs
	*/

	if ((patch_addr()) != 0) {
		error(ER120, unix_abs);
		(void)unlink(unix_abs);
		exit(1);
	}
}	

update_scn(fd,faddr,shdr)
int fd;
long faddr;
SCNHDR *shdr;
{
	lseek(fd,faddr,0);
	write(fd,shdr,SCNHSZ);
}

elf_patch ()
{
	char *np;
	int i, j, fd;
	unsigned int string_index;
	extern int errno;
	long ivecindex, paddr, clicks, nsyms, btime, ivecstart, size;
	struct tm *boot_time;
	Elf *elfd;
	Elf_Data *data;
	Elf_Scn *esec;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *ephdr;
	Elf32_Shdr *eshdrsym, *eshdrstr;
	Elf32_Sym *symtab;

	if ((fd = open (unix_abs, O_RDWR)) < 0) {
		error (ER112, unix_abs, errno);
		return (-1);
	}

	if ((elfd = elf_begin (fd, ELF_C_RDWR, NULL)) == NULL) {
		error (ER106, unix_abs, elf_errmsg(-1));
		(void)close (fd);
		return (-1);
	}

	if (!is_elf_exec(elfd)) {
		/* this is a disaster...unix_abs is corrupt, should be unlinked, cunix exit'd */
		elf_end(elfd);
		(void)close(fd);
		return (-1);
	}

	if ((ehdr = elf32_getehdr (elfd)) == NULL) {
		error(ER119, unix_abs, elf_errmsg(-1));
		elf_end (elfd);
		close (fd);
		return (-1);
	}

	if ((ephdr = elf32_getphdr (elfd)) == NULL) {
		error (ER113, elf_errmsg (elf_errno ()));
		elf_end (elfd);
		close (fd);
		return (-1);
	}

	for (i=0; (Elf32_Half)i < ehdr->e_phnum; i++){
		if (ephdr[i].p_paddr ){
			paddr= ephdr[i].p_paddr;
			size = ctob(btoc(ephdr[i].p_memsz));
			break;
		}
	}

	for (i=0; (Elf32_Half)i < ehdr->e_phnum; i++){
		if (ephdr[i].p_paddr || ephdr[i].p_memsz == 0 ||
		     ephdr[i].p_type != PT_LOAD)
			continue;

		paddr += size;
		ephdr[i].p_paddr = paddr;
		size = ctob(btoc(ephdr[i].p_memsz));
	}
		
	elf_flagphdr (elfd, ELF_C_SET, ELF_F_DIRTY);

	for (esec = (Elf_Scn *) 0; esec = elf_nextscn (elfd, esec); ) {
		if ((eshdrsym = elf32_getshdr (esec)) == NULL) {
			error (ER114, elf_errmsg (elf_errno ()));
			elf_end (elfd);
			close (fd);
			return (-1);
		}
		if (eshdrsym->sh_type == SHT_SYMTAB)
			break;
	}
	if (esec == NULL || (data = elf_getdata (esec, NULL)) == NULL || (symtab = (Elf32_Sym *) data->d_buf) == NULL) {
		error (ER111, unix_abs);
		elf_end (elfd);
		close (fd);
		return (-1);
	}
	nsyms = eshdrsym->sh_size / eshdrsym->sh_entsize - 1;

	string_index = eshdrsym->sh_link;

	time (&btime);
	boot_time = gmtime (&btime);
	btime = boot_time->tm_sec + boot_time->tm_min * 60 +
	    boot_time->tm_hour * 60 * 60 + boot_time->tm_mday *24 * 60 * 60;
	if ((elf_patch_symbol (elfd, "crash_sync", (unsigned int) btime,
			nsyms, string_index, symtab)) == -1) {
			return (-1);
	}
	

	for (i = 1; i <= nsyms; i++)
		if (strcmp ("Ivect", elf_strptr (elfd, string_index, symtab[i].st_name)) == 0)
			break;
	if (i > nsyms) {
		error (ER116, "Ivect");
		return (-1);
	}

	ivecstart = symtab[i].st_value;
	ivecindex = symtab[i].st_shndx;

	for (j = 1; j <= nsyms; j++)
		if (strncmp ("kpcb", np = elf_strptr (elfd, string_index, symtab[j].st_name), 4) == 0)
			for (i = 0; i < nkpcb; i++)
				if (strcmp (&np[4], pcb_patch[i].suffix) == 0)
					elf_patch_value (elfd, ivecstart + pcb_patch[i].vector * sizeof(long), pcb_patch[i].value = symtab[j].st_value, ivecindex);


	elf_flagelf (elfd, ELF_C_SET, ELF_F_LAYOUT);
	elf_update (elfd, ELF_C_WRITE);
	elf_end (elfd);
	(void)close (fd);

	if (!confdebug)
		unlink (conf_file);
	return (0);
}



elf_patch_symbol (elfd, name, value, nsyms, string_index, symtab)
Elf *elfd;
char *name;
unsigned int value, string_index;
long nsyms;
Elf32_Sym *symtab;
{
	long i;

	for (i = 1; i <= nsyms; i++)
		if (strcmp (name, elf_strptr (elfd, string_index, symtab[i].st_name)) == 0)
			break;
	if (i > nsyms) {
		error (ER116, name);
		return (-1);
	}
	return (elf_patch_value (elfd, symtab[i].st_value, value, symtab[i].st_shndx));
}

elf_patch_value (elfd, vaddr, value, index)
Elf *elfd;
Elf32_Addr vaddr;
unsigned int value;
Elf32_Half index;
{
	Elf_Data *data;
	Elf_Scn *esec;
	Elf32_Shdr *eshdr;

	if ((esec = elf_getscn(elfd, index)) == NULL){
		error(ER117, vaddr);
		return(-1);
	}
	if ((eshdr = elf32_getshdr(esec)) == NULL){
		error(ER114, elf_errmsg(elf_errno()));
		return(-1);
	}

	if ((data = elf_getdata (esec, NULL)) == NULL || data->d_buf == NULL) {
		error (ER118, vaddr);
		return (-1);
	}

	*((unsigned int *) (&((char *) data->d_buf)[vaddr - eshdr->sh_addr])) = value;
	elf_flagdata (data, ELF_C_SET, ELF_F_DIRTY);
	return (1);
}

	/*
	**	This function patches the variables in unix_abs that are to contain
	**	(defined as ints in startup.c)
	**	the starting virtual addresses of kernel segments (sections).
	**	The addresses are determined by reading the segment (program) headers
	**	which were set by ld(1) based on the mapfile (ifile).
	*/
patch_addr()
{
	register Elf32_Sym *symtab;
	register int n, s;
	Elf *elf;
	Elf32_Ehdr *ehdr;
	Elf32_Shdr *shdr;
	Elf32_Phdr *phdr;
	Elf_Scn *scn;
	Elf_Data *data, *symdata;
	char *strtab;
	char *name;
	long offset;
	int fd;
	int nsyms;
	int objtype;
	static struct {
		char	*sname;
		long	addr;
		int	sect;
		long	value;
	} symbols[] = {
		{ "sgate", 0, 0, 0 },
		{ "null", 0, 0, 0 },
		{ "sboot", 0, 0, 0 },
		{ "stext", 0, 0, 0 },
		{ "sdata", 0, 0, 0 },
		{ "sbss", 0, 0, 0 },
		{ NULL }
	};

	if ((fd = open (unix_abs, O_RDWR)) == -1) {
		perror(unix_abs);
		return (-1);
	}

	if ((elf = elf_begin (fd, ELF_C_READ, NULL)) == NULL) {
		error(ER106, unix_abs, elf_errmsg(-1));
		close (fd);
		return (-1);
	}

	objtype = elf_kind(elf);

	if ((objtype != ELF_K_ELF) && (objtype != ELF_K_COFF)) {
		elf_end(elf);
		close(fd);
		return (-1);
	}

	if ((ehdr = elf32_getehdr (elf)) == NULL) {
		error(ER119, unix_abs, elf_errmsg(-1));
		elf_end (elf);
		close (fd);
		return (-1);
	}

	if (ehdr->e_type != ET_EXEC) {
		elf_end (elf);
		close (fd);
		return (-1);
	}

	/*
	**	set up values
	*/

	if (objtype == ELF_K_ELF) {
		if ((phdr = elf32_getphdr(elf)) == NULL) {
			error(ER113, unix_abs);
			elf_end(elf);
			close(fd);
			return(-1);
		}

		s = (int)ehdr->e_phnum;
		for (n = 0; n < s; n++) {
			if (n == 1)
				continue;

			if (!symbols[n].sname)
				break;

			symbols[n].value = phdr[n].p_vaddr;
		}
	} else {	/* For COFF, work by section headers */

		s = (int)ehdr->e_shnum;
		for (n = 1; n < s; n++) {
			scn = elf_getscn(elf, n);
			if ((shdr = elf32_getshdr(scn)) == NULL) {
				error(ER114, unix_abs);
				elf_end(elf);
				close(fd);
				return(-1);
			}

			if (!symbols[n-1].sname)
				break;

			name = elf_strptr(elf, ehdr->e_shstrndx, shdr->sh_name);

			if ((strcmp(name, ".gate")) == 0)
				symbols[0].value = shdr->sh_addr;
			else if ((strcmp(name, "boot")) == 0)
				symbols[2].value = shdr->sh_addr;
			else if ((strcmp(name, ".text")) == 0)
				symbols[3].value = shdr->sh_addr;
			else if ((strcmp(name, ".data")) == 0)
				symbols[4].value = shdr->sh_addr;
			else if ((strcmp(name, ".bss")) == 0)
				symbols[5].value = shdr->sh_addr;
		}
	}

	/*
	**	get symbol table
	*/

	scn = NULL;
	while ((scn = elf_nextscn(elf, scn)) != NULL) {

		if ((shdr = elf32_getshdr(scn)) == NULL) {
			error(ER114, unix_abs);
			elf_end(elf);
			close(fd);
			return(-1);
		}

		if (shdr->sh_type != SHT_SYMTAB)
			continue;

		data = elf_getdata(scn, NULL);

		if ((!data) || (!data->d_buf) || (!data->d_size)) {
			error(ER111, unix_abs);
			elf_end(elf);
			close(fd);
			return(-1);
		}

		symdata = data;		/* save for update */

		symtab = data->d_buf;

		nsyms = data->d_size / sizeof(Elf32_Sym);

		/*
		**	get string table
		*/

		if ((scn = elf_getscn(elf, shdr->sh_link)) == NULL) {
			error(ER115, unix_abs);
			elf_end(elf);
			close(fd);
			return(-1);
		}

		data = elf_getdata(scn, NULL);

		if ((!data) || (!data->d_buf) || (!data->d_size)) {
			error(ER115, unix_abs);
			elf_end(elf);
			close(fd);
			return(-1);
		}

		strtab = data->d_buf;

		break;
	}

	if (!symtab) {
		error(ER111, unix_abs);
		elf_end(elf);
		close(fd);
		return(-1);
	}

	symtab++;
	nsyms--;

	for (s = 0; s < nsyms; s++) {
		name = strtab + symtab[s].st_name;

		if (name) {
			for (n = 0; symbols[n].sname; n++) {
				if ((strcmp(name, symbols[n].sname)) == 0) {
					symbols[n].addr = symtab[s].st_value;
					symbols[n].sect = symtab[s].st_shndx;
					break;
				}
			}
		}
	}

	/*
	**	Set the absolute
	*/

	for (n = 0; symbols[n].sname; n++) {
		if (n == 1)
			continue;

		scn = elf_getscn(elf, symbols[n].sect);
		if ((shdr = elf32_getshdr(scn)) == NULL) {
			error(ER114, unix_abs);
			elf_end(elf);
			close(fd);
			return(-1);
		}

		offset = symbols[n].addr - shdr->sh_addr + shdr->sh_offset;

		if ((lseek(fd, offset, 0)) == -1L) {
			perror("lseek");
			elf_end(elf);
			(void)close(fd);
			return (-1);
		}

		if ((write(fd, &symbols[n].value, sizeof(int))) != sizeof(int)) {
			perror("write");
			elf_end(elf);
			(void)close(fd);
			return (-1);
		}
	}

	elf_end(elf);

	close(fd);

	return (0);
}


/*
 * set r_vaddr now that an address has  been established for R_SPEC4
 * symbols.
 */
rel_spec4(locctr)
register address locctr;
{
	register SYMBOL *sp;
	register int i,j;

	for(i=data_rel.count - 1; i> -1; i--)
		if (data_rel.rp[i].r_type == R_SPEC4){
			sp = (SYMBOL *)Xsym_name(last_exp);
			if (sp->flag & CONFLOCAL){
				j = sp->nsymindex;
				if (data_rel.rp[i].r_symndx == data_symndx || data_rel.rp[i].r_symndx == text_symndx || (nsym.nsyment[j].n_scnum == 0 && nsym.nsyment[j].n_value != 0))
					*((long *)locctr) += nsym.nsyment[j].n_value;
			}
			data_rel.rp[i].r_type = R_DIR32;
			data_rel.rp[i].r_vaddr = locctr;
			break;
		}
}

/* 
 * search nsym table.
 *
 */
searchnsym(name)
register char *name;
{
	register int i;
	register int j;

	j = strlen(name);
	for(i=nsym.count - 1;i > -1;i--)
		if (nsym.nsyment[i].n_zeroes ){
			if (strncmp(nsym.nsyment[i].n_name,name,SYMNMLEN) == 0 && j <= SYMNMLEN)
				return(i);
		}
		else 
			if (strcmp((char *)(nsym.nsyment[i].n_offset + string_tab.origin),name) == 0)
				return(i);	
	return(-1);
}


/*
 * Xrel_replace(paddr,type,name,rel) 
 *
 * Replace a previous relocation reference to this address with name
 *
 */
 void
Xrel_replace(paddr,type, name,rel)
register address paddr;
register unsigned short type;
register char *name;
register REL_TAB *rel;
{
	register int i;
	register int j,k;

	for (i = rel->count; i > -1;i--)
		if (rel->rp[i].r_vaddr == paddr){
			j = ((struct Xsymbol *)Xsym_name(name))->nsymindex;
			if ((k = nsym.nsyment[j].n_scnum) != 0){
				if (k == 1)
					rel->rp[i].r_symndx = text_symndx;
				else	
					rel->rp[i].r_symndx = data_symndx;
				*((long *)paddr) = nsym.nsyment[j].n_value;
			}
			else {
				if ( nsym.nsyment[j].n_value != 0)
					*((long *)paddr) = nsym.nsyment[j].n_value;
				rel->rp[i].r_symndx = j;
			      }
			return;
		}
}
	
		
/*
 * Xrelocate(paddr, type, name)
 *
 * This routine will either perform the relocation, or queue a Xreloc entry
 * for a symbol.  Input is the real address to be relocated, the type of
 * relocation and the name of the symbol to which `paddr' is to be relocated.
 * User Level comments - here and in Xrel_replace we build the relocation
 * entries that eventually get put in conf.o . This gets a little tricky
 * since we have to check whether the symbol is defined within conf.o
 * text or data and also bss symbols have the size of the symbol in location
 * paddr.
 */
 void
Xrelocate(paddr, type, name, rel)
	register address paddr;
	register unsigned short type;
	register char *name;
	register REL_TAB *rel;
	{

	register struct Xsymbol *sp;
	register RELOC *prp;
	register int i,j;

	sp = (struct Xsymbol *)Xsym_name(name);

	prp = &rel->rp[rel->count++];

	if ((address)prp >= rel->end){
		error(ER11);
		exit(1);
	}

	prp->r_vaddr = paddr;
	prp->r_type = type;
	i = sp->nsymindex;
	if ((j = nsym.nsyment[i].n_scnum) != 0){
		if (j == 1)
			prp->r_symndx = text_symndx;
		else 	
			prp->r_symndx = data_symndx;
		*((long *)paddr) = nsym.nsyment[i].n_value;
	}
	else  {
		if ( nsym.nsyment[i].n_value != 0)
			*((long *)paddr) = nsym.nsyment[i].n_value;
		prp->r_symndx = i;
	      }

	}


/*
 * Allocate(&locctr, name, size)
 *
 * Allocate `size' storage for symbol `name' at *locctr.  The location counter
 * is updated to the next mod sizeof(int) boundry.  The symbol `name'
 * must not have been previously defined.
 */
 address
allocate(locctr, name, size,scnum)
	register address *locctr;
	register char *name;
	long size;
	register int scnum;
	{

	register struct Xsymbol *sp;

	if (Ksym_name(name) != NULL)
		{
		/* <name>: already allocated */
		error(ER62, name);
		donotload=1;
		return(0);
		}

	sp = (struct Xsymbol *) Xsym_name(name);

	if (sp->flag & DEFINED)
		{
		/* <name>: previously allocated */
		error(ER48, name);
		donotload=1;
		return(0);
		}

	sp->flag |= DEFINED | CONFLOCAL;
	sp->nsymindex = nsym.count;


	copysym(name,nsym.count);
	nsym.nsyment[nsym.count].n_value = *locctr;
	nsym.nsyment[nsym.count].n_scnum = scnum;
	nsym.nsyment[nsym.count].n_type = T_NULL;
	nsym.nsyment[nsym.count].n_sclass = C_EXT;
	nsym.nsyment[nsym.count].n_numaux = (char) 0;
	if ((address)&nsym.nsyment[++nsym.count] >= nsym.end){
		error(ER11);
		exit(1);
	}

	sp->x.value = *locctr;

	*locctr += (size + sizeof(int) - 1) & ~(sizeof(int) - 1);

	if ((scnum == 1 && (address)*locctr >= text_locctr.end) || (scnum == 2 && (address)*locctr >= (address)data_locctr.end)){
		error(ER11);
		exit(1);
	}

	return(sp->x.value);
	}

/*
 * adjust r_vaddr for all symbols of type typ.
 * see G_DATA in generate.
 */
adj_rel(locctr,typ)
register address locctr;
register unsigned short int typ;
{
	register int i;

	for(i=0; i< data_rel.count; i++)
		if (data_rel.rp[i].r_type == typ){
			data_rel.rp[i].r_type = R_DIR32;
			data_rel.rp[i].r_vaddr += locctr;
			}
}

/*
 * Copysym(name,nindex)
 * 
 * copy symbol to name or string table.
 */
copysym(name,nindex)
char *name;
{
	register int n;

	if ((n=strlen(name)) > 8){
		strcpy((char *)string_tab.locctr,name);
		nsym.nsyment[nindex].n_zeroes = 0;
		nsym.nsyment[nindex].n_offset = string_tab.locctr - string_tab.origin;
		string_tab.locctr += (address) n + (address)1;
		if (string_tab.locctr >= string_tab.end){
			error(ER11);
			exit(1);
		}
	} else
		strcpy(nsym.nsyment[nindex].n_name,name);
}

/*
 * Alloc_string(string)
 *
 * Allocate a character string in the data section and return its virtual
 * address. The data location counter is updated to the next mod sizeof(int)
 * boundry.
 */
 address
alloc_string(string)
	register char *string;
	{

	register char *to;
	register address save;

	to = (char*) REAL(save=data_locctr.v_locctr, data_locctr);

	strcpy(to, string);

	data_locctr.v_locctr += (strlen(string) + 1 + sizeof(int) - 1) & ~(sizeof(int) - 1);

	if (data_locctr.v_locctr >= data_locctr.end){
		error(ER11);
		exit(1);
	}

	return(save);
	}
/*
 * create an external entry in the symbol table.
 *
 */
ext_allocate(name)
register char *name;
{
	register struct Xsymbol *sp;

	sp = (struct Xsymbol *) Xsym_name(name);
	sp->flag |= CONFLOCAL;
	sp->nsymindex = nsym.count;
	copysym(name,nsym.count);
	nsym.nsyment[nsym.count].n_value = 0;
	nsym.nsyment[nsym.count].n_scnum = 0;
	nsym.nsyment[nsym.count].n_type = T_NULL;
	nsym.nsyment[nsym.count].n_sclass = C_EXT;
	nsym.nsyment[nsym.count].n_numaux = (char) 0;
	if ((address)&nsym.nsyment[++nsym.count] >= nsym.end){
		error(ER11);
		exit(1);
	}
	
}

/* 
 * create an bss entry in the symbol table.
 */
bss_allocate(name,size)
register char *name;
register address size;
{
	register struct Xsymbol *sp;

	/*
	 * We can not create a bss entry of size 0. However, the
	 * original lboot puts this symbol in the symbol table anyway.
	 * This is a bug since the old lboot can still bind a reference
	 * to this address. There will be a compatibility problem if
	 * some included brain damaged driver refers to this symbol.
	 */
	if (size == 0) {
		/* common situation */
		size = sizeof(int);
	}

	if (Ksym_name(name) != NULL)
		{
		/* <name>: already allocated */
		error(ER62, name);
		donotload=1;
		return(0);
		}

	sp = (struct Xsymbol *) Xsym_name(name);

	if (sp->flag & DEFINED)
		{
		/* <name>: previously allocated */
		error(ER48, name);
		donotload=1;
		return(0);
		}

	sp->flag |= DEFINED | CONFLOCAL;



	sp->nsymindex = nsym.count;
	copysym(name,nsym.count);
	nsym.nsyment[nsym.count].n_value = size;
	nsym.nsyment[nsym.count].n_scnum = 0;
	nsym.nsyment[nsym.count].n_type = T_NULL;
	nsym.nsyment[nsym.count].n_sclass = C_EXT;
	nsym.nsyment[nsym.count].n_numaux = (char) 0;
	if ((address)&nsym.nsyment[++nsym.count] >= nsym.end){
		error(ER11);
		exit(1);
	}
	return(1);
}

/*
 * Coff_section(fname)
 *
 * Read the section headers of an object file.  The first time this routine
 * is called, it must be given a path name for the object file.  Each
 * subsequent time the routine is called, fname must be NULL.  A pointer
 * to a static section header is returned.  When there are no more
 * section headers in the object file, NULL is returned and the file is closed.
 *
 * Any section marked NOLOAD is ignored.
 */
 SCNHDR *
coff_section(fname)
	register char *fname;
	{

	static SCNHDR shdr;
	static int fd = -1;
	static unsigned short nscns;
	FILHDR fhdr;


	if (fname != NULL)
		{
		if (fd != -1)
			close(fd);
		if ((fd=open(fname,O_RDONLY)) == -1)
			{
			error(ER80, fname);
			return(NULL);
			}
		read_and_check(fd, (char*)&fhdr, FILHSZ);
		if (fhdr.f_magic != FBOMAGIC || (nscns=fhdr.f_nscns) == 0)
			{
			/* <fname>: invalid object file */
			error(ER58, fname);
			goto exit;
			}
		seek_and_check(fd, (long)fhdr.f_opthdr, 1);
		}
	else
		{
		if (fd == -1)
			return(NULL);
		}

	/*
	 * read the section headers
	 */
	while (nscns-- != 0)
		{

		read_and_check(fd, (char*)&shdr, SCNHSZ);

		if (! (shdr.s_flags & (STYP_NOLOAD|STYP_DSECT)))
			return(&shdr);
		}

exit:	close(fd);
	fd = -1;
	return(NULL);
	}

/*
 * Coff_symbol(fname, number)
 *
 * Read the symbol table of an object file.  The first time this routine
 * is called, it must be given a path name for the object file.  Each
 * subsequent time the routine is called, fname must be NULL.  A pointer
 * to a static symbol table entry is returned.  When there are no more
 * symbols in the object file, NULL is returned and the file is closed.
 *
 * Only symbols whose storage class is C_EXT or C_STAT are returned.  An
 * optional second parameter is a pointer to an long; if not NULL
 * then the index number of the symbol is returned.
 *
 * If the symbol is exactly SYMNMLEN characters long, then it will be
 * converted to a flexname in order to make it null-terminated.  If the
 * symbol name is a flexname, then the string table will be read.
 * The offset (n_offset) is modified to be relative to the symbol table
 * entry n_name[].  Thus, the symbol name is accessed by:
 *
 *		s = coff_symbol(...)
 *		if (s->n_zeroes)
 *			/* s->n_name[SYMNMLEN] is the null-terminated symbol
 *		else
 *			/* (s->n_name+s->n_offset) -> the symbol
 */
 SYMENT *
coff_symbol(fname, number)
	register char *fname;
	register long *number;
{

	extern boolean	DebugMode;

	/* For ELF */

	Elf_Scn *scn;
	Elf32_Shdr *eshdr;
	Elf32_Ehdr *ehdr;
	Elf_Data *data;
	static Elf *elfd = NULL;
	static Elf32_Sym *symtab = NULL;
	static char *strtab;

	/* For COFF */

	static FILE *stream = NULL;
	static FILE *string = NULL;
	static long stringorigin;
	static unsigned int strndx = -1;
	union auxent	auxentry;
	FILHDR fhdr;

	/* Both ELF & COFF */
	char *sname;
	static char *bootmod;
	static long nsyms, sequence;
	static int is_elf = 0;
	static int fd = -1;
	static struct {
		SYMENT syment;
		char flexname[256];
	} sentry;

	if ((fname == NULL) && (fd == -1))
		return (NULL);

	if (fname != NULL) {

			/* clean up and close previous file */
		if (elfd)
			elf_end(elfd);

		if (stream)
			fclose(stream);

		if (string)
			fclose(string);

		if (fd != -1)
			close(fd);

		stream = NULL;
		string = NULL;
		elfd = NULL;
		symtab = NULL;
		fd = -1;

			/* Open new file */
		if ((fd = open(fname, O_RDONLY)) == -1) {
			error(ER80, fname);
			return (NULL);
		}

		bootmod = fname;

		switch (object_file_type(fd)){
		case O_COFF:
			is_elf = 0;

			if ((lseek(fd, 0L, 0)) == -1) {
				goto end_of_job;
			}

			read_and_check(fd, (char*)&fhdr, FILHSZ);

			if ((nsyms = fhdr.f_nsyms) == 0) {
				goto end_of_job;
			}

			seek_and_check(fd, fhdr.f_symptr, 0);

			stream = fdopen(fd,"r");

			stringorigin = fhdr.f_symptr + nsyms * SYMESZ;

			if ((string = fopen(fname, "r")) == NULL) {
				error(ER80, fname);
				goto end_of_job;
			}

			sequence = 0;

			break;

		case O_ELF:
			is_elf = 1;

			if ((elfd = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
				error(ER106, bootmod, elf_errmsg(-1));
				goto end_of_job;
			}

				/* Check if libelf knows this as an ELF
				** object.  We didn't need to do this in
				** the COFF case, above, because the
				** function object_file_type() specifically
				** checked FBOMAGIC.
				*/
	
			if (!is_elf_reloc(elfd)) {
				error(ER58, bootmod);
				goto end_of_job;
			}



				/* Get symbol table */
			scn = NULL;
			while ((scn = elf_nextscn(elfd, scn)) != NULL) {

				if ((eshdr = elf32_getshdr(scn)) == NULL) {
					error(ER114, bootmod);
					goto end_of_job;
				}

				if (eshdr->sh_type != SHT_SYMTAB)
					continue;

				/* got a symbol table, read it in */

				if ((data = elf_getdata(scn, NULL)) == NULL) {
					error(ER118, eshdr->sh_offset);
					goto end_of_job;
				}

				if (((symtab = (Elf32_Sym *)data->d_buf) == NULL)
					|| data->d_size == 0) {
					error(ER111, bootmod);
					goto end_of_job;
				}

				nsyms = data->d_size / sizeof(Elf32_Sym);

				sequence = 1;

				/* get string tab ptr and read it in */

				if ((scn = elf_getscn(elfd, eshdr->sh_link)) == NULL) {
					error(ER115, bootmod);
					goto end_of_job;
				}

				if ((eshdr = elf32_getshdr(scn)) == NULL) {
					error(ER114, bootmod);
					goto end_of_job;
				}

				if (eshdr->sh_type != SHT_STRTAB) {
					error(ER115, bootmod);
					goto end_of_job;
				}

				if ((data = elf_getdata(scn, NULL)) == NULL) {
					error(ER118, eshdr->sh_offset);
					goto end_of_job;
				}

				if (((strtab = (char *)data->d_buf) == NULL)
					|| data->d_size == 0) {
					error(ER115, bootmod);
					goto end_of_job;
				}

				break;	/* Can process only 1 symbol table */
			}

			if (symtab == NULL) {
			/* Came out of while loop & never found SHT_SYMTAB */
				error(ER111, bootmod);
				goto end_of_job;
			}

			if (strtab == NULL) {
			/* Came out of while loop & never found SHT_STRTAB */
				error(ER115, bootmod);
				goto end_of_job;
			}

			break;
		default:
			error(ER58, bootmod);
			goto end_of_job;
		}
	}

	/*
	 * read the symbol table
	 */

	while (sequence < nsyms) {

		if (is_elf) {

			symtab++;

			if (number != NULL)
				*number = sequence;

			sequence++;

			if (ELF32_ST_TYPE(symtab->st_info) == STT_SECTION ||
			    ELF32_ST_TYPE(symtab->st_info) == STT_FILE) {
				continue;
			}

			if ((ELF32_ST_BIND(symtab->st_info) == STB_LOCAL) &&
				(symtab->st_shndx == SHN_UNDEF)) {
				continue;
			}

			sname = strtab + symtab->st_name;

			if (sname)
				strncpy (sentry.flexname, sname, 255);

			if (strlen(sentry.flexname) < (size_t) SYMNMLEN)
				strcpy (sentry.syment.n_name, sentry.flexname);
			else {
				sentry.syment.n_zeroes = 0;
				sentry.syment.n_offset = sentry.flexname - sentry.syment.n_name;
			}

			sentry.syment.n_value = symtab->st_value;

			sentry.syment.n_sclass =
			(ELF32_ST_BIND(symtab->st_info) == STB_LOCAL ? C_STAT : C_EXT);

			sentry.syment.n_type = T_NULL;

			sentry.syment.n_numaux = 0;

			sentry.syment.n_scnum =
			(ELF32_ST_TYPE(symtab->st_info) == STT_FUNC ? 1 : 2 );

			switch ((int)symtab->st_shndx) {
			case SHN_ABS:
				sentry.syment.n_scnum = N_ABS;
				break;
			case SHN_COMMON:
				sentry.syment.n_value = symtab->st_size;
				/* fall through */
			case SHN_UNDEF:
				sentry.syment.n_scnum = N_UNDEF;
			default:
				break;
			}

			return(&sentry.syment);

		} else {
			if (fread((char*)&sentry.syment,SYMESZ,1,stream) == 0)
				{
				error(ER49, bootmod, "symbol table");
				break;
				}

			if (number != NULL)
				*number = sequence;

			sequence++;
		
			if (DebugMode  &&  sentry.syment.n_numaux > 0  &&
			   sentry.syment.n_sclass == C_TPDEF) {
				if (fread(&auxentry, AUXESZ, 1, stream) == 0) {
					error(ER49, bootmod, "aux entry");
					break;
				}
				sequence += 1;
				sentry.syment.n_numaux--;
				sentry.syment.n_value =
					auxentry.x_sym.x_misc.x_lnsz.x_size;
			}

			if (sentry.syment.n_numaux > 0) {
				if (fseek(stream,
				   (long)sentry.syment.n_numaux*AUXESZ,1) == -1) {
					break;
				}
				sequence += sentry.syment.n_numaux;
			}

			if (sentry.syment.n_sclass == C_EXT   ||
			   sentry.syment.n_sclass == C_STAT  ||
			   (DebugMode  &&  (sentry.syment.n_sclass == C_TPDEF  ||
					    sentry.syment.n_sclass == C_MOS    ||
					    sentry.syment.n_sclass == C_MOU)))
			{
				if (sentry.syment.n_zeroes) {
					/*
					 * not a flexname, but if its exactly SYMNMLEN
					 * characters long it must be converted to a
					 * flexname so that it is null terminated
					 */

					if (sentry.syment.n_name[SYMNMLEN-1]) {
						/* convert to a flexname */
	
						strncat(strcpy(sentry.flexname,""),
							sentry.syment.n_name,
							SYMNMLEN);
						sentry.syment.n_zeroes = 0;
						sentry.syment.n_offset =
							sentry.flexname -
							sentry.syment.n_name;
					}
				} else {
					/*
					 * flexname
					 */

					register int c, n = sizeof(sentry.flexname);
					register char *p = sentry.flexname;

					if (fseek(string,stringorigin+sentry.syment.n_offset,0) == -1) {
						break;
					}

					while (--n >= 0 && (c = getc(string)) != EOF)
						{
						if ((*p++ = c) == '\0')
							break;
						}

					if (n < 0)
						/* flexname too long */
						error(ER5);

					if (c == EOF)
						{
						error(ER59);
						break;
						}

					sentry.syment.n_offset = sentry.flexname - sentry.syment.n_name;
				}

				return(&sentry.syment);
			}
		}
	}

end_of_job:

	if (is_elf) {
		elf_end (elfd);
		elfd = NULL;
	} else {
		fclose(stream);
		fclose(string);
		stream = NULL;
		string = NULL;
	}
	if (fd != -1) {
		close (fd);
		fd = -1;
	}
	return(NULL);
}
