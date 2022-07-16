/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/boot/3b2_io.c	1.7"
#include	"sys/types.h"
#include	"sys/psw.h"
#include	"sys/elog.h"
#include	"sys/iobuf.h"
#include	"sys/boot.h"
#include	"sys/firmware.h"
#include	"sys/id.h"
#include	"sys/sbd.h"
#include 	"sys/csr.h"
#include 	"sys/immu.h"
#include	"sys/nvram.h"
#include	"sys/vtoc.h"
#include	"sys/fsiboot.h"

/* Function to read only one sector from any device attached to 3b2 */

void
read_onesect(sector, mem)
long sector;
char *mem;
{

	switch(P_CMDQ->b_dev){
		case FLOPDISK:
			FD_ACS(sector, mem, DISKRD, NOCHANGE);
			break;

		case HARDDISK0:
		case HARDDISK1:
			HD_ACS(P_CMDQ->b_dev - HARDDISK0, sector, mem, DISKRD);
			break;

		default:
			IOBLK_ACS(P_CMDQ->b_dev, sector, mem, DISKRD);
			break;
	}
}

/* Function to write only one sector from any device attached to 3b2 */

void
write_onsect(sector, mem)
long sector;
char *mem;
{

	switch(P_CMDQ->b_dev){
		case FLOPDISK:
			FD_ACS(sector, mem, DISKWT, NOCHANGE);
			break;

		case HARDDISK0:
		case HARDDISK1:
			HD_ACS(P_CMDQ->b_dev - HARDDISK0, sector, mem, DISKWT);
			break;

		default:
			IOBLK_ACS(P_CMDQ->b_dev, sector, mem, DISKWT);
			break;
	}
}



/* The speed of this boot program comes from this function.  It attempts to read
   multiple sectors from the boot device.  Presently it only does actual single
   command multiple sector reads from "HD" devices.  Other devices are handled
   by nonhd_bigread which loops through and reads one sector at a time. */

 int
big_read(sector, mem, nsectors)
long sector;
char *mem;
long nsectors;
{

	int head;
	int msector;
	int sectpercyl;
	int sectleft;
	register int j;
	extern struct pdinfo idpdt[];
	extern struct badblock baddies[];
	unsigned char unit;
	int nsects;
	long offset;

	if ((P_CMDQ->b_dev != HARDDISK0) && (P_CMDQ->b_dev != HARDDISK1))
		return (nonhd_bigread(sector, mem, nsectors));


	unit = P_CMDQ->b_dev - HARDDISK0;

	nsects = nsectors;

	/* We must not attempt to read through a bad block.  If a bad block
	   exists within this range we read up to it, read the good block,
	   and then read the rest. */

	for (j=0 ; j < IDDEFCNT; j++)
	{
		if (baddies[j].bad == 0)
			break;

		if ((baddies[j].bad >= sector) && (baddies[j].bad < (sector + nsects)))
		{
			if (baddies[j].bad > sector){
				big_read(sector, mem, baddies[j].bad - sector);
				mem +=  ((baddies[j].bad - sector) * SECTSIZE);
			}

			read_onesect(baddies[j].good, mem);
			mem += SECTSIZE;

			if ((baddies[j].bad + 1) < (sector + nsects))
				big_read(baddies[j].bad+1, mem, nsects - 
					((baddies[j].bad - sector) + 1));

			return (nsectors*SECTSIZE);
		}
	}

	sectpercyl = idpdt[unit].tracks * idpdt[unit].sectors;
	sectleft = sectpercyl - (sector % sectpercyl);

	/* The hd driver will not allow a single read past the end of the
	   cylinder.  We read up to the end and call ourself recursively */

	if (nsects > sectleft)
		nsects = sectleft;

	offset = sector % sectpercyl;
	head = offset / idpdt[unit].sectors;
	msector = offset % idpdt[unit].sectors;


	/* The hd driver will not allow a single read to cross the 8th track(!).
	   We must read to it and call ourself recursively. */

	if (head < IDMAXHD)
	{
		sectleft = ((IDMAXHD - head) * idpdt[unit].sectors) - msector;
		if (nsects > sectleft)
			nsects = sectleft;

	}

	/* Do the read */
	j = hd_read_sectors(P_CMDQ->b_dev - HARDDISK0, sector, mem, nsects);

	if (j == FW_FAIL)
		PRINTF("hd_read_sectors failed\n"); /* should not occur */
				  	/* unless unmapped bad block */

	/* If we haven't handled the entire request, call recursively. */

	if (nsects < nsectors)
		return ((nsects*SECTSIZE) + big_read(sector+nsects, 
			mem + (nsects*SECTSIZE), nsectors - nsects));
	else
		return (nsectors*SECTSIZE);
}

/* Loop through the sectors reading one at a time.  For unsupported devices. */

nonhd_bigread(sector, mem, nsectors)
long sector;
char *mem;
long nsectors;
{
	register int i;

	for (i=0; i < nsectors; i++)
	{
		read_onesect(sector+i, mem);
		mem = mem + SECTSIZE;
	}

	return (nsectors * SECTSIZE);
}
