/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/bio.c	1.38.1.6"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/sbd.h"
#include "sys/param.h"
#include "sys/conf.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/immu.h"
#include "sys/proc.h"
#include "sys/disp.h"
#include "sys/systm.h"
#include "sys/sysinfo.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/debug.h"
#include "sys/user.h"
#include "sys/buf.h"
#include "sys/open.h"
#include "sys/iobuf.h"
#include "sys/conf.h"
#include "sys/var.h"
#include "sys/vfs.h"
#include "sys/cred.h"
#include "sys/vnode.h"
#include "sys/cmn_err.h"
#include "sys/inline.h"
#include "sys/kmem.h"
#include "vm/page.h"

/* Convert logical block number to a physical number */
/* given block number and block size of the file system */
/* Assumes 512 byte blocks (see param.h). */
#define LTOPBLK(blkno, bsize)	(blkno * ((bsize>>SCTRSHFT)))

/* count and flag for outstanding async writes */
int basyncnt, basynwait;

struct buf bhdrlist;	/* free buf header list */

void	printbuf();

/*
 * The following several routines allocate and free
 * buffers with various side effects.  In general the
 * arguments to an allocate routine are a device and
 * a block number, and the value is a pointer to
 * to the buffer header; the buffer is marked "busy"
 * so that no one else can touch it.  If the block was
 * already in core, no I/O need be done; if it is
 * already busy, the process waits until it becomes free.
 * The following routines allocate a buffer:
 *	getblk
 *	bread
 *	breada
 * Eventually the buffer must be released, possibly with the
 * side effect of writing it out, by using one of
 *	bwrite
 *	bdwrite
 *	bawrite
 *	brelse
 */

/*
 * Read in (if necessary) the block and return a buffer pointer.
 */
struct buf *
bread(dev, blkno, bsize)
	register dev_t dev;
	daddr_t blkno;
	long bsize;
{
	register struct buf *bp;

	sysinfo.lread++;
	bp = getblk(dev, blkno, bsize);
	if (bp->b_flags & B_DONE)
		return bp;
	bp->b_flags |= B_READ;
	bp->b_bcount = bsize;
	(*bdevsw[getmajor(dev)].d_strategy)(bp);
	u.u_ior++;
	sysinfo.bread++;
	(void) biowait(bp);
	return(bp);
}

/*
 * Read in the block, like bread, but also start I/O on the
 * read-ahead block (which is not allocated to the caller).
 */
struct buf *
breada(dev, blkno, rablkno, bsize)
	register dev_t dev;
	daddr_t blkno, rablkno;
	long bsize;
{
	register struct buf *bp, *rabp;

	bp = NULL;
	if (!incore(dev, blkno, bsize)) {
		sysinfo.lread++;
		bp = getblk(dev, blkno, bsize);
		if ((bp->b_flags & B_DONE) == 0) {
			bp->b_flags |= B_READ;
			bp->b_bcount = bsize;
			(*bdevsw[getmajor(dev)].d_strategy)(bp);
			u.u_ior++;
			sysinfo.bread++;
		}
	}
	if (rablkno && bfreelist.b_bcount>1 && !incore(dev, rablkno, bsize)) {
		rabp = getblk(dev, rablkno, bsize);
		if (rabp->b_flags & B_DONE)
			brelse(rabp);
		else {
			rabp->b_flags |= B_READ|B_ASYNC;
			rabp->b_bcount = bsize;
			(*bdevsw[getmajor(dev)].d_strategy)(rabp);
			u.u_ior++;
			sysinfo.bread++;
		}
	}
	if (bp == NULL)
		return bread(dev, blkno, bsize);
	(void) biowait(bp);
	return bp;
}

/*
 * Write the buffer, waiting for completion.
 * Then release the buffer.
 */
void
bwrite(bp)
	register struct buf *bp;
{
	register flag;

	sysinfo.lwrite++;
	flag = bp->b_flags;
	bp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
	u.u_iow++;
	sysinfo.bwrite++;
	(*bdevsw[getmajor(bp->b_edev)].d_strategy)(bp);
	if ((flag & B_ASYNC) == 0) {
		(void) biowait(bp);
		brelse(bp);
	} else
		basyncnt++;
}

