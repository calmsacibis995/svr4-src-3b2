/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/olboot/lboot.c	11.10"

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

extern lls();

int bootstartaddr;

#define SYSTEMFILE "/etc/system"   /* Used to compare dates */
#define HMAJOR 1
#define TRUE (char)1
#define FALSE (char)0
#define FMAJOR 2
#define	restart() { RST=1; for (;;) ; }
#define XEDTSIZE 25

extern char	IOBASE[];	/* base io buffer */
extern char	IND3[];		/* 3rd level indirect block */
extern char	DATA[];		/* a data block */
extern char	AHDR[];		/* a.out header block for UNIX */
extern char	IND2[];		/* 2nd level indirect block */
extern char	IND1[];		/* 1st level indirect block */
extern char	INODE[];	/* inode block */
extern char	DIR[];		/* directory block */


extern struct inode	Dinode;		/* Inode of file system root directory */
extern struct dinode disk_inode;/* Disk inode for mtimes */
extern struct inode	Fndinode;	/* Inode of file found by findfile() */
extern struct inode	muinode;	/* Inode of mUNIX program */
extern struct inode	Linode;		/* Inode of last directory found */
char		bfname[BOOTNAME];	/* Boot file name */
extern int		Fso;		/* File system offset */
extern int		fstype;		/* File system type flag */
char		MagicMode = 0;	/* see lboot source */
#define MYVTOC ((struct vtoc *) (BOOTADDR + BSIZE))
#define MYPDINFO ((struct pdinfo *)(BOOTADDR + 2 * BSIZE))
unsigned char   check_config;
int xedtsect;

