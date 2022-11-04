/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/physdsk.c	1.7"
/* 
 *		3B2 Computer UNIX Integral Disk Driver Common
 *		DMA break-up routine (used by integral hard and 
 *		floppy disk drivers)
 *
 */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sbd.h"
#include "sys/immu.h"
#include "sys/fs/s5dir.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/sysmacros.h"
#include "sys/conf.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/proc.h"
#include "sys/errno.h"
#include "sys/buf.h"
#include "sys/elog.h"
#include "sys/iobuf.h"
#include "sys/systm.h"
#include "sys/inline.h"
#include "vm/seg_kmem.h"

/*
 *	Break up the request that came from physio into chunks of
 *	contiguous memory so we can get around the DMAC limitations
 *	We must be sure to pass at least 512 bytes (one sector) at a
 *	time (except for the last request).   */

/* determine no. bytes till page boundary */

#define	pgbnd(a)	(NBPP - ((NBPP - 1) & (int)(a)))
#define dbbnd(a)	(NBPSCTR - ((NBPSCTR - 1) & (int)(a)))
#define SLEEP(v)	sleep((caddr_t)v,PRIBIO)

/*
 * This routine only exists for compatibility with old-style
 * drivers.  It will go away in the next major release.  dma_pageio()
 * should be used instead.
 */
void
dma_breakup(strat, bp)
void (*strat)();
register struct buf *bp;
{
	register char *va;
	register int cc, rw;

	rw = bp->b_flags & B_READ;

	if (dbbnd(u.u_base) <  NBPSCTR) {

		/* user area is not aligned on block (512 byte) boundary 
		 * so copy data to a contiguous kernel buffer and do the
		 * dma from there
		 */

		va = kseg(1);
		if (va == NULL) {
			bp->b_flags |= B_ERROR | B_DONE;
			bp->b_error = EAGAIN;
			return;
		}
		bp->b_un.b_addr = va;
		u.u_segflg = 0;
		do {
			bp->b_blkno = btod(u.u_offset);
			bp->b_bcount = cc = min(u.u_count, NBPP);
			bp->b_flags &= ~B_DONE;

			if (rw == B_READ) {
				(*strat)(bp);
				spl6();
				while ((bp->b_flags & B_DONE) == 0) {
					bp->b_flags |= B_WANTED;
					SLEEP(bp);
				}
				spl0();
				if (bp->b_flags & B_ERROR) {
					unkseg(va);
					return;
				}
				iomove(va, cc, rw);
			} else {
				iomove(va, cc, rw);
				(*strat)(bp);

				spl6();
				while ((bp->b_flags & B_DONE) == 0) {
					bp->b_flags |= B_WANTED;
					SLEEP(bp);
				}
				spl0();
				if (bp->b_flags & B_ERROR) {
					unkseg(va);
					return;
				}
			}
		} while (u.u_count);
		unkseg(va);
	} else {
		/*	The buffer is on a sector boundary
		**	but not necessarily on a page boundary.
		*/
	
		if ((bp->b_bcount = cc = 
			min( u.u_count, pgbnd(u.u_base))) < NBPP) {

			/*
			 *	Do the fragment of the buffer that's in the
			 *	first page
			 */

			bp->b_flags &= ~B_DONE;
			(*strat)(bp);
			spl6();
			while ((bp->b_flags & B_DONE) == 0) {
				bp->b_flags |= B_WANTED;
				SLEEP(bp);
			}
			spl0();
			if (bp->b_flags & B_ERROR) {
				return;
			}
			bp->b_blkno += btod(cc);
			bp->b_un.b_addr += cc;
			u.u_count -= cc;
		}

		/*
		 *	Now do the DMA a page at a time
		 */

		while (u.u_count) {
			bp->b_bcount = cc = min(u.u_count, NBPP);
			bp->b_flags &= ~B_DONE;
			(*strat)(bp);
			spl6();
			while ((bp->b_flags & B_DONE) == 0) {
				bp->b_flags |= B_WANTED;
				SLEEP(bp);
			}
			spl0();
			if (bp->b_flags & B_ERROR) {
				return;
			}
			bp->b_blkno += btod(cc);
			bp->b_un.b_addr += cc;
			u.u_count -= cc;
		}
	}
}  /* end dma_breakup */