/*
 * Release the buffer, marking it so that if it is grabbed
 * for another purpose it will be written out before being
 * given up (e.g. when writing a partial block where it is
 * assumed that another write for the same block will soon follow).
 * Also save the time that the block is first marked as delayed
 * so that it will be written in a reasonable time.
 */
void
bdwrite(bp)
	register struct buf *bp;
{
	sysinfo.lwrite++;
	if ((bp->b_flags & B_DELWRI) == 0)
		bp->b_start = lbolt;
	bp->b_flags |= B_DELWRI | B_DONE;
	bp->b_resid = 0;
	brelse(bp);
}

/*
 * Release the buffer, start I/O on it, but don't wait for completion.
 */
void
bawrite(bp)
	register struct buf *bp;
{

	if (bfreelist.b_bcount > 4)
		bp->b_flags |= B_ASYNC;
	bwrite(bp);
}

/*
 * Release the buffer, with no I/O implied.
 */
void
brelse(bp)
	register struct buf *bp;
{
	register struct buf **backp;
	register s;

	if (bp->b_flags & B_WANTED)
		wakeup((caddr_t)bp);
	if (bfreelist.b_flags & B_WANTED) {
		bfreelist.b_flags &= ~B_WANTED;
		wakeup((caddr_t)&bfreelist);
	}
	if (bp->b_flags & B_ERROR) {
		bp->b_flags |= B_STALE|B_AGE;
		bp->b_flags &= ~(B_ERROR|B_DELWRI);
		bp->b_error = 0;
		bp->b_oerror = 0;
	}
	s = spl6();
	if (bp->b_flags & B_AGE) {
		backp = &bfreelist.av_forw;
		(*backp)->av_back = bp;
		bp->av_forw = *backp;
		*backp = bp;
		bp->av_back = &bfreelist;
	} else {
		backp = &bfreelist.av_back;
		(*backp)->av_forw = bp;
		bp->av_back = *backp;
		*backp = bp;
		bp->av_forw = &bfreelist;
	}
	bp->b_flags &= ~(B_WANTED|B_BUSY|B_ASYNC);
	bfreelist.b_bcount++;
	bp->b_reltime = (unsigned long)lbolt;
	splx(s);
}

/*
 * See if the block is associated with some buffer
 * (mainly to avoid getting hung up on a wait in breada).
 */
int
incore(dev, blkno, bsize)
	register dev_t dev;
	register daddr_t blkno;
	register long bsize;
{
	register struct buf *bp;
	register struct buf *dp;

	blkno = LTOPBLK(blkno, bsize);
	dp = bhash(dev, blkno);
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
		if (bp->b_blkno == blkno && bp->b_edev == dev
		  && (bp->b_flags & B_STALE) == 0)
			return 1;
	return 0;
}

/* 
 * getfreeblk() is called from getblk() or ngeteblk()
 * It runs down the free buffer list to free up
 * buffers when total number of buffer or total memory used
 * by buffers exceeds thresholds
 * If there is a buffer matches the request size, reuse
 * that buffer.
 * Otherwise, free buffers and re-allocate a new buffer
 */
