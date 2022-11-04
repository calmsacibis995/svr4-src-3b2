/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:ml/cdump.c	1.9"

#include "sys/sbd.h"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/psw.h"
#include "sys/immu.h"
#include "sys/nvram.h"
#include "sys/firmware.h"
#include "sys/cdump.h"
#include "sys/todc.h"
#include "sys/debug.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"

#define NVRBYTES  512		/* number of bytes of NVRAM dumped */

extern char *mmusrama, *mmusramb, *mmufltcr, *mmufltar, *mmucr;
extern int dumpbad;

#ifdef __STDC__
STATIC void diskdump(void);
STATIC void flopdump(struct crash_hdr *);
STATIC int nquery(char *);
STATIC int query(char *);
#else
STATIC void diskdump();
STATIC void flopdump();
STATIC int nquery();
STATIC int query();
#endif

/*******************************************************************
 *
 *	This routine writes to floppy disk the following information:
 *		-the contents of NVRAM
 *		-the contents of the following mmu goodies
 *			-srama
 *			-sramb
 *			-fault code register
 *			-fault address register
 *			-configuration register
 *		-as much of the contents of mainstore as can be stored
 *		 	on the  floppy
 *
 *	procedural outline: (here is basically what the routine does)
 *		-dump the contents of the first 710K bytes of mainstore 
 *			to the floppy (or all of mainstore if the system
 *			contains less than 710K bytes of mainstore)
 *		-write out two more blocks containing:
 *			-contents of NVRAM
 *			-mmu information
 *		-allow the user to put in a second floppy if the
 *			rest of mainstore is to be dumped
 *
 ***************************************************************************/

STATIC void
flopdump(crash_hdr)
struct	crash_hdr	*crash_hdr ;
{
	char	*memaddr;
	char	*restart;	/* address at start of a floppy */
	int	bindx;		/* counter for blocks written to disk */
	int	blocks_left ;
	int	blks_2_dmp ;
	char	*addr ;
	int	seq_flag ;
	
	if (!query("\nInsert first sysdump floppy."))
	{
		PRINTF("\nNo dump made\n") ;
		PRINTF("\nReturning to firmware.\n") ;
		return ;
	}


	/* make the dump */
	
	PRINTF("\nDumping mainstore\n");

	/* start dump at the beginning of mainstore */
	memaddr = (char *)MAINSTORE;
	restart = memaddr;		/* set initial restart address */
	blocks_left = SIZOFMEM / BLKSZ ;
	
	while (blocks_left > 0)
	{
		/* set up to write the first block */

		if (crash_hdr->seq_num == 1)
		{
			if (blocks_left > MAXBLKS)
				blks_2_dmp = MAXBLKS ;
			else
				blks_2_dmp = blocks_left ;
		}
		else
		{
			/* allow for block of crash header */
			if (blocks_left > (MAXBLKS - 1))
				blks_2_dmp = MAXBLKS ;
			else
				blks_2_dmp = blocks_left + 1;
		}
		
		/* write the floppy */
		for (bindx = 0; bindx < blks_2_dmp; bindx++)
		{
			if (bindx == 0)
			{
				seq_flag = FIRST ;
				if (crash_hdr->seq_num == 1)
					addr = memaddr ;
				else
					addr = (char *)crash_hdr ;
			}
			else if (bindx == blks_2_dmp - 1)
			{
				seq_flag = LAST ;
				addr = memaddr ;
			}
			else
			{
				seq_flag = NOCHANGE ;
				addr = memaddr ;
			}
		
			/* write out a block */
			if (FD_ACS(bindx, addr, DISKWT, seq_flag) == FW_FAIL)
			{
				PRINTF("\nUnable to write floppy.\n") ;
				if (!query("\nInsert new floppy for retry."))
				{
					PRINTF("\nDump Terminated.\n");
					PRINTF("%d floppies written\n\n",crash_hdr->seq_num - 1) ;
					PRINTF("Returning to firmware\n") ;
					return ;
				}
				memaddr = restart;	/* restart floppy */
				break ;
			}
			
			/* print some dots for the folks to see */
			if (bindx % 10 == 9) PRINTF(".");
	
			if (bindx != 0 || crash_hdr->seq_num == 1)
				memaddr += BLKSZ ;
		}

		/* for loop terminated abnormally */
		if (bindx != blks_2_dmp)
			continue ;
			
		restart = memaddr;		/* set new restart address */

		if (crash_hdr->seq_num == 1)
			blocks_left -= blks_2_dmp ;
		else
			blocks_left -= blks_2_dmp - 1 ;
			
		if (blocks_left > 0)
		{
			PRINTF("\nIf you wish to dump more of mainstore,\n");
			if (!query("insert new floppy.\n"))
				break ;
				
			crash_hdr->seq_num++ ;
			PRINTF("\nDumping more mainstore\n") ;
		}
	}

