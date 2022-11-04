/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/s5/s5bmap.c	1.12"
#include "sys/types.h"
#include "sys/buf.h"
#include "sys/debug.h"
#include "sys/errno.h"
#include "sys/fbuf.h"
#include "sys/file.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/vfs.h"
#include "sys/vnode.h"

#include "sys/proc.h"	/* XXX -- needed for user-context kludge in ILOCK */
#include "sys/disp.h"	/* XXX */

#include "sys/fs/s5param.h"
#include "sys/fs/s5inode.h"
#include "sys/fs/s5macros.h"
#include "vm/seg.h"
#include "vm/page.h"

/*
 * bmap defines the structure of file system storage by mapping
 * a logical block number in a file to a physical block number
 * on the device.  It should be called with a locked inode when
 * allocation is to be done.
 *
 * bmap translates logical block number lbn to a physical block
 * number and returns it in *bnp, possibly along with a read-ahead
 * block number in *rabnp.  bnp and rabnp can be NULL if the
 * information is not required.  rw specifies whether the mapping
 * is for read or write.  If alloc_only is set, bmap may not create
 * any in-core pages that correspond to the new disk allocation.
 * Otherwise, the in-core pages will be created and initialized as
 * needed.
 *
 * Returns 0 on success, or a non-zero errno if an error occurs.
 */