struct buf *
getfreeblk(bsize)
	long bsize;
{
	register struct buf *bp, *savebp = NULL;
	register int s;
	
loop:
	s = spl6();
	bp = bfreelist.av_forw;
	if (bp != &bfreelist
	  && (bfreelist.b_bufsize < bsize
	    || (bp->b_flags & B_AGE) || bhdrlist.av_forw == NULL)) {
		ASSERT(bp != NULL);
		notavail(bp);			
		bp->av_forw = bp->av_back = NULL;

		/*
		 * This buffer hasn't been written to disk yet.
		 * Do it now and free it later.
		 */
		if (bp->b_flags & B_DELWRI) {
			bp->b_flags |= B_ASYNC | B_AGE;
			bwrite(bp);
		}
		else {
			bremhash(bp);
			if (savebp == NULL && bp->b_bufsize == bsize) {
				savebp = bp;
			}
			/*
			 * If size doesn't match, free it.
			 */
			else {
				kmem_free(bp->b_un.b_addr, bp->b_bufsize);
				bfreelist.b_bufsize += bp->b_bufsize;
				struct_zero(bp, sizeof(struct buf));
				bp->b_flags |= B_KERNBUF;
				bp->av_forw = bhdrlist.av_forw;
				bhdrlist.av_forw = bp;
				/*
				 * notavail() decremented b_bcount already.
				 *
				 * bfreelist.b_bcount--;
				 */
				if (bhdrlist.b_flags&B_WANTED) {
					bhdrlist.b_flags &= ~B_WANTED;
					wakeup((caddr_t)&bhdrlist);
				}

			}
		}
		(void)splx(s);
		goto loop;
	}
	(void)splx(s);
	if (savebp != NULL) {
		return (savebp);
	}
	/*
	 * If not enough memory for this buffer, sleep.  When we
	 * return from sleep(), we must return to the caller to
	 * check the hash queue again.
	 */	
	if (bfreelist.b_bufsize < bsize) {
		ASSERT(bfreelist.av_forw == &bfreelist);
		bfreelist.b_flags |= B_WANTED;
		sleep((caddr_t)&bfreelist, PRIBIO+1);
		return NULL;
	}
	/*
	 * Allocate a new buffer.  Get a buffer header first.
	 * If no free buffer header, sleep.  When we return from
	 * sleep(), we must return to the caller to check the
	 * hash queue again.
	 */	
	if (bhdrlist.av_forw == NULL) {
		bhdrlist.b_flags |= B_WANTED;
		sleep((caddr_t)&bhdrlist, PRIBIO + 1);
		return NULL;
	}
	bp = bhdrlist.av_forw;
	bhdrlist.av_forw = bp->av_forw;
	bp->av_forw = bp->av_back = NULL;
	bp->b_un.b_addr = (caddr_t)kmem_zalloc(bsize, KM_SLEEP);
	bp->b_bufsize = bsize;
	bfreelist.b_bufsize -= bsize;
	return bp;
}		

/*
 * Assign a buffer for the given block.  If the appropriate
 * block is already associated, return it; otherwise search
 * for the oldest non-busy buffer and reassign it.
 */
struct buf *
getblk(dev, blkno, bsize)
	register dev_t dev;
	register daddr_t blkno;
	long bsize;
{
	register struct buf *bp;
	register struct buf *dp; 

	if (getmajor(dev) >= bdevcnt)
		cmn_err(CE_PANIC,"blkdev");

	blkno = LTOPBLK(blkno, bsize);
loop:
	spl0();
	if ((dp = bhash(dev, blkno)) == NULL)
		cmn_err(CE_PANIC,"devtab");
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
		if (bp->b_blkno != blkno || bp->b_edev != dev
		  || bp->b_flags & B_STALE)
			continue;
		spl6();
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			syswait.iowait++;
			sleep((caddr_t)bp, PRIBIO+1);
			syswait.iowait--;
			goto loop;
		}
		spl0();
		bp->b_flags &= ~B_AGE;
		notavail(bp);
		return bp;
	}

	bp = getfreeblk(bsize);
	if (bp == NULL)
		goto loop;
     found:
	bp->b_flags = B_KERNBUF | B_BUSY;
	bp->b_forw = dp->b_forw;
	bp->b_back = dp;
	dp->b_forw->b_back = bp;
	dp->b_forw = bp;
	bp->b_edev = dev;
	bp->b_dev = (o_dev_t)cmpdev(dev);
	bp->b_blkno = blkno;
	bp->b_bcount = bsize;
	return bp;
}

/*
 * get an empty block,
 * not assigned to any particular device.
 */
struct buf *
ngeteblk(bsize)
	long bsize;
{
	register struct buf *bp;
	register struct buf *dp;

loop:
	dp = &bfreelist;

	bp = getfreeblk(bsize);
	if (bp == NULL)
		goto loop;

found:
	bp->b_flags = B_KERNBUF | B_BUSY | B_AGE;
	bp->b_forw = dp->b_forw;
	bp->b_back = dp;
	dp->b_forw->b_back = bp;
	dp->b_forw = bp;
	bp->b_dev = (o_dev_t)NODEV;
	bp->b_edev = (dev_t)NODEV;
	bp->b_bcount = bsize;
	return bp;
}

