/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/olboot/flboot.c	1.1.1.5"

#include	"sys/types.h"
#include	"sys/psw.h"
#include	"sys/inode.h"
#include	"sys/elog.h"
#include	"sys/iobuf.h"
#include	"sys/boot.h"
#include	"sys/firmware.h"
#include	"sys/id.h"
#include	"a.out.h"
#include 	"sys/ino.h"
#include	"sys/fsiboot.h"
#include	"sys/edt.h"
#include	"sys/extbus.h"
#include	"sys/sys3b.h"
#include	"sys/param.h"
#include	"sys/lboot.h"
#include	"sys/sbd.h"
#include 	"sys/csr.h"
#include 	"sys/immu.h"
#include	"sys/nvram.h"
#include	"sys/vtoc.h"

char euname[] = "Essential Boot Utilities";

extern lls();
extern icd_acs();

int bootstartaddr;

#define TRUE (char)1
#define FALSE (char)0
#define FNUMPAR	8
#define MINMEM		0x400000	/* minimum requirement for main memory */
#define NIFBLK		1422		/* number of sectors on a floppy */

extern char	IOBASE[];	/* base io buffer */
extern char	IND3[];		/* 3rd level indirect block */
extern char	DATA[];		/* a data block */
extern char	AHDR[];		/* a.out header block for UNIX */
extern char	IND2[];		/* 2nd level indirect block */
extern char	IND1[];		/* 1st level indirect block */
extern char	INODE[];	/* inode block */
extern char	DIR[];		/* directory block */

extern struct inode	Dinode;		/* Inode of file system root directory */
extern struct dinode disk_inode;	/* Disk inode for mtimes */
extern struct inode	Fndinode;	/* Inode of file found by findfile() */
extern struct inode	muinode;	/* Inode of mUNIX program */
extern struct inode	Linode;		/* Inode of last directory found */
char		bfname[BOOTNAME];	/* Boot file name */
extern int		Fso;		/* File system offset */
extern int		fstype;		/* File system type flag */
char		MagicMode = 0;	/* see lboot source */
#define ICDVTOC ((struct vtoc *) (BOOTADDR + ICDBLKSZ))
int xedtsect;