/*
 * New DMA breakup routine for use by "new" drivers.
 */

void
dma_pageio(strat, bp)
void (*strat)();
register struct buf *bp;
{
	register int cc, rw;
	register struct buf *nbp;
	extern struct buf *ngeteblk();

	rw = bp->b_flags & B_READ;

	if (dbbnd(bp->b_un.b_addr) <  NBPSCTR) {

		/* user area is not aligned on block (512 byte) boundary 
		 * so copy data to a contiguous kernel buffer and do the
		 * dma from there
		 */

		nbp = ngeteblk(NBPP);
		if (nbp == NULL) {
			bp->b_flags |= B_ERROR | B_DONE;
			bp->b_error = EAGAIN;
			return;
		}
		nbp->b_dev = bp->b_dev;
		nbp->b_edev = bp->b_edev;
		nbp->b_flags = bp->b_flags;
		nbp->b_proc = bp->b_proc;
		do {
			nbp->b_blkno = bp->b_blkno;
			nbp->b_bcount = cc = min(bp->b_bcount, NBPP);
			nbp->b_flags &= ~B_DONE;

			if (rw == B_READ) {
				(*strat)(nbp);
				spl6();
				while ((nbp->b_flags & B_DONE) == 0) {
					nbp->b_flags |= B_WANTED;
					SLEEP(nbp);
				}
				spl0();
				if (nbp->b_flags & B_ERROR) {
					bp->b_error = nbp->b_error;
					bp->b_flags |= B_ERROR|B_DONE;
					brelse(nbp);
					return;
				}
				if (copyout(nbp->b_un.b_addr, bp->b_un.b_addr, cc)) {
					brelse(nbp);
					bp->b_flags |= B_ERROR|B_DONE;
					bp->b_error = EFAULT;
					return;
				}
			} else {
				if (copyin(bp->b_un.b_addr, nbp->b_un.b_addr, cc)) {
					brelse(nbp);
					bp->b_flags |= B_ERROR|B_DONE;
					bp->b_error = EFAULT;
					return;
				}
				(*strat)(nbp);

				spl6();
				while ((nbp->b_flags & B_DONE) == 0) {
					nbp->b_flags |= B_WANTED;
					SLEEP(nbp);
				}
				spl0();
				if (nbp->b_flags & B_ERROR) {
					bp->b_error = nbp->b_error;
					bp->b_flags |= B_ERROR|B_DONE;
					brelse(nbp);
					return;
				}
			}
			bp->b_un.b_addr += cc;
			bp->b_bcount -= cc;
			bp->b_blkno += btod(cc);
		} while (bp->b_bcount);
		bp->b_flags |= B_DONE;
		brelse(nbp);
	} else {
		/*	The buffer is on a sector boundary
		**	but not necessarily on a page boundary.
		*/
	
		cc = bp->b_bcount;
		if ((bp->b_bcount = MIN(bp->b_bcount, pgbnd(bp->b_un.b_addr))) < NBPP) {

			/*
			 *	Do the fragment of the buffer that's in the
			 *	first page
			 */

			bp->b_flags &= ~B_DONE;
			(*strat)(bp);
			spl6();
			while ((bp->b_flags & B_DONE) == 0) {
				bp->b_flags |= B_WANTED;
				SLEEP(bp);
			}
			spl0();
			if (bp->b_flags & B_ERROR) {
				return;
			}
			bp->b_blkno += btod(bp->b_bcount);
			bp->b_un.b_addr += bp->b_bcount;
			cc -= bp->b_bcount;
		}

		/*
		 *	Now do the DMA a page at a time
		 */

		while (cc) {
			bp->b_bcount = MIN(cc, NBPP);
			bp->b_flags &= ~B_DONE;
			(*strat)(bp);
			spl6();
			while ((bp->b_flags & B_DONE) == 0) {
				bp->b_flags |= B_WANTED;
				SLEEP(bp);
			}
			spl0();
			if (bp->b_flags & B_ERROR) {
				return;
			}
			bp->b_blkno += btod(bp->b_bcount);
			bp->b_un.b_addr += bp->b_bcount;
			cc -= bp->b_bcount;
		}
	}
}