/* 
 * Interface of geteblk() is kept intact to maintain driver compatibility.
 * Use ngeteblk() to allocate block size other than 1 KB.
 */
struct buf *
geteblk()
{
	return ngeteblk((long)1024);
}


/*
 * Wait for I/O completion on the buffer; return errors
 * to the user.
 */
int
iowait(bp)
	struct buf *bp;
{
	return biowait(bp);
}

/*
 * Mark I/O complete on a buffer, release it if I/O is asynchronous,
 * and wake up anyone waiting for it.
 */
void
iodone(bp)
	struct buf *bp;
{
	biodone(bp);
}

/*
 * Zero the core associated with a buffer.
 */
void
clrbuf(bp)
	struct buf *bp;
{
	bzero((caddr_t)bp->b_un.b_words, bp->b_bcount);
	bp->b_resid = 0;
}

/*
 * Make sure all write-behind blocks on dev (or NODEV for all)
 * are flushed out.
 */
void
bflush(dev)
	register dev_t dev;
{
	register struct buf *bp;
	register int s;

loop:
	s = spl6();
	for (bp = bfreelist.av_forw; bp != &bfreelist; bp = bp->av_forw) {
		if ((bp->b_flags & B_DELWRI)
		  && (dev == NODEV || dev == bp->b_edev)) {
			bp->b_flags |= B_ASYNC;
			notavail(bp);
			bwrite(bp);
			(void) splx(s);
			goto loop;
		}
	}
	(void) splx(s);
}

/*
 * Ensure that a specified block is up-to-date on disk.
 */
void
blkflush(dev, blkno, bsize)
	dev_t dev;
	daddr_t blkno;
	int bsize;
{
	register struct buf *bp, *dp;
	int s;

	blkno = LTOPBLK(blkno, bsize);
	dp = bhash(dev, blkno);
loop:
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
		if (bp->b_blkno != blkno || bp->b_edev != dev
		  || (bp->b_flags & B_STALE))
			continue;
		s = spl6();
		if (bp->b_flags & B_BUSY) {
			bp->b_flags |= B_WANTED;
			syswait.iowait++;
			(void) sleep((caddr_t) bp, PRIBIO+1);
			syswait.iowait--;
			(void) splx(s);
			goto loop;
		}
		if (bp->b_flags & B_DELWRI) {
			(void) splx(s);
			notavail(bp);
			bwrite(bp);
			goto loop;
		}
		(void) splx(s);
	}
}

/*
 * Wait for asynchronous writes to finish.
 */
void
bdwait()
{
	register int s;

	s = spl6();
	while (basyncnt) {
		basynwait = 1;
		sleep((caddr_t)&basyncnt, PRIBIO);
	}
	splx(s);
}

/*
 * Invalidate blocks for a dev after last close.
 */
void
binval(dev)
	register dev_t dev;
{
	register struct buf *dp;
	register struct buf *bp;
	register i;

	for (i = 0; i < v.v_hbuf; i++) {
		dp = (struct buf *)&hbuf[i];
		for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
			if (bp->b_edev == dev)
				bp->b_flags |= B_STALE|B_AGE;
	}
}

/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device hash buffer lists to empty.
 */
void
binit()
{
	register struct buf *bp;
	register struct buf *dp;
	register unsigned i;
		
	/*
	 * Change buffer memory usage high-water-mark from kbytes
	 * to bytes.
	 */
	bfreelist.b_bufsize = v.v_bufhwm * 1024;

	dp = &bfreelist;
	dp->b_forw = dp->b_back = dp->av_forw = dp->av_back = dp;
	bhdrlist.av_forw = bp = buf;

	for (i = 0; i < v.v_buf-1; i++,bp++) {
		bp->b_dev = (o_dev_t)NODEV;
		bp->b_edev = (dev_t)NODEV;
		bp->b_un.b_addr = NULL;
		bp->av_forw = bp + 1;
		bp->b_flags = B_KERNBUF;
		bp->b_bcount = 0;
	}
	bp->av_forw = NULL;
	pfreecnt = v.v_pbuf;
	pfreelist.av_forw = bp = pbuf;
	for (; bp < &pbuf[v.v_pbuf-1]; bp++)
		bp->av_forw = bp+1;
	bp->av_forw = NULL;
	for (i = 0; i < v.v_hbuf; i++)
		hbuf[i].b_forw = hbuf[i].b_back = (struct buf *)&hbuf[i];
}