main()
{
	register char	*p, *p1;
	struct  inode	binode;		/* inode of boot program */
	struct  dinode  bdinode;
	int s;
	unsigned char   soft_demand;	/* In case config pgm not around */
	struct	aouthdr	*ahp;		/* ptr to UNIX header */
	int	(*uboot)();		/* pointer to boot program entry */
	char	oname[40];
	unsigned char demand_config;	
	char	major;
	int	fndresp;		/* Return code from findfile() */
 	int i;
	unsigned long logstrt;		/* logical start # */

	/*
	 * Turn ON PRINTF
	 */
	IO = ON;

	demand_config = FALSE;
	soft_demand = FALSE;

	/* define file system offset for disk type */

	switch (P_CMDQ->b_dev)
		{
		case FLOPDISK:
			/* assign value for integral floppy disk */
			Fso = FFSO;
			break;

		case HARDDISK0:
		case HARDDISK1:
			/* save logical start # for integral disk */
			logstrt = PHYS_INFO[P_CMDQ->b_dev - HARDDISK0].logicalst;
			break;

		default:
			/* save logical start # for peripheral device */
			logstrt = MYPDINFO->logicalst;
			break;
		}

	if (P_CMDQ->b_dev != FLOPDISK)
		{
		/* Read LBOOT's copy of VTOC for integral disk */
		for (i = 0; i < V_NUMPAR; i++)
			{
			if (MYVTOC->v_part[i].p_tag == V_ROOT)
				{
				/* ROOT partition located */
				Fso = MYVTOC->v_part[i].p_start + logstrt;
				break;
				}
			}

		/* ROOT not found */
		if (i >= V_NUMPAR)
			return(FAIL);

		xedtsect = MYVTOC->v_part[7].p_start + MYVTOC->v_part[7].p_size
					 - XEDTSIZE -1;
		xedtsect += logstrt;

	}
	major = FMAJOR;
	WNVRAM(&major, (char *) &(UNX_NVR->rootdev), 1);


	readsb();	/* read in SUPERBLOCK */

	/* Set up access to root file system. */

	if (!findfs()) {
		PRINTF("Cannot access root file system !\n");
		RUNFLG = FATAL;		/* FATAL error */
		RST = 1;		/* Request firmware reset */
		return(FAIL);		/* Return to firmware */
	}

	/* Initialize boot file name to that passed by firmware */
	for (p = bfname, p1 = P_CMDQ->b_name; *p++ = *p1++;);


	/* Loop until a bootable program is found */
	for (;;) {

		if ((P_CMDQ->b_type == UNIXBOOT) ||
		   (P_CMDQ->b_type == AUTOBOOT))
			check_config = TRUE;
		else 
			check_config = FALSE; 

	
		if ((0 == STRCMP(bfname+1, DGMON)) ||
			 (0 == STRCMP(bfname+1, FILLEDT)) ||
			 (soft_demand) || 
			 (0 == STRCMP(bfname, LBOOT)))
				check_config = FALSE;
		else
			if (P_CMDQ->b_type == UNIXBOOT)
				strcpy(bfname, UNIX);

		if (0 == STRCMP(bfname, "magic mode")) {
			MagicMode = 1;
			PRINTF("POOOF!\n");
			if (prompt(bfname))
				return(FAIL);
			continue;
		}

		if (P_CMDQ->b_type == AUTOBOOT)
		{	
			RNVRAM((char *)(UNX_NVR->bootname), oname, 20);

			if (((STRCMP(oname, FASTBOOT) == 0)) && 
				((STRCMP(bfname+1,DGMON) == 0)))
				{
					RUNFLG=REENTRY;
					LONGJMP(0);
				}

		}

		oname[0] = '\0';

		if ((((STRCMP(bfname, LBOOT)) != 0) && (P_CMDQ->b_type == UNIXBOOT)) ||
					(P_CMDQ->b_type == DEMANDBOOT))
			if (!demand_config)
				WNVRAM(oname, (char *)(UNX_NVR->bootname), 20);


		fndresp = findfile(bfname);

		/* if we can't find the requested file */
		if (fndresp == NOTFOUND) {
			switch (P_CMDQ->b_type) {
			case DEMANDBOOT:
				/* If path is a directory, list the contents */
				if (Linode.i_mode & 0111) {
					PRINTF("%s is not a bootable file !",bfname);
					PRINTF("\nLast valid directory in path contains:\n");
					lls(&Linode);
				}
				if (prompt(bfname)) {
					RUNFLG = REENTRY;
					return(FAIL);
				}
				continue;
			case AUTOBOOT:
				RUNFLG = FATAL;		/* Fatal error */
				RST = 1;		/* Req. FW reset */
				return(FAIL);

			case UNIXBOOT:
				/* If the configuration program is
				   not found after /etc/system is newer,
				   do a soft DEMANDBOOT of /unix */

				if ((STRCMP(bfname, LBOOT)) == 0)
				{
					PRINTF("Configuration program not found.  Booting UNIX\n");
					strcpy(bfname, UNIX);

			/* soft_demand is set to TRUE so that a DEMANDBOOT
			   can be simulated and no check for configuration
			   will be made the next time around. */

					soft_demand = TRUE;
					continue;
				}
				RUNFLG = FATAL;
				RST = 1;
				return(FAIL);
			default:
				RUNFLG = FATAL;		/* Fatal error */
				RST = 1;		/* Req. FW reset */
				return(FAIL);
			}
		} else if (fndresp == DIRFOUND) {	     /* Request was directory */
			if (P_CMDQ->b_type == AUTOBOOT) {
				RUNFLG = FATAL;		/* Fatal error */
				RST = 1;		/* Req. FW reset */
				return(FAIL);
			}
			if (bfname[0] == '\0') {
				PRINTF("The first level boot directory contains:\n");
			} else {
				PRINTF("%s is a directory !\nIt contains:\n", bfname);
			}
			lls(&Linode);
			if (prompt(bfname))
				return(FAIL);
			continue;
		} else {				/* Boot file found */
			binode = Fndinode;

			/* Keep track of mUNIX inode to avoid endless loop */
			if ((STRCMP(bfname, LBOOT)) == 0)
				muinode = binode;

			/*
			 * Boot the file as specified by the inode: binode
 			 */


			if (check_config)
			{
				bdinode = disk_inode;

				fndresp = findfile(SYSTEMFILE);
				if (fndresp == FOUND)
				{

				/* Compare the date of the bootable with
				   the date of /etc/system.  If /etc/system
                                   is newer, load the configuration pgm */



					if 
					(disk_inode.di_mtime > bdinode.di_mtime)
					{
						WNVRAM(SYSTEMFILE,
						   (char *)(UNX_NVR->bootname),
								 20);
						strcpy(bfname, LBOOT);
						demand_config = TRUE;
						continue;
					}

					s = nloadprog(binode);
					if (s == 3)
					{
						WNVRAM(SYSTEMFILE,
						   (char *)(UNX_NVR->bootname),
								 20);
						strcpy(bfname, LBOOT);
						demand_config = TRUE;
						continue;
					}

					if (s > 0)
						break;

					PRINTF("Cannot load %s.\n", bfname);
					if (prompt(bfname))
						return(FAIL);
					continue;
				}
				else
				{
					s = nloadprog(binode);
					if (s == 3)
					{
						PRINTF("Cannot configure /unix.\n");
						return (LDFAIL);
					}


					if (s > 0)
						break;

					PRINTF("Cannot load %s.\n", bfname);
					if (prompt(bfname))
						return(FAIL);
					continue;
				}
			}
			else
			{
				s = nloadprog(binode);

				if (s > 0)
					break;

				if (s == 3)
				{
					strcpy(bfname, LBOOT);
					demand_config = TRUE;
					continue;
				}

				PRINTF("Cannot load %s.\n", bfname);
				if (prompt(bfname))
					return(FAIL);
				continue;
			}
		}
	}

	/* get entry point address */

	ahp = (struct aouthdr *)(AHDR + sizeof(struct filehdr));
	uboot = (int(*)()) ahp->entry;		/* load entry address */

	/* save entry point address in NVRAM */

	WNVRAM((char *) &(ahp->entry), (char *) &(UNX_NVR->spmem), 4);

	if (P_CMDQ->b_dev == FLOPDISK)
		DISK_ACS(0, IOBASE, DISKRD, LAST);	/* dummy read to unlock floppy */


	/* if (P_CMDQ->b_type == DEMANDBOOT) {
		PRINTF("Do you want %s to be executed (y or n): ", bfname);
		GETS(response);
		if (!STRCMP(response, "n"))
			return(FAIL);
	} */

	if (MagicMode) {
		PRINTF("go %x to execute %s\n", bootstartaddr, bfname);
		RUNFLG=VECTOR;
		restart();
	}

	/* execute the bootable program */

	uboot = (int (*)())bootstartaddr;
	(*uboot)();		/* call the program */

	/*
	 * Something when very wrong return to FIRMWARE
	 */
	return(FAIL);
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
