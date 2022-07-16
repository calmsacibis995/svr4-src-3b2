/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:page.c	1.10.7.1"

/*
 * This file contains code for the crash functions: page, as, sdt, and ptbl.
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <signal.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/fs/s5dir.h>
#include <sys/sbd.h>
#include <sys/immu.h>
#include <vm/vm_hat.h>
#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>
#include <vm/page.h>
#include <sys/psw.h>
#include <sys/pcb.h>
#include <sys/user.h>
#include <sys/var.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/sgs.h>
#include "crash.h"

/* namelist symbol pointers */
extern struct syment *V;	/* ptr to var structure */
static struct syment *Pages;	/* ptr to start of page structure array */
static struct syment *Epages;	/* ptr to end of page structure array   */

static long pages = 0;
static long epages = 0;

extern char *sramapt[4];
extern SRAMB srambpt[4];
extern long vtop();
extern long lseek();

void prsegs();

/* get arguments for page function */
int
getpage()
{
	int slot = -1;
	int all = 0;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	unsigned  size;

	if(!Pages) {
		if(!(Pages = symsrch("pages")))
			error("pages not found in symbol table\n");
	}
	readmem((long)Pages->n_value, 1, -1, (char *)&pages, sizeof pages, "pages: ptr to page structures");

	if(!Epages) {
		if(!(Epages = symsrch("epages")))
			error("epages not found in symbol table\n");
	}
	readmem((long)Epages->n_value, 1, -1, (char *)&epages, sizeof epages, "epages: ptr to end of page structures");

	optind = 1;
	while((c = getopt(argcnt,args,"epw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}

	size = ((epages - pages) / sizeof(struct page)) + 1;

	fprintf(fp,"PAGE STRUCTURE TABLE SIZE: %d\n\n", size);
	fprintf(fp,"SLOT  KEEPCNT       VNODE        HASH        PREV      VPPREV  FLAGS\n");
	fprintf(fp,"          NIO      OFFSET                    NEXT      VPNEXT\n");
	if(args[optind]) {
		all = 1;
		do {
			getargs((int)size,&arg1,&arg2);
			if(arg1 == -1) 
				continue;
			if(arg2 != -1)
				for(slot = arg1; slot <= arg2; slot++)
					prpage(all,slot,phys,addr,size);
			else {
				if(arg1 < size)
					slot = arg1;
				else
					addr = arg1;
				prpage(all,slot,phys,addr,size);
			}
			slot = addr = arg1 = arg2 = -1;
		} while(args[++optind]);
	} else
		for(slot = 0; slot < size; slot++)
			prpage(all,slot,phys,addr,size);
}

/* print page structure table */
int
prpage(all,slot,phys,addr,size)
int all,slot,phys;
long addr;
unsigned size;
{

	long page;
	struct page pagebuf;
	long next, prev;
	long vpnext, vpprev;
	long hash;

	readbuf(addr,pages+slot*sizeof pagebuf,phys,-1,
		(char *)&pagebuf,sizeof pagebuf,"page structure table");

	/* check page flags */
	if ((*((ushort *)&pagebuf) == 0) && !all)
		return;

	if(slot == -1)
		fprintf(fp,"   -  ");
	else
		fprintf(fp,"%4d  ",slot);

	fprintf(fp,"%7d  0x%08x  ",
		pagebuf.p_keepcnt,
		pagebuf.p_vnode);

	/* calculate page structure entry number of pointers */

	hash = ((long)pagebuf.p_hash - pages)/sizeof(struct page);
	if ((hash < 0) || (hash > size))
		fprintf(fp,"0x%08x  ", pagebuf.p_hash);
	else
		fprintf(fp,"  %8d  ",hash);

	prev = ((long)pagebuf.p_prev - pages)/sizeof(struct page);
	if ((prev < 0) || (prev > size))
		fprintf(fp,"0x%08x  ", pagebuf.p_prev);
	else
		fprintf(fp,"  %8d  ",prev);

	vpprev = ((long)pagebuf.p_vpprev - pages)/sizeof(struct page);
	if ((vpprev < 0) || (vpprev > size))
		fprintf(fp,"0x%08x  ", pagebuf.p_vpprev);
	else
		fprintf(fp,"  %8d  ",vpprev);

	fprintf(fp,"%s%s%s%s%s%s%s%s%s%s\n",
		pagebuf.p_lock    ? "lock "    : "",
		pagebuf.p_want    ? "want "    : "",
		pagebuf.p_free    ? "free "    : "",
		pagebuf.p_intrans ? "intrans " : "",
		pagebuf.p_gone    ? "gone "    : "",
		pagebuf.p_mod     ? "mod "     : "",
		pagebuf.p_ref     ? "ref "     : "",
		pagebuf.p_pagein  ? "pagein "  : "",
		pagebuf.p_nc      ? "nc "      : "",
		pagebuf.p_age     ? "age "     : "");

	/* second line */

	fprintf(fp,"      %7d    %8d              ",
		pagebuf.p_nio,
		pagebuf.p_offset);

	next = ((long)pagebuf.p_next - pages)/sizeof(struct page);
	if ((next < 0) || (next > size))
		fprintf(fp,"0x%08x  ", pagebuf.p_next);
	else
		fprintf(fp,"  %8d  ",next);

	vpnext = ((long)pagebuf.p_vpnext - pages)/sizeof(struct page);
	if ((vpnext < 0) || (vpnext > size))
		fprintf(fp,"0x%08x  \n", pagebuf.p_vpnext);
	else
		fprintf(fp,"  %8d  \n",vpnext);
}

/* get arguments for sdt function */
int
getsdt()
{
	int proc = Procslot;
	int all = 0;
	int phys = 0;
	long addr = -1;
	int c;
	int section;
	int count = 1;
	struct proc prbuf;
	struct as asbuf;

	optind = 1;
	while((c = getopt(argcnt,args,"epw:s:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 's' :	proc = setproc();
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(!args[optind])
		longjmp(syn,0);
	if((section = strcon(args[optind++],'h')) == -1)
		error("\n");
	if(section > 3) {
		addr = (long)section;
		section = -1;
		if(args[optind]) 
			if((count = strcon(args[optind],'d')) == -1)
				error("\n");
	}
	procntry(proc,&prbuf);

	readmem((long)prbuf.p_as, 1, -1, (char *)&asbuf, sizeof asbuf, "as structure");
	prsdt(all,phys,section,addr,count,proc,&asbuf.a_hat);
}


/* print segment descriptor table */
int     
prsdt(all,phys,section,addr,count,proc,hatp)
int all,phys,section,count,proc;
long addr;
struct hat *hatp;
{
	long sdtaddr;
	sde_t segbuf;
	int len,i;

	if(section > -1) {
		/* determine address and length of table by section */
		switch(section) {
			case 0:	/* kernel section 0 */
				sdtaddr = (long)sramapt[0];
				len = srambpt[0].SDTlen;
				break;
			case 1:	/* kernel section 1 */
				sdtaddr = (long)sramapt[1];
				len = srambpt[1].SDTlen;
				break;
			case 2:	/* text, data and bss section */
				sdtaddr = hatp->hat_srama[0];
				len = hatp->hat_sramb[0].SDTlen;
				break;
			case 3:	/* shared memory, stack and ublock */
				sdtaddr = hatp->hat_srama[1];
				len = hatp->hat_sramb[1].SDTlen;
				break;
		}

		fprintf(fp, "SDT: start %x length %d\n\n", sdtaddr, len);
	} else {
		if(phys || !Virtmode) {
			sdtaddr = addr;
			len = count;
		} else {
			sdtaddr = vtop(addr,proc);
			if(sdtaddr == -1)
				error("%x is an invalid address\n",addr);
			sdtaddr |= MAINSTORE;
			len = count;
		}
	}

	fprintf(fp,"SLOT SEG KPROT UPROT LENGTH  ADDRESS FLAGS\n");

	if(lseek(mem, sdtaddr - MAINSTORE, 0) == -1)
		error("seek error on segment descriptor table address\n");
	for(i = 0; i <= len; i++) {
		if(read(mem,&segbuf,sizeof segbuf) != sizeof segbuf) 
			error("read error on segment descriptor table\n");
		if(!(segbuf.seg_flags & SDE_V_bit) && !all)
			continue;
		fprintf(fp,"%4d ",i);
		if(SD_ISCONTIG(&segbuf))
			fprintf(fp,"con ");
		else
			fprintf(fp,"pt  ");
		switch(segbuf.seg_prot & KRWE) {
			case KNONE: fprintf(fp,"knone"); break;
			case KEO: fprintf(fp,"keo  "); break;
			case KRE: fprintf(fp,"kre  "); break;
			case KRWE: fprintf(fp,"krwe "); break;
			default: fprintf(fp,"  -  "); break;
		}
		switch(segbuf.seg_prot & URWE) {
			case UNONE: fprintf(fp," unone"); break;
			case UEO: fprintf(fp,"   ueo"); break;
			case URE: fprintf(fp,"   ure"); break;
			case URWE: fprintf(fp,"  urwe"); break;
			default: fprintf(fp,"   -  "); break;
		}
		fprintf(fp," %6u %8x %s%s%s%s%s\n",
			segbuf.seg_len * 8,
			segbuf.wd2.address,
			segbuf.seg_flags & SDE_I_bit ? "i " : "",
			segbuf.seg_flags & SDE_V_bit ? "v " : "",
			segbuf.seg_flags & SDE_T_bit ? "t " : "",
			segbuf.seg_flags & SDE_C_bit ? "c " : "",
			segbuf.seg_flags & SDE_P_bit ? "p " : "");
	}
}

/* get arguments for ptbl function */
int
getptbl()
{
	int proc = Procslot;
	int all = 0;
	int phys = 0;
	long addr = -1;
	int c;
	struct proc prbuf;
	struct as asbuf;
	int section;
	int segment;
	int count = 1;

	optind = 1;
	while((c = getopt(argcnt,args,"epw:s:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 's' :	proc = setproc();
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(!args[optind])
		longjmp(syn,0);
	if((section = strcon(args[optind++],'h')) == -1)
		error("\n");
	if(section > 3) {
		addr = (long)section;
		segment = section = -1;
		if(args[optind]) 
			if((count = strcon(args[optind],'d')) == -1)
				error("\n");
	}
	else {
		if(!args[optind])
			longjmp(syn,0);
		if((segment = strcon(args[optind],'d')) == -1)
			error("\n");
	}
	procntry(proc,&prbuf);
	readmem((long)prbuf.p_as, 1, -1, (char *)&asbuf, sizeof asbuf, "as");
	prptbl(all,phys,section,segment,addr,count,proc,&asbuf.a_hat);
}

/* print page table */
int
prptbl(all,phys,section,segment,addr,count,proc,hatp)
int all,phys,section,segment,count,proc;
long addr;
struct hat *hatp;
{
	long  plen,paddr;
	pte_t ptebuf;
	long  sdtaddr,len;
	sde_t sdtbuf;
	int i;

	if(section > -1) {
		/* determine address and length by section */
		switch(section) {
			case 0:	/* kernel section 0 */
				sdtaddr = (long)sramapt[0];
				len = srambpt[0].SDTlen;
				break;
			case 1:	/* kernel section 1 */
				sdtaddr = (long)sramapt[1];
				len = srambpt[1].SDTlen;
				break;
			case 2:	/* text, data and bss section */
				sdtaddr = hatp->hat_srama[0];
				len = hatp->hat_sramb[0].SDTlen;
				break;
			case 3:	/* shared memory, stack and ublock */
				sdtaddr = hatp->hat_srama[1];
				len = hatp->hat_sramb[1].SDTlen;
				break;
		}
		fprintf(fp, "SDT: start %x length %d\n\n", sdtaddr, len);

		if(segment < 0 || segment > len) 
			error("segment number out of range\n");

		readmem((long)((sdtaddr+segment*sizeof sdtbuf) - MAINSTORE),
			0,proc,(char *)&sdtbuf,sizeof sdtbuf,
			"segment descriptor table");

		if(SD_ISCONTIG(&sdtbuf)) 
			error("contiguous segment - not valid in page table\n");
		/* locate page table */
		paddr = sdtbuf.wd2.address & ~0x7;
		plen = btopt(sdtbuf.seg_len);
	}
	else {
		if(phys || !Virtmode) {
			paddr = addr;
			plen = count;
		} else {
			paddr = vtop(addr,proc);
			if(paddr == -1)
				error("%x is an invalid address\n",addr);
			paddr |= MAINSTORE;
			plen = count;
		}
	}

	fprintf(fp,"SLOT    PFN   TAG   FLAGS\n");

	if(lseek(mem,paddr - MAINSTORE, 0) == -1)
		error("seek error on page table address\n");
	for(i = 0; i < plen; i++) {
		if(read(mem,&ptebuf, sizeof ptebuf) != sizeof ptebuf) 
			error("read error on page table\n");
		if(!ptebuf.pgm.pg_pfn && !all)
			continue;
		fprintf(fp,"%4d %6x   %3x   %s%s%s%s%s\n",
			i,
			ptebuf.pgm.pg_pfn,
			ptebuf.pgm.pg_tag,
			ptebuf.pgm.pg_ref   ? "ref "   : "",	
			ptebuf.pgm.pg_w     ? "w "     : "",	
			ptebuf.pgm.pg_last  ? "last "  : "",	
			ptebuf.pgm.pg_mod   ? "mod "   : "",	
			ptebuf.pgm.pg_v     ? "v "     : "");	
	}
}

/* get arguments for as function */
int
getas()
{
	struct var varbuf;
	int slot = -1;
	int proc = -1;
	int full = 0;
	int all = 0;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	char *heading = "PROC  KEEPCNT        SEGS     SEGLAST  MEM_CLAIM    SRAMA[2]    SRAMB[2]  FLAGS\nSLOT                                                SRAMA[3]    SRAMB[3]\n";

	optind = 1;
	while((c = getopt(argcnt,args,"efpw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}

	if (!full)
		fprintf(fp,"%s",heading);

	if(args[optind]) {
		do {
			getargs(vbuf.v_proc,&arg1,&arg2);
			if(arg1 == -1) 
				continue;
			if(arg2 != -1)
				for(proc = arg1; proc <= arg2; proc++)
					pras(all,full,proc,phys,addr,heading);
			else
				pras(all,full,arg1,phys,addr,heading);
			proc = arg1 = arg2 = -1;
		} while(args[++optind]);
	} else if (all) {
		readmem((long)V->n_value,1,-1,(char *)&vbuf,
			sizeof vbuf,"var structure");
		for(proc = 0; proc < vbuf.v_proc; proc++) 
			pras(all,full,proc,phys,addr,heading);
	} else
		pras(all,full,proc,phys,addr,heading);
}


/* print address space structure */
int
pras(all,full,slot,phys,addr,heading)
int all,full,slot,phys;
char *heading;
{
	struct proc prbuf, *procaddr;
	struct as asbuf;
	proc_t *slot_to_proc();

	procaddr = slot_to_proc(slot);

	if (procaddr) {
		readmem((long)procaddr,1, -1,(char *)&prbuf,sizeof prbuf,
		    "proc table");
	} else {
		return;
	}

	if (full)
		fprintf(fp,"\n%s",heading);

	fprintf(fp, "%4d  ", slot);

	if (prbuf.p_as == NULL) {
		fprintf(fp, "- no address space.\n");
		return;
	}

	readbuf(addr,(long)(prbuf.p_as),phys,-1,
		(char *)&asbuf,sizeof asbuf,"as structure");

	fprintf(fp,"%7d  0x%08x  0x%08x  %9d  0x%08x  0x%08x  %s%s\n",
		asbuf.a_keepcnt,
		asbuf.a_segs,
		asbuf.a_seglast,
		asbuf.a_rss,
		asbuf.a_hat.hat_srama[0],
		asbuf.a_hat.hat_sramb[0],
		(asbuf.a_lock == 0) ? "" : "lock " ,
		(asbuf.a_want == 0) ? "" : "want " );
	fprintf(fp,"                                                  0x%08x  0x%08x\n",
		asbuf.a_hat.hat_srama[1],
		asbuf.a_hat.hat_sramb[1]);

	if (full) { 
		prsegs(prbuf.p_as, (struct as *)&asbuf, phys, addr);
	}
}


/* print list of seg structures */
void
prsegs(as, asbuf, phys, addr)
	struct as *as, *asbuf;
	long phys, addr;
{
	struct seg *seg, *sseg;
	struct seg  segbuf;
	struct syment *sp;
	extern char * strtbl;
	extern struct syment *findsym();

	sseg = seg = asbuf->a_segs;

	if (seg == NULL)
		return;

	fprintf(fp, "      LOCK        BASE     SIZE        NEXT       PREV          OPS        DATA\n");

	do {
		readbuf(addr, seg, phys, -1, (char *)&segbuf, sizeof segbuf,
			"seg structure");
		fprintf(fp, "      %4d  0x%08x %8d  0x%08x 0x%08x ",
			segbuf.s_lock,
			segbuf.s_base,
			segbuf.s_size,
			segbuf.s_next,
			segbuf.s_prev);

		/* Try to find a symbolic name for the sops vector. If
		 * can't find one print the hex address.
		 */
		sp = findsym((unsigned long)segbuf.s_ops);
		if ((!sp) || ((unsigned long)segbuf.s_ops != sp->n_value))
			fprintf(fp,"0x%08x  ", segbuf.s_ops);
		else if (sp->n_zeroes) 
			fprintf(fp, "  %8.8s  ", sp->n_name);
		else
			fprintf(fp, "%12.12s  ", strtbl+sp->n_offset);

		fprintf(fp,"0x%08x\n", segbuf.s_data);

		if (segbuf.s_as != as) {
			fprintf(fp, "WARNING - seg was not pointing to the correct as struct: 0x%8x\n",
				segbuf.s_as);
			fprintf(fp, "          seg list traversal aborted.\n");
			return;
		}
	} while((seg = segbuf.s_next) != sseg);
}