	PRINTF("\nDump completed.\n");
	PRINTF("%d floppies written\n",crash_hdr->seq_num) ;
	PRINTF("\nReturning to firmware\n");

}	/* flopdump */


/*
**	These are defaults; iddumpinit() sets them from DUMPDEV.
**	These variable must be here so they are physical addresses
**	and not virtual addresses, since dumpdisk runs  in
**	physical mode.
*/

STATIC int dumpoff = 0;		/* offset into dump partition */

dev_t	dumpdev	= makedevice(17, 1);	/* should be set by master.d/kernel */
int	dumpbad = 1;		/* DUMPDEV was bad; parameters not set */
dev_t	ddumpdev = makedevice(17, 1);	/* DUMPDEV for diskdump to use */
int	dumpunit = 0;		/* which disk to dump on */
int	dumppart = 1;		/* which partition to dump on */
int	dumppartst = 100;	/* partition starting block */
int	dumpsize = 8192;	/* size of dump (swap) partition */
int	dumplogicalst = 162;	/* first usable block */

/* make sure buffer is word aligned and allocated a physical address */
/* it is used by the rdsk and wdsk routines in idisk */
int dumpbuf[512/sizeof(int)] = { 0 };

STATIC void
diskdump()
{
	register	char	*memaddr;
	register	int	bindx;		/* blocks written to disk */
	register	int	blocks_left ;
	register	int	dumpaddr;	/* physical block number */
	
	PRINTF("\nDump disk: %d\n", dumpunit );
	PRINTF("Dump partition: %d\n", dumppart );
	PRINTF("Logical start: %d\n", dumplogicalst);
	PRINTF("Partition start: %d\n",dumppartst);
	PRINTF("Partition size: %d\n",dumpsize);
	PRINTF("Dump partition offset: %d\n",dumpoff);

	dumpaddr = dumppartst + dumplogicalst + dumpoff;
	PRINTF("\nDump start: %d\n", dumpaddr );

	/* start dump at the beginning of mainstore */
	memaddr = (char *)MAINSTORE;
	blocks_left = SIZOFMEM / BLKSZ ;
	
	if ( blocks_left > dumpsize )  {
		PRINTF("\nDump truncated to %d blocks\n", dumpsize );
		blocks_left = dumpsize;
	}

	if (dumpbad)  {
		PRINTF("\nDUMPDEV (%x) not correctly set up\n" , ddumpdev );
		PRINTF("\nNo dump made.\n");
		PRINTF("\nReturning to firmware\n") ;
		return ;
	}


	if (!query("\nDo you want to continue and dump to disk?"))
	{
		PRINTF("\nNo dump made.\n");
		PRINTF("\nReturning to firmware\n") ;
		return ;
	}

	/* make the dump */
	
	PRINTF("\nDumping %d blocks\n", blocks_left );

	for ( bindx = 0; bindx < blocks_left; bindx++ )
	{
		
		/* write out a block */
		if (HD_ACS(dumpunit, dumpaddr, memaddr, DISKWT) == FW_FAIL)
		{
			PRINTF("\nUnable to write hard disk %d ", dumpunit);
			PRINTF("block %x from memaddr %x.\n",dumpaddr,memaddr);
			PRINTF("Dump Terminated.\n");
			PRINTF("%d blocks written\n\n",bindx) ;
			PRINTF("Returning to firmware\n") ;
			return ;
		}
		memaddr  += BLKSZ;
		dumpaddr++;

		/* print some dots for the folks to see */
		if (bindx % 10 == 9) PRINTF(".");


	}

	PRINTF("\nDump completed.\n");
	PRINTF("%d blocks written\n",bindx) ;
	PRINTF("\nReturning to firmware\n");

}	/* diskdump */


STATIC int
query(s)
char	*s ;
{
	char	reply[100] ;
	
	PRINTF("%s\n",s) ;
	for (;;)
	{
		PRINTF("Enter 'c' to continue, 'q' to quit: ") ;
		GETS(reply) ;
		
		if (STRCMP(reply,"q") == 0)
			return (0) ;

		if (STRCMP(reply,"c") == 0)
			return (1) ;
			
		PRINTF("\nreply \"%s\" unrecognized\n",reply) ;
	}
	/* NOTREACHED */
}

