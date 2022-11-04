/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/setup.c	1.4"


/*
 * program to set up access to init file system on device specified
 * by values in command queue (boot.h)
 */


#include <sys/types.h>
#include <sys/sbd.h>
#include <sys/boot.h>
#include <sys/firmware.h>
#include <sys/diagnostic.h>
#include <sys/inode.h>
#include <sys/param.h>
#include <sys/lboot.h>
#include <sys/id.h>
#include <sys/vtoc.h>

extern struct inode Dinode;		/* inode of file system root directory */
extern struct inode Fndinode;		/* inode of file found by findfile() */
extern struct inode Linode;		/* inode of last directory found */
extern int Fso;			/* file system offset		*/
extern int fstype;			/* file system block size	*/

/* define boot device's VTOC & pdinfo locations after MBOOT code */

#define BLKSIZE 0x200
#define MYVTOC ((struct vtoc *)(BOOTADDR + BLKSIZE))
#define MYPDINFO ((struct pdinfo *)(BOOTADDR + 2 * BLKSIZE))

extern char *strcpy(), *strcat();
extern char	IOBASE[];	/* base i/o buffer		*/
extern char	IND3[];		/* 3rd level indirect block	*/
extern char 	DATA[];		/* a data block			*/
extern char	AHDR[];		/* a.out header block for UNIX	*/
extern char	IND2[];		/* 2nd level indirect block	*/
extern char	IND1[];		/* 1st level indirect block	*/
extern char	INODE[];	/* inode block			*/
extern char	DIR[];		/* directory block		*/

extern struct request p_req;
#define P_REQ (&p_req)


setup()
{

	unsigned long logstrt;		/* logical start # */
	unsigned short i;		/* index into vtoc partitions */

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
		}

	readsb();		/* read SUPERBLOCK */

	/* set up access to init file system */

	if ( !findfs() )
		{
		return(FAIL);		/* return to firmware */
		}

	return(PASS);
}


/*
 * Killbdev is a routine to de-select the integral floppy boot device
 * and send peripheral boot devices back toward their physical
 * description blocks before resetting them.
 *
 * DGMON and FILLEDT use this routine before returning to MCP
 * to compensate for the lack in the Release 1.1 MCP boot code
 * and a known problem with one peripheral's boot PROM code.
 */

#include <sys/iu.h>
#include <sys/iobd.h>
#include <sys/csr.h>

extern unsigned char int_flag;	/* interrupt flag - to suppress PRINTFs in boot device and console routines */
void
killbdev(bootdev)
char bootdev;
{
char slot;	/* slot # of boot device */

/* Shutdown the floppy disk if it's the boot device. */

if (bootdev == 0)
	{
	SBDWCSR->c_flop = ON;
	IDUART->scc_ropbc = F_LOCK;
	IDUART->scc_ropbc = F_SEL;
	}

/*
 * Send peripheral boot dev back to block 0 with dummy read to
 * BOOTADDR.
 * Return value of read is ignored; it does't matter if mboot
 * is overwritten.
 * Reset peripheral boot device before return to MCP.
 */

else if ((slot = (bootdev >> 4)) != 0)
	{
	IOBLK_ACS(bootdev, 0, BOOTADDR, BLKRD);
	IO_SMART(slot)->reset = 0;
	}
}

/*
 * Rstrbdev is a routine to restore the integral floppy and peripheral
 * boot devices when diagnostic tests of them completed or received
 * exceptions or interrupts.
 */

#include <sys/edt.h>

#define FD_DELAY 300		/* 10msec tick count for floppy spin-up delay */

unsigned long jump_reg[12];	/* storage for SETJMP/LONGJMP */

rstrbdev(bootdev)
char bootdev;
{
char slot;	/* slot # of device */

slot = bootdev >> 4;

	/*
	 * Check the floppy disk motor's CSR bit for floppy boot.
	 * Set the bit if it's cleared & wait 3 sec for spin-up.
	 */

if (bootdev == 0 && (SBDRCSR & CSRFLOP) == 0)
	{
	SBDWCSR->s_flop = ON;
	HWCNTR(FD_DELAY);
	}

	/*
	 * The peripheral boot device interface must be restored after its tests
	 * are done.
	 */

if (slot == EDTP(OPTION)->opt_slot && slot != 0)
	{
	SL_RESET(OPTION) = 0;		/* reset peripheral before SYSGEN to be safe */
	if (FW_SYSGEN(slot) == FAIL)
		{
		/* SYSGEN of peripheral boot device failed; print error message if no interrupt */
		if (int_flag == 0)
			PRINTF("SYSGEN FAILED FOR LOAD DEVICE %s IN SLOT %d\n",
				EDTP(OPTION)->dev_name, EDTP(OPTION)->opt_slot);

		if (P_CMDQ->b_type == AUTOBOOT)
			{
			return(FAIL);	/* FAIL will abort the diagnostics and reset systen */
			}
		else	{
			LONGJMP(jump_reg);	/* return to prompt loop in DEMAND mode */
			}
		}

	/* boot SYSGEN passed: read block 0 to home device */

	else	IOBLK_ACS(bootdev, 0, BOOTADDR, BLKRD);
	}

return(PASS);
}