/*
 * Wait for I/O completion on the buffer; return error code.
 * If bp was for synchronous I/O, bp is invalid and associated
 * resources are freed on return.
 */
int
biowait(bp)
	register struct buf *bp;
{
	int error = 0, s;

	syswait.iowait++;
	s = spl6();
	while ((bp->b_flags & B_DONE) == 0) {
		curproc->p_swlocks++;
		curproc->p_flag |= SSWLOCKS;
		bp->b_flags |= B_WANTED;
		(void) sleep((caddr_t)bp, PRIBIO);
	}
	if(--curproc->p_swlocks == 0)
		curproc->p_flag &= ~SSWLOCKS;
	(void) splx(s);
	syswait.iowait--;
	error = geterror(bp);

	if ((bp->b_flags & B_ASYNC) == 0) {
		if (bp->b_flags & B_PAGEIO) {
			pvn_done(bp);
		}
		else if (bp->b_flags & B_REMAPPED)
			bp_mapout(bp);
	}
	return error;
}

/*
 * Mark I/O complete on a buffer, release it if I/O is asynchronous,
 * and wake up anyone waiting for it.
 */
void
biodone(bp)
	register struct buf *bp;
{
	if (bp->b_iodone && (bp->b_flags & B_KERNBUF)) {
		(*(bp->b_iodone))(bp);
		return;
	}
	ASSERT((bp->b_flags & B_DONE) == 0);
	bp->b_flags |= B_DONE;
	if (bp->b_flags & B_ASYNC) {
		if ((bp->b_flags & B_READ) == 0)
			basyncnt--;
		if (basyncnt == 0 && basynwait) {
			basynwait = 0;
			wakeup((caddr_t)&basyncnt);
		}
		if (bp->b_flags & (B_PAGEIO|B_REMAPPED))
			swdone(bp);
		else
			brelse(bp);		/* release bp to 1k freelist */
	} else {
		bp->b_flags &= ~B_WANTED;
		wakeup((caddr_t)bp);
	}
	return;
}

/*
 * Pick up the device's error number and pass it to the user;
 * if there is an error but the number is 0 set a generalized code.
 */

#undef geterror

int
geterror(bp)
	register struct buf *bp;
{
	int error = 0;

	if ((bp->b_flags & B_ERROR)
	  && (error = bp->b_error) == 0 && (error = bp->b_oerror) == 0)
		error = EIO;
	return error;
}

/*
 * Support for pageio buffers.
 *
 * This stuff should be generalized to provide a generalized bp
 * header facility that can be used for things other than pageio.
 */

/*
 * Pageio_out is a list of all the buffers currently checked out
 * for pageio use.
 */
STATIC struct bufhd pageio_out = {
	B_HEAD,
	(struct buf *)&pageio_out,
	(struct buf *)&pageio_out,
};

#define	NOMEMWAIT() (u.u_procp == nproc[2])

/*
 * Allocate and initialize a buf struct for use with pageio.
 */
struct buf *
pageio_setup(pp, len, vp, flags)
	struct page *pp;
	u_int len;
	struct vnode *vp;
	int flags;
{
	register struct buf *bp;

        bp = (struct buf *) kmem_zalloc(sizeof (*bp),
          NOMEMWAIT() ? KM_NOSLEEP : KM_SLEEP);

        if (bp == NULL) {
                /*
                 * We are pageout and cannot risk sleeping for more
                 * memory so we return an error condition instead.
                 */
                return NULL;
        }         

	binshash(bp, (struct buf *)&pageio_out);
	bp->b_un.b_addr = 0;
	bp->b_error = 0;
	bp->b_oerror = 0;
	bp->b_resid = 0;
	bp->b_bcount = len;
	bp->b_bufsize = len;
	bp->b_pages = pp;
	bp->b_flags = B_KERNBUF | B_PAGEIO | B_NOCACHE | B_BUSY | flags;

	VN_HOLD(vp);
	bp->b_vp = vp;

	/*
	 * This count is bumped for async writes here and in
	 * bwrite().
	 */
	if ((flags & (B_ASYNC|B_READ)) == B_ASYNC)
		basyncnt++;

	/*
	 * Caller sets dev & blkno and can adjust
	 * b_addr for page offset and can use bp_mapin
	 * to make pages kernel addressable.
	 */
	return bp;
}

