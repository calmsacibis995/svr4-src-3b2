/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:fnode.c	1.2.2.1"
/*
 * This file contains code for the crash functions:  fnode.
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/var.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/conf.h>
#include <sys/fs/snode.h>
#include <sys/fs/fifonode.h>
#include "crash.h"

extern struct syment *Vfs, *Vfssw, *File;	/* namelist symbol pointers */ 
struct syment *Snode;

struct fifonode fnbuf;			/* buffer for fnode */


/* get arguments for fnode function */
int
getfnode()
{
	int slot = -1;
	int full = 0;
	int all = 0;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	char *heading = "MAJ/MIN  REALVP     COMMONVP LASTR  SIZE    COUNT FLAGS\n";


	if(!Snode)
		if(!(Snode = symsrch("stable")))
			error("snode table not found in symbol table\n");
	optind = 1;
	while((c = getopt(argcnt,args,"fpw:")) !=EOF) {
		switch(c) {
			case 'f' :	full = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}

	if(!full)
		fprintf(fp,"%s",heading);
	if(args[optind]) {
		do{
			if((addr = strcon(args[optind++], 'h')) == -1)
				error("\n");
			prfnode(full,phys,addr,heading);
		}while(args[optind]);
	}
	else longjmp(syn,0);
}



/* print fnode table */
int
prfnode(full,phys,addr,heading)
int full,phys;
long addr;
char *heading;
{
	struct fifonode *fnp, fnbuf;
	struct snode *snp;
	extern long lseek();

	readbuf(addr,(long)fnp,phys,-1,(char *)&fnbuf,sizeof fnbuf,"fifonode");
	snp = &fnbuf.fn_snode;

	if(full)
		fprintf(fp,"%s",heading);

	fprintf(fp," %4u,%-5u %8x  %8x %4d %5d    %5d ",
		getemajor(snp->s_dev),
		geteminor(snp->s_dev),
		snp->s_realvp,
		snp->s_commonvp,
		snp->s_lastr,
		snp->s_size,
		snp->s_count);

	fprintf(fp,"%s%s%s%s%s\n",
		snp->s_flag & SLOCKED ? " lk" : "",
		snp->s_flag & SUPD ? " up" : "",
		snp->s_flag & SACC ? " ac" : "",
		snp->s_flag & SWANT ? " wt" : "",
		snp->s_flag & SCHG ? " ch" : "");

	fprintf(fp, "fn_size: %d, fn_ino: %d, fn_wcnt: %d, fn_rcnt: %d\n",
		fnbuf.fn_size,
		fnbuf.fn_ino,
		fnbuf.fn_wcnt,
		fnbuf.fn_rcnt);
	fprintf(fp, "fn_wptr: %d, fn_rptr: %d, fn_nbuf: %d, ",
		fnbuf.fn_wptr,
		fnbuf.fn_rptr,
		fnbuf.fn_nbuf);
	fprintf(fp, "fn_flag: %s%s\n",
		fnbuf.fn_flag & FIFO_RBLK ? " rblk" : "",
		fnbuf.fn_flag & FIFO_WBLK ? " wblk" : "");
	
	if(full)
	{
		/* print vnode info */
		fprintf(fp,"VNODE :\n");
		fprintf(fp,"VCNT VFSMNTED   VFSP   STREAMP VTYPE   RDEV    VDATA   VFILOCKS VFLAG     \n");
		prvnode(&snp->s_vnode);
		fprintf(fp,"\n");
	}

}