/**********************************************************************/
/*                                                                    */
/*    Function Name: cleanup (taken from MCP code)                    */
/*          Authors: G. E. Laggis, M. H. Halt                         */
/*                                                                    */
/*          Purpose: clear all interrupt and fault sources            */
/*                                                                    */
/* Input Parameters: none                                             */
/*                                                                    */
/*     Return Value: none                                             */
/*                                                                    */
/* Globals Affected: All programmable CSR bits, dma, and floppy disk  */
/*                                                                    */
/**********************************************************************/

#include <sys/sit.h>
#include <sys/dma.h>
#include <sys/if.h>

void
cleanup()
{
char eflag;	/* dummy variable */

SBDWCSR->c_sanity = ON;		/* clear ALL csr bits that can be */
SBDWCSR->c_parity = ON;
SBDWCSR->c_align = ON;
SBDWCSR->c_pir8 = ON;
SBDWCSR->c_pir9 = ON;

IDMAC->RTR_WMC = ON;		/*  clear DMAC  */
eflag=IDUART->c_uartdma;	/* clear uart dma interrupt */
eflag=IFLOPY->statcmd;		/* clear floppy interrupt */
SBDSIT->command = SITCT1 | SITLSB | SITMD3 | SITBIN;
eflag=SBDSIT->c_latch;		/* clear interval timer interrupt */

SBDWCSR->c_timers = ON;
SBDWCSR->c_inhibit = ON;
}

/*
 * Routine to reset system after AUTOBOOT interrupts and exceptions in dgmon
 * and filledt and after catastrophic failures such as failure to restore
 * a peripheral floating console
 */

extern long (*fst_int_hand)();	/* storage for original interrupt handler */
extern long (*fst_exc_hand)();	/* storage for original exception handler */
void
sysreset()
{
INT_HAND = fst_int_hand;	/* restore original interrupt handler */
EXC_HAND = fst_exc_hand;	/* restore original exception handler */

/* shutdown floating boot device since int. or exc. probably missed regular shutdown */

killbdev(P_CMDQ->b_dev);

/* delay 20 msec for completing any console transmission, then reset 3b2 */

HWCNTR(2);

RUNFLG = FATAL;
RST = ON;
for (;;);	/* wait for the reset */
}


/*
 * Rstrcons is a routine to restore peripheral console devices when
 * diagnostic tests are complete, when the syg routine has checked
 * subdevices and for exceptions and interrupts.
 *
 * The FL_CONS structure provides the console location data.  The OPTION
 * variable stores which EDT entry is diagnosed or queried for subdevices.
 */

#include <sys/dsd.h>

extern char Dirname[];
extern char filename[];

void
rstrcons()
{
unsigned int slot;	/* current slot # */
			/* OPTION is the current EDT entry # */

slot = EDTP(OPTION)->opt_slot;

if (slot != 0 && FL_CONS->cons_found == ON
	&& FL_CONS->device == slot)
	{
	SL_RESET(OPTION) = 0;	/* reset peripheral before SYSGEN to be safe */
	if (FW_SYSGEN(slot) == FAIL)
		{
		if (int_flag == 0)
			PRINTF("console SYSGEN failed for %s in slot %d\n",EDTP(OPTION)->dev_name,slot);
		sysreset();	/* no console; reset system */
		}

	/* construct pump file path name */
	(void) strcat(strcpy(Dirname,"dgn/C."),EDTP(OPTION)->dev_name);

	if (EDTP(OPTION)->cons_file == ON && fw_pump(slot,Dirname) == FAIL)
		{
		if (int_flag == 0)
			PRINTF("console pump failed for %s in slot %d\n",EDTP(OPTION)->dev_name,slot);
		sysreset();	/* no console; reset system */
		}

	if (fw_dcons(slot,FL_CONS->port) == FAIL)
		{
		if (int_flag == 0)
			PRINTF("determine console failed for %s in slot %d\n",EDTP(OPTION)->dev_name,slot);
		sysreset();	/* no console; reset system */
		}

	if (int_flag == 0)
		PRINTF("console terminal re-established\n");
	}

/* by this point the console is restored; return */
	return;
}

/*
 * Routine to check for a soft power request.
 * CHECKPWR macro cast as a function to save space.
 */

void
checkpwr()
{
CHECKPWR;
}