void
pageio_done(bp)
	register struct buf *bp;
{

	if (bp->b_flags & B_REMAPPED)
		bp_mapout(bp);
	bremhash(bp);
	VN_RELE(bp->b_vp);
	kmem_free((caddr_t)bp, sizeof (*bp));
}
	
/*
 * Break up the request that came from bread/bwrite into chunks of
 * contiguous memory so we can get around the DMAC limitations
 */

/*
 * Determine number of bytes to page boundary.
 */
#define	pgbnd(a)	(NBPP - ((NBPP - 1) & (int)(a)))

void
buf_breakup(strat, obp)
	int (*strat)();
	register struct buf *obp;
{
	register int cc, iocount, s;
	register struct buf *bp;	
	
/*	ASSERT((obp->b_flags & B_PAGEIO)== NULL); */
	bp = (struct buf *)kmem_zalloc(sizeof (*bp), KM_SLEEP);
	bcopy((caddr_t)obp, (caddr_t)bp, sizeof(*bp));
	iocount = obp->b_bcount;
	bp->b_flags &= ~B_ASYNC;

	/*
	 * The buffer is on a sector boundary but not necessarily
	 * on a page boundary.
	 */
	if ((bp->b_bcount = cc = 
	  min(iocount, pgbnd(bp->b_un.b_addr))) < NBPP) {
		/*
		 * Do the fragment of the buffer that's in the
		 * first page.
		 */
		bp->b_flags &= ~B_DONE;
		(*strat)(bp);
		s = spl6();
		while ((bp->b_flags & B_DONE) == 0) {
			bp->b_flags |= B_WANTED;
			sleep((caddr_t)bp, PRIBIO);
		}
		(void) splx(s);
		if (bp->b_flags & B_ERROR) {
			goto out;
		}
		bp->b_blkno += btod(cc);
		bp->b_un.b_addr += cc;
		iocount -= cc;
	}

	/*
	 * Now do the DMA a page at a time.
	 */
	while (iocount > 0) {
		bp->b_bcount = cc = min(iocount, NBPP);
		bp->b_flags &= ~B_DONE;
		(*strat)(bp);
		s = spl6();
		while ((bp->b_flags & B_DONE) == 0) {
			bp->b_flags |= B_WANTED;
			sleep((caddr_t)bp, PRIBIO);
		}
		(void) splx(s);
		if (bp->b_flags & B_ERROR) {
			goto out;
		}
		bp->b_blkno += btod(cc);
		bp->b_un.b_addr += cc;
		iocount -= cc;
	}
	kmem_free((caddr_t)bp, sizeof(*bp));
	s = spl6();
	biodone(obp);
	splx(s);
	return;
out:
	if (bp->b_error)
		obp->b_error = bp->b_error;
	else if (bp->b_oerror)
		obp->b_error = bp->b_oerror;
	obp->b_flags |= B_ERROR;
	kmem_free((caddr_t)bp, sizeof(*bp));
	s = spl6();
	biodone(obp);
	splx(s);
	return;
}

/* 
 * Debugging print of buffer headers.  Can be invoked from kernel debugger.
 */
#ifdef DEBUG

STATIC void
printbuf()
{
	struct buf *bp;
	register i;

	printf("bufno		av_f		av_b		forw	back	flag	count\n");
	bp = &bfreelist;
	printf("%x\n", bp);
	printf("freelist	%x	%x	%x	%x	%d\n",
	  bp->av_forw, bp->av_back,bp->b_forw, bp->b_back,
	  bp->b_bcount);
	bp = &bhdrlist;
	printf("%x\n", bp);
	printf("bhdrlist	%x\n", bp->av_forw);

	for (i = 0, bp = buf; i < v.v_buf; i++, bp++) {
		printf("%x	%x	%x	%x	%x	%x	%d\n",
		  bp, 
		  bp->av_forw, bp->av_back, bp->b_forw, bp->b_back,
		  bp->b_flags,
		  bp->b_bufsize);
	}
}

#endif
