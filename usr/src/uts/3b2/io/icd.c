/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/icd.c	1.15"

#include "sys/types.h"
#include "sys/param.h"
#include "sys/boot.h"
#include "sys/buf.h"
#include "sys/cmn_err.h"
#include "sys/errno.h"
#include "sys/firmware.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/sbd.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/vtoc.h"
#include "sys/inline.h"
#include "sys/systm.h"
#include "sys/mkdev.h"
#include "sys/ddi.h"
#include "sys/uio.h"
#include "sys/cred.h"

#define ICDPART(dev)	getminor(dev) 	/* partition number */

/* The icdstate variable may be set to one of these two values */
#define NOTSETUP	0	/* There is no valid VTOC on ICD */
#define PRESENT		1	/* The ICD is present and may be used */

extern unsigned icdblk;

/* The volume table of contents for the in-core disk image */
struct vtoc icdvtoc;

unsigned char icdstate;		/* The state of the ICD */
unsigned long icdstart;		/* Start addr in memory of the ICD */
extern timeout(), sleep();
void icdwake();

int icddevflag = 0; 	/* new-style driver */

void
icdinit()
{
	int	x;

	if (icdblk) {
		icdstart = MAINSTORE + VSIZOFMEM - icdblk * ICDBLKSZ;

		/* Read the VTOC */
		bcopy((char *)(ICDBLKSZ + icdstart), 
		      (char *)&icdvtoc, sizeof(struct vtoc));
		if(icdvtoc.v_sanity != VTOC_SANE) {
			icdstate = NOTSETUP;
			return;
		}
		icdstate = PRESENT;

		/* Force a process switch to have some kernel veriables */
		/* initialized properly. This change should be made in  */
		/* the main program of kernel in order to make it work  */
		/* with all the fast device as ICD.			*/
		x = splhi();
		timeout(icdwake, 0, 2);
		sleep((caddr_t)&icdblk, 0);
		splx(x);
	} else
		icdstate = NOTSETUP;
}

void
icdwake()
{
	wakeup((caddr_t)&icdblk);
}

/* ARGSUSED */
icdopen(devp,flag,otyp,cr)
dev_t *devp;
int flag;
int otyp;
cred_t *cr;
{
	if (icdstate != PRESENT)
		/* The state must be ABSENT or NOTSETUP */
		return(ENXIO);

	return(0);
}

/* ARGSUSED */
icdclose(devp,flag,otyp,cr)
dev_t *devp;
int flag;
int otyp;
cred_t *cr;
{
	/* NULL FUNCTION */
	return(0);
}

void
icdstrategy(bp)
register struct buf *bp;
{

	register char *toptr;
	register char *frmptr;
	register char partition;	/* partition number */
	register long icd_addr;		/* Address of the job */
	register long end_addr;		/* End address of ICD */

	if(icdstate != PRESENT) {
		cmn_err(CE_WARN, "In-Core Disk not existing\n");
		goto error;
	}

	partition = ICDPART(bp->b_edev);

	end_addr = icdstart + icdblk * ICDBLKSZ;
	icd_addr = icdstart + ((bp->b_blkno + 
           icdvtoc.v_part[partition].p_start) * ICDBLKSZ);

	/* If job is out of bounds - fail it */
	if ((icd_addr >= end_addr) || ((icd_addr + bp->b_bcount) > end_addr))
		goto error;

	if (bp->b_flags & B_READ) {
		toptr = bp->b_un.b_addr;
		frmptr = (char *)icd_addr;
	} else {
		toptr = (char *)icd_addr;
		frmptr = bp->b_un.b_addr;
	}

	bp->b_resid = 0;

	bcopy(frmptr, toptr, bp->b_bcount);

	iodone(bp);
	return;

error:
	bp->b_flags |= B_ERROR;
	bp->b_error = ENXIO;
	iodone(bp);
	return;
}  

int
icdsize(dev)
dev_t dev;
{
	return icdblk;
}

dev_t
icdmkdev(par)
int par;
{
	extern major_t icdmaj;
	long icdemaj;

	icdemaj = itoemajor(icdmaj, -1);
	return(makedevice(icdemaj, par));
}

void
icdbreakup(bp)
register struct buf *bp;
{
	dma_pageio(icdstrategy, bp);
}

/* ARGSUSED */
icdread(dev, uiop, cr)
register dev_t dev;
register uio_t *uiop;
register cred_t *cr;
{
	return(physiock(icdbreakup, NULL, dev, B_READ, 
		icdvtoc.v_part[ICDPART(dev)].p_size, uiop));
}

/* ARGSUSED */
icdwrite(dev, uiop, cr)
register dev_t dev;
register uio_t *uiop;
register cred_t *cr;
{
	return(physiock(icdbreakup, NULL, dev, B_WRITE, 
		icdvtoc.v_part[ICDPART(dev)].p_size, uiop));
}
