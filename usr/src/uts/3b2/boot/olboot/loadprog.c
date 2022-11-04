/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/olboot/loadprog.c	11.13"

#include	"sys/param.h"
#include	"sys/types.h"
#include	"sys/inode.h"
#include	"sys/firmware.h"
#include	"sys/fsiboot.h"
#include	"sys/lboot.h"
#include	"sys/psw.h"
#include	"sys/boot.h"
#include	"sys/edt.h"
#include	"sys/extbus.h"
#include	"sys/sys3b.h"
#include	"sys/sbd.h"
#include	"sys/csr.h"
#include	"sys/immu.h"
#include	"sys/nvram.h"
#include	"a.out.h"
#include	"sys/elftypes.h"
#include	"sys/elf.h"


/*
**	loadprog - Load program into memory.
**
**	This routine loads the program, indicated by its inode, into memory
**	at the address in the file header, if:
**	  1. it is a plain, executable file
**	  2. sizes and addresses of text, data, and bss don't overlap or overlay
**		lboot
**	  3. no errors occur on the load
**
**	Return code:  LDPASS if successfully loaded the program;
**		      LDFAIL if failed to load for the reasons mentioned.
*/
extern int xedtsect;
struct s3bconf sys3bconfig[120];
char xdev = 0;
static void confirm();
static void xconfirm();
static void edtscan();
extern char bfname[];
extern unsigned short	rb();
int iscoff = 1;
Elf32_Ehdr elfhdr;
Elf32_Phdr elfphdr;
Elf32_Shdr elfshdr;

unsigned short
nloadprog(finode)
struct inode finode;	/* boot program inode */
{
	register int		c,	/* byte count */
				*mp,	/* ptr to memory being loaded */
				tc,	/* byte transfer count for this block */
				*bp;	/* ptr to buffer being copied */
	short			*xmp,	/* ptr to memory being loaded if x86 */
				*xbp;	/* ptr to buffer being copied if x86 */
	register struct scnhdr	*shp;	/* ptr to section header */
	register struct filehdr	*fhp;	/* ptr to file header */
	int			i,	/* loop count */
				nsect,	/* number of sections to check-load */
				fa,	/* file byte address */
				s,
				fndresp;/* return code from findfile */
	extern bootstartaddr;
	extern struct inode muinode;

	if (!(finode.i_nlink)) {

		/* Unallocated inode. */
		PRINTF("File link count = 0.\n");
		return(LDFAIL);
	}
	if ((finode.i_mode & IFMT) != IFREG) {

		/* Not regular file. */
		PRINTF("Not a regular file.\n");
		return(LDFAIL);
	}
	if (!(finode.i_size)) {

		/* Zero-length file. */
		PRINTF("File size = 0.\n");
		return(LDFAIL);
	}

	/* Read file and section headers and verify them. */
	if (!rb(AHDR, 0, &finode))
		return(LDFAIL);
	fhp = (struct filehdr *)(AHDR);
	if ((fhp->f_magic != FBOMAGIC) && (fhp->f_magic != X86MAGIC) && (fhp->f_magic != 0x7f45)) {

		/* If not executable call mUNIX to configure using the
		   file as the system file (unless this IS mUNIX)  */

		if (finode.i_number == muinode.i_number)
		{
			PRINTF("Configuration program not executable!\n");
			return(LDFAIL);
		}

		fndresp=findfile(LBOOT);
		if (fndresp == NOTFOUND)
		{
			PRINTF("Configuration program not found!\n");
			return(LDFAIL);
		}

		WNVRAM(bfname, (char *)(UNX_NVR->bootname), 20);
		muinode = Fndinode;
		return(nloadprog(muinode));	/* Call recursively */
	}

	if (!(finode.i_mode & 0111))
	{
		/* File is an executable, but is not executable */
		PRINTF("Program not executable.\n");
		return(LDFAIL);
	}

	/* COFF or ELF */
	if (fhp->f_magic == 0x7f45)
		iscoff = 0;

	/* LOAD file sections */
 	bootstartaddr = 0x7fffffff;

