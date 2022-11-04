/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/fw_pump.c	1.3"

# include	"sys/types.h"
# include	"sys/inode.h"
# include	"a.out.h"
# include	"sys/firmware.h"
# include	"sys/param.h"
# include	"sys/lboot.h"
# include	"sys/boot.h"
# include	"sys/sbd.h"
# include 	"sys/csr.h"
# include	"sys/cio_defs.h"
# include	"sys/diagnostic.h"
# include	"sys/iobd.h"

extern char	IOBASE[];	/* base io buffer */
extern char	IND3[];		/* 3rd level indirect block */
extern char	AHDR[];		/* a.out header block for UNIX */
extern char	IND2[];		/* 2nd level indirect block */
extern char	IND1[];		/* 1st level indirect block */
extern char	INODE[];	/* inode block */
extern char	DIR[];		/* directory block */

extern struct inode	Dinode;		/* Inode of file system root direct */
extern struct inode	Fndinode;	/* Inode of file found by findfile() */
extern struct inode	Linode;		/* Inode of last directory found */
extern int		Fso;		/* File system offset */
extern int		fstype;		/* File system type flag */
unsigned long	old_ipl;


fw_pump(slot,filenm)
unsigned long slot;
char * filenm;
{
	struct  inode	binode;		/* inode of boot program */
	int	fndresp;		/* Return code from findfile() */
	register int		c,	/* byte count */
				tc;	/* byte transfer count for this block */
	register struct scnhdr	*shp;	/* ptr to section header */
	register struct filehdr	*fhp;	/* ptr to file header */
	struct aouthdr *ohp;		/* ptr to optional header */
	int			i,	/* loop count */
				nsect,	/* number of sections to check-load */
				fa;	/* disk file byte address */
	short			j;	/* loop count */
	unsigned long		bp;	/* main memory addr of loaded prog */
	unsigned long		strtaddr;  /* peripheral xeq address */

	struct Tsection {	/* Structure for sections to be loaded */
	    short Tflag;		/*  0=Loaded in periph, 1=not yet  */
	    unsigned short Taddr;	/*  Peripheral load address  */
	    unsigned short Tsize;	/*  Bytecount of section  */
	    unsigned long Tmmaddr;	/*  Main mem address  */
	} Tsection[20];

	unsigned short Totalsize;	/* Total bytecnt of sctns to load */
	short nentries;			/* # sections to load */
	short index;			/* current index to Tsection */
	unsigned long Tcurrent;

	long			(*OLDINTHAND)();  /* save old int handler */
	extern unsigned short	rb();	/* read disk block */
	extern int		comioxmt();	/* DLM code to peripheral */
	extern short		comioxeq();	/* xeq code on peripheral */
	extern long		ihandler();	/* int handler */

#ifdef	DEBUG
char answ[50];
#endif

    OLDINTHAND = INT_HAND;  /*  Save the old interrupt handler  */
    INT_HAND=ihandler;      /*  Use our interrupt handler  */
    asm ("	EXTFW	&3, &13, %psw, old_ipl");  /*  Save old IPL  */
#ifdef	DEBUG
    PRINTF("FW_PUMP called.  slot=%d,  filename=%s\n",slot,filenm);
PRINTF("FLOPPY?  ");
GETS(answ);
if (answ[0] == 'y') P_CMDQ->b_dev = FLOPDISK;
#endif

	fndresp = findfile(filenm);  /*  Search for file  */

	if (fndresp == NOTFOUND) {  /*  Not there  */
		PRINTF("File not found\n");
		INT_HAND = OLDINTHAND;
    		asm ("	INSFW	&3, &13, old_ipl, %psw");
		return (FAIL);		/* Return to firmware */
	}
	else if (fndresp == DIRFOUND) {	     /* Request was directory */
		PRINTF("File is a directory\n");
		INT_HAND = OLDINTHAND;
    		asm ("	INSFW	&3, &13, old_ipl, %psw");
		return (FAIL);		/* Return to firmware */
	}

	binode = Fndinode;

	/* Check that this file is ok to be pumped.  */

	if(!(binode.i_nlink)) {

		/* Unallocated inode. */
		PRINTF("File link count = 0.\n");
		INT_HAND = OLDINTHAND;
    		asm ("	INSFW	&3, &13, old_ipl, %psw");
		return (FAIL);		/* Return to firmware */
	}
	if((binode.i_mode & IFMT) != IFREG) {

		/* Not regular file. */
		PRINTF("Not a regular file.\n");
		INT_HAND = OLDINTHAND;
    		asm ("	INSFW	&3, &13, old_ipl, %psw");
		return (FAIL);		/* Return to firmware */
	}
	if(!(binode.i_mode & 0111)) {

		/* Not executable. */
		PRINTF("File not executable.\n");
		INT_HAND = OLDINTHAND;
    		asm ("	INSFW	&3, &13, old_ipl, %psw");
		return (FAIL);		/* Return to firmware */
	}
	if(!(binode.i_size)) {

		/* Zero-length file. */
		PRINTF("File size = 0.\n");
		INT_HAND = OLDINTHAND;
    		asm ("	INSFW	&3, &13, old_ipl, %psw");
		return (FAIL);		/* Return to firmware */
	}

	/*  Now setup the common I/O board */

	IO_SMART(slot)->reset = 1;	/* reset the board - to be safe */
	HWCNTR(1);			/* wait ~10 msec for reset */
	if (FW_SYSGEN(slot) != PASS) {  /*  Sysgen failed  */
		INT_HAND = OLDINTHAND;
    		asm ("	INSFW	&3, &13, old_ipl, %psw");
		return (FAIL);		/* Return to firmware */
	}

	/* Read file and section headers and verify them. */

	if (!rb(AHDR, 0, &binode)) {
		INT_HAND = OLDINTHAND;
    		asm ("	INSFW	&3, &13, old_ipl, %psw");
		return (FAIL);		/* Return to firmware */
	}
	fhp = (struct filehdr *)(AHDR);
	if((fhp->f_magic != FBOMAGIC) && (fhp->f_magic != X86MAGIC) &&
		(fhp->f_magic != B16MAGIC)) {

		/* Not MAC32, B16, or X86 format. */
		PRINTF("Warning:  Not MAC32, B16, or X86 magic.\n");
	}

	/*  Set up load table Tsection from header information  */

	nsect = fhp->f_nscns;	/* number of sections to load */
	fa = sizeof(struct filehdr)+fhp->f_opthdr+fhp->f_nscns * sizeof(struct scnhdr);
	shp = (struct scnhdr *) (((long)(fhp +1)) + fhp->f_opthdr);
	strtaddr = (unsigned long) shp->s_paddr; /*  XEQ ADDR  */
	if (fhp->f_opthdr > 0) {
	    ohp = (struct aouthdr *) ((long)(fhp +1));
	    strtaddr = ohp->entry;
	}

	nentries = 0;
	Totalsize = 0;
	for (nsect = fhp->f_nscns; nsect > 0; nsect-- , shp++) {
#ifdef	DEBUG
	    PRINTF("fhp = 0x%x, shp = 0x%x, fa = 0x%x, nsec = 0x%x\n",fhp,shp,fa,nsect);
	    PRINTF("Section %s, paddr 0x%x, size 0x%x, flags 0x%x\n",shp->s_name,shp->s_paddr,shp->s_size,shp->s_flags);
#endif
	    if (strcmp(shp->s_name,".start") == 0)
		strtaddr = shp->s_paddr; /* Set start address to .start */
	    if((shp->s_flags & STYP_NOLOAD) != 0 || (shp->s_scnptr == 0)) {
                continue;
	    }
	    Tsection[nentries].Taddr = shp->s_paddr;
	    Tsection[nentries].Tflag = 1;  /*  Section not yet downloaded  */
	    Tsection[nentries].Tsize = shp->s_size;
	    Tsection[nentries].Tmmaddr = fa + Totalsize + DOWNADDR;
	    Totalsize += shp->s_size;
	    nentries++;
	}

#ifdef	DEBUG
	PRINTF("Totalsize=0x%x  nentries=%d\n",Totalsize,nentries);
	PRINTF("0 = 0x%x, 1=0x%x\n",Tsection[0].Taddr,Tsection[1].Taddr);
#endif
	bp = DOWNADDR;  /*  Load code here in main memory  */
	tc = FsBSIZE(fstype);

	/*  Load code (all sections) from file (but not symbol table, etc.)  */

	for(c = Totalsize; c > 0; c -= tc, fa += tc, bp += tc) { 
		if (!rb((char *) bp, fa, &binode)) {
			PRINTF("READ FAILED\n");
			INT_HAND = OLDINTHAND;
    			asm ("	INSFW	&3, &13, old_ipl, %psw");
			return (FAIL);		/* Return to firmware */
		}
	}

	/*
	    This section sorts through the Tsection structure (order n*n)
	    pulling out the section with the highest address, downloading
	    that section to the peripheral, and then removing that
	    section from consideration in Tsection.  This is done
	    because download addresses are 256 byte page addresses
	    and in order to get all memory in a particular section down,
	    I may have to download up to the previous 255 bytes.
	    Downloaded in the wrong order, this could corrupt
	    already downloaded code/data.
	*/

	Tcurrent = 0;
	index = 9999;
	for (i=0; i < nentries; i++) {  /*  make nentries passes  */
	    for (j=0; j < nentries; j++)  /* search all sections each pass */
		if ((Tsection[j].Tflag) && (Tsection[j].Taddr > Tcurrent)) {
		    /* Highest addr section yet */
		    Tcurrent = Tsection[j].Taddr;
		    index = j;
		}
	    if (index < 9999) {  /*  Download this section  */
	        if (comioxmt(slot, Tsection[index].Tmmaddr,
	            Tsection[index].Taddr, Tsection[index].Tsize) != PASS) {
			INT_HAND = OLDINTHAND;
    			asm ("	INSFW	&3, &13, old_ipl, %psw");
			return (FAIL);		/* Return to firmware */
		}
	        Tsection[index].Tflag = 0;  /*  This section now sent  */
		Tcurrent = 0;
		index = 9999;
	    }
	}

	comioxeq(slot, strtaddr);  /*  Tell peripheral to XEQ the code  */
	FW_SYSGEN(slot);	/*  Finally, resysgen the peripheral  */

#ifdef	DEBUG
	PRINTF("Load Complete!\n");
#endif
    /*  Restore old interrupt handler and interrupt priority  */
    INT_HAND = OLDINTHAND;
    asm ("	INSFW	&3, &13, old_ipl, %psw");
    return(PASS);
}


/*  This interrupt handler does nothing but return  */
long 
ihandler() {
#ifdef	DEBUG
PRINTF("Interrupt taken\n");
#endif
}