int
bmap(ip, lbn, bnp, rabnp, rw, alloc_only)
	register inode_t *ip;	/* file to be mapped */
	daddr_t lbn;		/* logical block number */
	enum seg_rw rw;		/* S_READ, S_WRITE, or S_OTHER */
	daddr_t *bnp;		/* mapped block number */
	daddr_t *rabnp;		/* read-ahead block */
	int alloc_only;		/* allocate disk blocks but create no pages */
{
	register daddr_t bn = lbn;
	register struct vnode *vp = ITOV(ip);
	register int bsize = VBSIZE(vp);
	register int i, j;
	dev_t dev = ip->i_dev;
	daddr_t nb, inb;
	struct buf *bp;
	register sh;
	daddr_t *bap;
	int nshift, error;
	int isdir, issync;
	struct vfs *vfsp = vp->v_vfsp;
	struct s5vfs *s5vfsp = S5VFS(vfsp);
	struct vnode *devvp = s5vfsp->vfs_devvp;
	struct fbuf *fbp;
	int blkalloc();
	int nblks;
	struct page *pp;
	int poffset, blkpp, ppblk;

	if (bnp)
		*bnp = S5_HOLE;
	if (rabnp)
		*rabnp = S5_HOLE;

	if (ip->i_map) {
		nblks = (ip->i_size + bsize - 1)/bsize;
		if (lbn >= nblks) {
			if (bnp)
				*bnp = S5_HOLE;
			if (rabnp)
				*rabnp = S5_HOLE;
			return 0;
		}
		if (bnp) {
			*bnp = ip->i_map[lbn];
			if (*bnp == 0)
				*bnp = S5_HOLE;
		}
		if (rabnp) {
			if ((lbn + 1) >= nblks) {
				*rabnp = S5_HOLE;
			} else {
				*rabnp = ip->i_map[lbn+1];
				if (*rabnp == 0)
					*rabnp = S5_HOLE;
			}
		}
		return 0;
	}
	isdir = (vp->v_type == VDIR);
	issync = ((ip->i_flag & ISYNC) != 0);
	if (isdir || issync)
		alloc_only = 0;		/* make sure */

	/*
	 * Blocks 0..NADDR-4 are direct blocks.
	 */
	if (bn < NADDR-3) {
		i = bn;
		if ((nb = ip->i_addr[i]) == 0) {
			if (rw != S_WRITE) {
				if (bnp)
					*bnp = S5_HOLE;
				return 0;
			}
			/*
			 * Unless "alloc_only" indicates otherwise, we have
			 * to get or create the page containing this block
			 * and clear the portion of the page that corresponds
			 * to it--but ONLY that portion, in case there are
			 * other blocks in this page whiich have already
			 * been allocated.  We do this by mapping in the
			 * file using fbread(); this will fault in the
			 * page (if necessary) using s5getpage(), which will
			 * automatically clear any unused portion.
			 *
			 * In order to avoid doing unnecessary disk I/O for
			 * the new block, we map in the file BEFORE we
			 * allocate the file block, so that the consequent
			 * call to s5getpage() will find a hole in the file,
			 * and clear it.
			 *
			 * Note that this results in a recursive call to
			 * bmap() (invoked by s5getpage()) in searching for
			 * this block.  This seems a little weird, but should
			 * be okay since we're not holding any locked resources
			 * at this point.  Note also that the call to fbread()
			 * MUST specify S_OTHER rather than S_WRITE here, since
			 * we don't want s5getpage() to allocate the block.
			 */
			fbp = NULL;
			if (alloc_only == 0
			  && (error = fbread(vp, lbn << s5vfsp->vfs_bshift,
			    bsize, S_OTHER, &fbp)))
				return error;
			if (error = blkalloc(vfsp, &nb)) {
				if (fbp)
					fbrelse(fbp, S_OTHER);
				return error;
			}

			if (alloc_only == 0) { /* fbread did get called */
				blkpp = PAGESIZE/bsize;
				if (blkpp >= 1) {
					poffset = (lbn << s5vfsp->vfs_bshift) & PAGEMASK;
					pp = page_find(vp, poffset);
					ASSERT(pp != NULL);
					pp->p_dblist[lbn % blkpp] = nb;
				} else {
					ppblk = bsize/PAGESIZE;
					poffset = lbn << s5vfsp->vfs_bshift;
					for (i=0; i<ppblk; i++) {
						poffset = poffset + (i * PAGESIZE);
						pp = page_find(vp, poffset);
						ASSERT(pp != NULL);
						pp->p_dblist[0] = nb;
					}
				}
			}
			/*
			 * Write directory blocks synchronously so that they
			 * never appear with garbage in them on the disk.
			 */
			if (isdir)
				(void) fbiwrite(fbp, devvp, nb, bsize);
			else if (fbp)
				fbrelse(fbp, S_WRITE);
			ip->i_addr[i] = nb;
			ip->i_flag |= IUPD|ICHG;
		}
		if (rabnp && i < NADDR-4)
			*rabnp = (ip->i_addr[i+1] == 0) ?
			  S5_HOLE : ip->i_addr[i+1];
		if (bnp)
			*bnp = nb;
		return 0;
	}

	/*
	 * Addresses NADDR-3, NADDR-2, and NADDR-1 have single, double,
	 * triple indirect blocks.  The first step is to determine how
	 * many levels of indirection.
	 */
	nshift = s5vfsp->vfs_nshift;
	sh = 0;
	nb = 1;
	bn -= NADDR-3;
	for (j = 3; j > 0; j--) {
		sh += nshift;
		nb <<= nshift;
		if (bn < nb)
			break;
		bn -= nb;
	}
	if (j == 0)
		return EFBIG;

	/*
	 * Fetch the address from the inode.
	 */
	if ((inb = ip->i_addr[NADDR-j]) == 0) {
		if (rw != S_WRITE) {
			if (bnp)
				*bnp = S5_HOLE;
			return 0;
		}
		if (error = blkalloc(vfsp, &inb))
			return error;
		/*
		 * Zero and synchronously write indirect blocks
		 * so that they never point at garbage.
		 */
		bp = getblk(dev, inb, bsize);
		clrbuf(bp);
		bwrite(bp);
		ip->i_addr[NADDR-j] = inb;
		ip->i_flag |= IUPD|ICHG;
	}

	/*
	 * Fetch through the indirect blocks.
	 */
	for (; j <= 3; j++) {
		if ((bp = bread(dev, inb, bsize))->b_flags & B_ERROR) {
			if ((error = bp->b_error) == 0
			  && (error = bp->b_oerror) == 0)
				error = EIO;
			brelse(bp);
			return error;
		}
		bap = bp->b_un.b_daddr;
		sh -= nshift;
		i = (bn >> sh) & s5vfsp->vfs_nmask;
		if ((nb = bap[i]) == 0) {
			struct buf *nbp;

			/*
			 * As in the direct-block case (above), we want to
			 * fault in any necessary pages before allocating
			 * the file block.  But in this case we're holding
			 * a buffer pointer which will also be needed by the
			 * recursive invocation of bmap().  Thus we have to
			 * release the bp now and re-acquire it later.
			 */
			brelse(bp);
			if (rw != S_WRITE) {
				if (bnp)
					*bnp = S5_HOLE;
				return 0;
			}
			if (j < 3) {
				/*
				 * Indirect block.
				 */
				if (error = blkalloc(vfsp, &nb))
					return error;
				nbp = getblk(dev, nb, bsize);
				clrbuf(nbp);
				bwrite(nbp);
			} else if (alloc_only == 0) {
				int bshift = s5vfsp->vfs_bshift;

				if (error = fbread(vp, lbn << bshift,
				  bsize, S_OTHER, &fbp))
					return error;
				if (error = blkalloc(vfsp, &nb)) {
					fbrelse(fbp, S_OTHER);
					return error;
				}

				blkpp = PAGESIZE/bsize;
				if (blkpp >= 1) {
					poffset = (lbn << bshift) & PAGEMASK;
					pp = page_find(vp, poffset);
					ASSERT(pp != NULL);
					pp->p_dblist[lbn % blkpp] = nb;
				} else {
					ppblk = bsize/PAGESIZE;
					poffset = lbn << bshift;
					for (i=0; i<ppblk; i++) {
						poffset = poffset + (i * PAGESIZE);
						pp = page_find(vp, poffset);
						ASSERT(pp != NULL);
						pp->p_dblist[0] = nb;
					}
				}
				/*
				 * We cause the new (zero) data to be
				 * synchronously written if (a) we're
				 * writing a directory, or (b) we're
				 * doing a user-requested synchronous
				 * write and we're filling in a hole in
				 * the middle of the file, in which case
				 * we want the data to appear before the
				 * indirect block which will be written
				 * synchronously below.
				 */
				if (isdir
				  || (issync && (bn << bshift) < ip->i_size))
					(void) fbiwrite(fbp, devvp, nb, bsize);
				else
					fbrelse(fbp, S_WRITE);
			} else if (error = blkalloc(vfsp, &nb))
				return error;

			/*
			 * Now reacquire the bp so that the new block can
			 * can be recorded.
			 */
			if ((bp = bread(dev, inb, bsize))->b_flags & B_ERROR) {
				if ((error = bp->b_error) == 0
				  && (error = bp->b_oerror) == 0)
					error = EIO;
				brelse(bp);
				return error;
			}
			bap = bp->b_un.b_daddr;

			bap[i] = nb;
			if (issync)
				bwrite(bp);
			else
				bdwrite(bp);
		} else
			brelse(bp);

		inb = nb;
	}

	/*
	 * Calculate read-ahead.
	 */
	if (rabnp && i < s5vfsp->vfs_nindir-1)
		*rabnp = (bap[i+1] == 0) ? S5_HOLE : bap[i+1];
	if (bnp)
		*bnp = nb;
	return 0;
}

/*
 * Allocate all the blocks in the range [first, last].
 */
int
bmapalloc(ip, first, last, alloc_only, dblist)
	struct inode *ip;
	daddr_t first;
	daddr_t last;
	int alloc_only;
	daddr_t *dblist;
{
	daddr_t lbn, pbn;
	daddr_t *dbp;
	int error = 0;

	dbp = dblist;
	ILOCK(ip);
	for (lbn = first; error == 0 && lbn <= last; lbn++) {
		error = bmap(ip, lbn,
		  (daddr_t *)&pbn, (daddr_t *)NULL, S_WRITE, alloc_only);
		if (error)
			break;
		if (dbp != NULL)
			*dbp++ = pbn;
	}
	IUNLOCK(ip);
	return error;
}