	if (iscoff) {
	    /* is COFF */
    	    nsect = fhp->f_nscns;	/* number of sections to load */
    	    fa = sizeof(struct filehdr)+fhp->f_opthdr+fhp->f_nscns * sizeof(struct scnhdr);
    	    shp = (struct scnhdr *) (((long)(fhp +1)) + fhp->f_opthdr);
    
    	    if (fhp->f_magic == FBOMAGIC) {
    	        for (nsect = fhp->f_nscns; nsect > 0; nsect--, shp++) {
    		    /*PRINTF("fhp = %x, shp = %x, fa = %x, nsec = %x\n",fhp,shp,fa,nsect);*/
    		    /*PRINTF("Section %s, paddr %x, size %x, flags %x\n",shp->s_name,shp->s_paddr,shp->s_size,shp->s_flags);*/
    
    
    		    if ((shp->s_flags & STYP_NOLOAD) != 0 || (shp->s_scnptr == 0))
            	            continue;
    
    		    if ((shp->s_flags & STYP_INFO) != 0)
    			    continue;
    
    		    mp = (int *)shp->s_paddr;	/* physical mem load address */
     		    if (((int)mp < bootstartaddr) && ((int)mp > 0x2000000))
     			    bootstartaddr = (int)mp;
    
    		    for (c = shp->s_size; c > 0; c -= tc, fa += tc) {
    			    tc = FsBSIZE(fstype) - (fa % FsBSIZE(fstype));	/* transfer count */
    			    bp = (int *)(DATA + FsBSIZE(fstype) - tc);
    			    if (!rb(DATA, fa, &finode))
    				    return(LDFAIL);
    			    if (tc > c)
    				    tc = c;
    			    for (i = tc / NBPW; i--; *mp++ = *bp++);
    		    }
    
    		    if ((STRCMP(shp->s_name, "<EDT>")) == 0)
    		    {
    			    s = do_edt();
    
    			    if (s)
    				    return (3);
    		    }
    
    	        }
    	    }
    	    else {
    	        for (nsect = fhp->f_nscns; nsect > 0; nsect--, shp++) {
    		    if ((shp->s_flags & STYP_NOLOAD) != 0 || (shp->s_scnptr == 0)) {
                            continue;
    		    }
    		    if ((shp->s_flags & STYP_INFO) != 0)
    			    continue;
    
    		    xmp = (short *)shp->s_paddr;	/* physical mem load address */
     		    if ((int)xmp < bootstartaddr)
     			    bootstartaddr = (int)xmp;
    
    		    for (c = shp->s_size; c > 0; c -= tc, fa += tc) {
    		        tc = FsBSIZE(fstype) - (fa % FsBSIZE(fstype));	/* transfer count */
    		        xbp = (short *)(DATA + FsBSIZE(fstype) - tc);
    		        if (!rb(DATA, fa, &finode))
    			    return(3);
    		        if (tc > c)
    			    tc = c;
    		        for (i = tc / 2; i--; *xmp++ = *xbp++);
    		    }
    	        }
    	    }
	} else {
	    /* is ELF */
	    if (!eread(&finode, 0, &elfhdr, sizeof(elfhdr)))
		return(LDFAIL);

	    for (nsect = 0; nsect < (int)elfhdr.e_phnum; nsect++) {
		eread(&finode, elfhdr.e_phoff + (elfhdr.e_phentsize * nsect), &elfphdr, sizeof(elfphdr));
		if (elfphdr.p_type == PT_LOAD) {
		    fa = elfphdr.p_offset;
		    mp = (int *)elfphdr.p_paddr;
		    if (((int)mp < bootstartaddr) && ((int)mp > 0x2000000))
			bootstartaddr = (int)mp;
		    eread(&finode, fa, mp, elfphdr.p_filesz);
		}
	    }

	}

	/* PRINTF("Load Complete!\n"); */
	return(LDPASS);
}

/* ALL The following code is required for AUTO-CONFIGURATION, and is
			 taken from LBOOT */