STATIC int
nquery(s)
char	*s ;
{
	char	reply[100] ;
	
	PRINTF("%s\n",s) ;
	for (;;)
	{
		PRINTF("Enter 'f' for floppy, 'd' for disk, 'q' to quit: ") ;
		GETS(reply) ;
		
		if (STRCMP(reply,"q") == 0)
			return (0) ;

		if (STRCMP(reply,"f") == 0)
			return (1) ;
			
		if (STRCMP(reply,"d") == 0)
			return (2) ;
			
		PRINTF("\nreply \"%s\" unrecognized\n",reply) ;
	}
	/* NOTREACHED */
}


/* This code has been blatently copied from rtodc() */

#define	MAXTRIES 10
#define FAILLOG(msg) { continue ; }
	
int
crash_timestamp()

{
	int		trys ,
			tmp ,
			timestamp ;
	
	/* timestamp is built from the seconds, minutes, hours, and
	 * days fields of the clock converted to seconds
	 */

	for (trys = 0 ; trys < MAXTRIES; trys++)
	{
		/* dummy read to clear errors */
		tmp = SBDTOD->tenths ;
		
		if ((timestamp = SBDTOD->secs.units & 0x0F) == 0x0F)
			FAILLOG("secs.units") ;
		
		if ((tmp = SBDTOD->secs.tens & 0x0F) == 0x0F)
			FAILLOG("secs.tens") ;
		timestamp += 10 * tmp ;

		if ((tmp = SBDTOD->mins.units & 0x0F) == 0x0F)
			FAILLOG("mins.units") ;
		timestamp += 60 * tmp ;
		
		if ((tmp = SBDTOD->mins.tens & 0x0F) == 0x0F)
			FAILLOG("mins.tens") ;
		timestamp += 60 * 10 * tmp ;
		
		if ((tmp = SBDTOD->hours.units & 0x0F) == 0x0F)
			FAILLOG("hours.units") ;
		timestamp += 60 * 60 * tmp ;
		
		if ((tmp = SBDTOD->hours.tens & 0x0F) == 0x0F)
			FAILLOG("hours.tens") ;
		timestamp += 60 * 60 * 10 * tmp ;
		
		if ((tmp = SBDTOD->days.units & 0x0F) == 0x0F)
			FAILLOG("days.units") ;
		timestamp += 24 * 60 * 60 * tmp ;
		
		if ((tmp = SBDTOD->days.tens & 0x0F) == 0x0F)
			FAILLOG("days.tens") ;
		timestamp += 24 * 60 * 60 * 10 * tmp ;

		return(timestamp) ;
	}
	return(-1) ;
}

void
cdump()
{
	int	indx;	
	struct	crash_hdr crash_hdr ;
	int	*crash_iptr ;
	char	*crashsanity = CRASHSANITY ;
	int	dev;
	
	/* ask the operator if a dump is to be made */
	if (dumpbad)
		dev = query("\nDo you want to dump a system image to floppy diskette?");
	else
		dev = nquery("\nDo you want to dump a system image?");

	if ( dev == 0 ) {
		PRINTF("\nNo dump made.\n");
		PRINTF("\nReturning to firmware\n") ;
		return ;
	}

	/* initialize crash header */
	
	for (indx = 0; crashsanity[indx] != '\0'; indx++)
		crash_hdr.sanity[indx] = crashsanity[indx] ;
	crash_hdr.sanity[indx] = '\0' ;
	crash_hdr.timestamp = crash_timestamp() ;
	crash_hdr.mem_size = SIZOFMEM ;
	crash_hdr.seq_num = 1 ;
	
	/*
	 * load last 1K of firmware stack with crash header,
	 * MMU information, and the contents of NVRAM
	 */
	
	/* SPMEM is start of kernel text */

	bcopy((caddr_t)&crash_hdr,
		(char *)SPMEM - CHDR_OFFSET, 
		sizeof(struct crash_hdr));

	crash_iptr = (int *)SPMEM 
		- howmany(CHDR_OFFSET, sizeof(int *))
		+ howmany(sizeof(struct crash_hdr), sizeof(int *));

	for (indx = 0; indx < 4; indx++)
		*crash_iptr++ = *(int *) (srama + indx);
	
	/* SRAMB entries */
	for (indx = 0; indx < 4; indx++)
		*crash_iptr++ = *(int *) (sramb + indx);
	
	/* get the other goodies, too */
	*crash_iptr++ = *(int *) fltcr;	/* fault code register */
	*crash_iptr++ = *fltar;		/* fault address register */
	*crash_iptr = *(int *) crptr;	/* configuration register */
	
	/* read in the NVRAM info */
	/* RNVRAM(NVRaddr, buffaddr, # of bytes) */
	RNVRAM(ONVRAM, (char *)SPMEM - NVR_OFFSET, NVRBYTES); 

	if ( dev == 1 )
		flopdump(&crash_hdr);
	else
		diskdump();

}