main()
{
	register char	*p, *p1;
	struct  inode	binode;		/* inode of boot program */
	int s;
	struct	aouthdr	*ahp;		/* ptr to UNIX header */
	int	(*uboot)();		/* pointer to boot program entry */
	char	oname[40];
	unsigned char demand_config = FALSE;
	int	fndresp;		/* Return code from findfile() */
	int i;
	caddr_t startadd;
	long	start, total, remain;
	int	floppy;
	char	cont[4];
	long icdsize; 	/* Number of blocks in icd. */
	long swapsz; 	/* Number of blocks in swap area of icd. */
	caddr_t icdstart; /* The starting address of icd in main memory. */

	/*
	 * turn on PRINTF for DEMANDBOOT
	 */

	IO = ON;

	if (SIZOFMEM < MINMEM) {
		PRINTF("\nYour system must be configured with at least 4 Megabytes \
main memory to proceed with the installation.\n\n");
		return(FW_FAIL);
	}

	for (i = 0; i < FNUMPAR; i++) {
		switch(ICDVTOC->v_part[i].p_tag) {
		case V_BACKUP:
			icdsize = ICDVTOC->v_part[i].p_size;
			icdstart = (caddr_t)(MAINSTORE + SIZOFMEM - icdsize * ICDBLKSZ);
			break;
		case V_SWAP:
			swapsz = ICDVTOC->v_part[i].p_size;
			break;
		case V_ROOT:
			Fso = ICDVTOC->v_part[i].p_start;
			break;
		default:
			break;
		}
	}

	WNVRAM((char *)&icdsize, (char *) &(UNX_NVR->icdsize), 4);

	total = icdsize - swapsz;
	if (STRCMP(P_CMDQ->b_name, "icdboot") != 0) {
		PRINTF("\nLoading %s. Please wait...\n", euname);
		startadd = icdstart - ICDBLKSZ;
		for(floppy = 2;; floppy++) {
			remain = NIFBLK;
			start = - 1;
			do {
				if (!FD_ACS(++start, startadd += ICDBLKSZ, DISKRD, NOCHANGE))
					return(FW_FAIL);
			} while(--total > 0 && --remain > 0);
			if (total <= 0)
				break;
			do {
				PRINTF("\nPlease insert the %s floppy number %d.\nType \"go\" when ready [ go quit ] ", euname,
				    floppy);
				GETS(cont);
				if(!STRCMP(cont, "quit"))
					return(FW_FAIL);
			} while (STRCMP(cont, "go"));
		}
		FD_ACS(0, IOBASE, DISKRD, LAST);	 /* dummy read to unlock floppy */
		/* PRINTF("You may remove the %s floppy number %d.\n", euname, floppy); */
	}

	P_CMDQ->b_dev = ICD;

	readsb();	/* read in SUPERBLOCK */

	/* Set up access to root file system. */

	if (!findfs()) {
		PRINTF("Can't access root filesystem!\n");
		RUNFLG = FATAL;		/* FATAL error */
		RST = 1;		/* Request firmware reset */
		return(FW_FAIL);	/* Return to firmware */
	}

	/* Initialize boot file name to that passed by firmware */
	for (p = bfname, p1 = P_CMDQ->b_name; *p++ = *p1++;);
	if (!STRCMP(bfname, "icdboot"))
		if (prompt(bfname))
			return(FW_FAIL);

	/* Loop until a bootable program is found */
	for (;;) {

		if (0 == STRCMP(bfname, "magic mode")) {
			MagicMode = 1;
			PRINTF("POOOF!\n");
			if (prompt(bfname))
				return(FW_FAIL);
			continue;
		}

		if (P_CMDQ->b_type == DEMANDBOOT)
			if (!demand_config)
				WNVRAM(oname, (char *)(UNX_NVR->bootname), 20);

		fndresp = findfile(bfname);

		/* if we can't find the requested file */
		if (fndresp == NOTFOUND) {
			/* If path is a directory, list the contents */
			if (Linode.i_mode & 0111) {
				PRINTF("%s isn't a bootable file!\nLast valid directory in path contains:\n",bfname);
				lls(&Linode);
			}
			if (prompt(bfname)) {
				RUNFLG = REENTRY;
				return(FW_FAIL);
			}
			continue;
		} else if (fndresp == DIRFOUND) {	     /* Request was directory */
			if (bfname[0] == '\0') {
				PRINTF("The boot directory contains:\n");
			} else {
				PRINTF("%s is a directory !\nIt contains:\n", bfname);
			}
			lls(&Linode);
			if (prompt(bfname))
				return(FW_FAIL);
			continue;
		} else {				/* Boot file found */
			binode = Fndinode;

			/*
			 * Boot the file as specified by the inode: binode
 			 */


			s = nloadprog(binode);

			if (s == 3)
				return(FW_FAIL);

			if (s > 0)
				break;

			PRINTF("Cannot load %s.\n", bfname);
			if (prompt(bfname))
				return(FW_FAIL);
			continue;
		}
	}

	if (MagicMode) {
		RUNFLG=VECTOR;
		PRINTF("Magic Mode.  Name=%s, start address = %x.\n",
			bfname, bootstartaddr);
		restart();
	}

	/* execute the bootable program */

	uboot = (int (*)())bootstartaddr;

	/* save entry point address in NVRAM */
	WNVRAM((char *) &(ahp->entry), (char *) &(UNX_NVR->spmem), 4);

	(*uboot)();		/* call the program */

	/*
	 * Something when very wrong, return to FIRMWARE
	 */
	return(FW_FAIL);
}

/*
 * prompt()
 *
 * Prompt for another boot file. Returns 0 if one was
 * provided, -1 otherwise.
 */
static int
prompt(name)
register char	*name;
{
	do {
		PRINTF("Enter name of boot file or 'q' to quit: ");
		GETS(name);
	} while (name[0] == '\0');
	return (name[0] == 'q' && name[1] == '\0' ? -1 : 0);
}