do_edt()
{
	register int i;
	struct s3bc *s3bc;

	edtscan(EDT_START, 0, confirm);
	if (xdev)
		xconfirm();

	for (i=0, s3bc=sys3bconfig->driver; i<sys3bconfig->count;++i, ++s3bc)
	{
		char buffer[40];


		if (!strcmp(s3bc->name, "SBD"))
		{
			if (s3bc->timestamp > SIZOFMEM)
			{
				PRINTF("%s configured for too much memory.\n",
					P_CMDQ->b_name);
				restart();
			}

			if (s3bc->timestamp < SIZOFMEM)
			{
				PRINTF("%s configured for too little memory.\n",
					P_CMDQ->b_name);
				restart();
			}
		}

		if (s3bc->flag & S3BC_IGN || s3bc->board == 0)
		/* device is ignored, it exists now, or this is a software driver */
			continue;

		strncat(strcpy(buffer,""), s3bc->name, sizeof(s3bc->name));

		if (s3bc->board & 0xF0 && !(s3bc->flag & S3BC_TCDRV))
		{
			PRINTF("Device %s previously configured on LBE (board code %d",
				buffer, ((unsigned int)(s3bc->board&0xf0)) >> 4);
			PRINTF(") at ELB board code %d\n", s3bc->board&0x0f);
			PRINTF("A new unix will be re-configured\n");
			return (1);
		}

		else
		{
			PRINTF("Device %s previously configured at board code %d\n",
					buffer, s3bc->board);
			PRINTF("A new unix will be re-configured\n");
			return (1);
		}

	}

	return (0);
}

static void
edtscan(base, lba, function)
register struct edt *base;
register int lba;
register int (*function)();
{
	register struct edt *edtp;
	register i;
	int elb;

	for (i=0, edtp=base; i < NUM_EDT; ++i)
	{
		elb = edtp->opt_slot;

		(*function)(edtp++, lba, elb);
	}
}

 static
 void
confirm(edtp, lba, elb)
register struct edt *edtp;
int lba;
int elb;
{
	register struct s3bc *s3bc;
	register i;

	/*
	 * find the matching device for this EDT entry
	 */
	for (i=0; i<sys3bconfig->count; ++i)
	{
		s3bc = &sys3bconfig->driver[i];

		if (0 != STRCMP(edtp->dev_name,s3bc->name))
			continue;

		if (s3bc->flag & S3BC_IGN)
			/* ignored before, therefore ignore it now */
			return;

		if (s3bc->board == ((lba<<4) | elb))
			/* found it, therefore mark it found */
			{
			s3bc->board = 0;
			if(edtp->indir_dev)
				/* load bus gen program for configured
				 * HA device.
				 */
				xdev++;
			return;
			}
		}

}

/*
 * xconfirm()
 *
 * Confirm that TC devices that exist now were configured at the time the
 * sys3bconfig structure was created
 */
static
void
xconfirm()
{

	register struct s3bc *s3bc;
	char *mem,*xbuf;
	long sect,size;
	register i,j,k,l;
	B_TC *xtcp;
	B_EDT *xedtp;
	long *sanity,*xhead;



	/*
         * We know we always transfer whole sectors. No need to check for
	 * misaligned read.
	 */

	mem = (char *)0x02188000; /* This should be a sector aligned address */
			          /* Enough space left for 16K of bss        */
	xbuf = mem;
	sect = xedtsect;
	size = XEDTSIZE * BSIZE;

	while (size){
		ndisk_acs(sect, mem);
		size -= BSIZE;
		sect++;
		mem += BSIZE;
	}

	sanity = (long *)xbuf;
	xhead = (long *)xbuf + 1;
 	if (*sanity != XEDTMAGIC) /* no extended EDT on disk */
	{
		return;
	}

	l=k=0;
	for (j=0; j< 15; j++){
		if (xhead[j] == (long)0)
			return;
		else	{
		
			xedtp = (B_EDT *)(xbuf + xhead[j] - XEDTSTART);

			xtcp = xedtp->tc;

			while (k < (int)xedtp->n_tcs && l < (int)xedtp->max_tc){
			   l++;
			   if (xtcp->equip_stat){
				k++;
                		/*
                 		* find the matching device for this XEDT entry
                 		*/
                		for (i=0; i<sys3bconfig->count; ++i){
					s3bc = &sys3bconfig->driver[i];
					if (0 != strcmp(xtcp->name,s3bc->name)
					     || s3bc->flag & S3BC_IGN)
						continue;
		
					if (s3bc->board == xtcp->maj){
                                	/* found it, therefore mark it found */
		
                                		s3bc->board = 0;
                                		break;
					}
				}
			   }
			xtcp = (B_TC *)((caddr_t)xtcp + xedtp->tc_size);
			}
		}
	}
}
