/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/s5/s5rdwri.c	1.18"
#include "sys/types.h"
#include "sys/buf.h"
#include "sys/conf.h"
#include "sys/debug.h"
#include "sys/errno.h"
#include "sys/file.h"
#include "sys/param.h"
#include "sys/swap.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/uio.h"
#include "sys/vfs.h"
#include "sys/vnode.h"

#include "sys/proc.h"	/* XXX -- needed for user-context kludge in ILOCK */
#include "sys/disp.h"

#include "sys/fs/s5param.h"
#include "sys/fs/s5inode.h"
#include "sys/fs/s5macros.h"

#include "vm/seg_kmem.h"
#include "vm/seg_map.h"
#include "vm/seg.h"

#include "sys/cmn_err.h"
#include "sys/kmem.h"

/*
 * Package the arguments into a uio structure and invoke readi()
 * or writei(), as appropriate.
 */
int
rdwri(rw, ip, base, len, offset, seg, ioflag, aresid)
	enum uio_rw rw;
	struct inode *ip;
	caddr_t base;
	int len;
	off_t offset;
	enum uio_seg seg;
	int ioflag;
	int *aresid;
{
	struct uio auio;
	struct iovec aiov;
	int error;

	aiov.iov_base = base;
	auio.uio_resid = aiov.iov_len = len;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_segflg = (short)seg;
	auio.uio_limit = offset + NBPSCTR;
	if (rw == UIO_WRITE) {
		auio.uio_fmode = FWRITE;
		error = writei(ip, &auio, ioflag);
	} else {
		auio.uio_fmode = FREAD;
		error = readi(ip, &auio, ioflag);
	}
	if (aresid)
		*aresid = auio.uio_resid;
	return error;
}

/*
 * Read the file corresponding to the supplied inode.
 */
/* ARGSUSED */
int
readi(ip, uiop, ioflag)
	register struct inode *ip;
	register struct uio *uiop;
	int ioflag;
{
	register unsigned int on, n;
	int mode, flags, error = 0;
	off_t off;
	caddr_t base;
	struct vnode *vp = ITOV(ip);

	mode = ip->i_mode;
	if (MANDLOCK(vp, mode)
	  && (error = chklock(vp, FREAD,
	    uiop->uio_offset, uiop->uio_resid, uiop->uio_fmode)))
		return error;
	if (uiop->uio_resid == 0)
		return 0;
	if (uiop->uio_offset < 0)
		return EINVAL;
	do {
		/*
		 * Prepare to map in a MAXBSIZE chunk of the file for I/O.
		 * Compute n, the number of bytes which can be read from
		 * this mapping.
		 */
		off = uiop->uio_offset & MAXBMASK;
		on = uiop->uio_offset & MAXBOFFSET;
		n = MIN(MAXBSIZE-on, uiop->uio_resid);
		n = (ip->i_size < uiop->uio_offset) ?
		  0 : MIN(n, ip->i_size - uiop->uio_offset);
		if (n == 0)
			break;
		base = segmap_getmap(segkmap, vp, off);
		flags = 0;
		if ((error = uiomove(base+on, n, UIO_READ, uiop)) == 0) {
			/*
			 * If we read to the end of the mapping or to
			 * EOF, we won't need these pages again soon.
			 */
			if (n + on == MAXBSIZE
			  || uiop->uio_offset == ip->i_size)
				flags |= SM_DONTNEED;
			error = segmap_release(segkmap, base, flags);
			ip->i_flag |= IACC;
		} else
			(void) segmap_release(segkmap, base, 0);
	} while (error == 0 && uiop->uio_resid > 0);

	return error;
}

/*
 * Write the file corresponding to the specified inode.
 */
int
writei(ip, uiop, ioflag)
	register struct inode *ip;
	register struct uio *uiop;
	int ioflag;
{
	register struct vnode *vp = ITOV(ip);
	register struct s5vfs *s5vfsp = S5VFS(vp->v_vfsp);
	register unsigned int n, on;
	off_t off;
	daddr_t firstlbn, lastlbn;
	caddr_t base;
	unsigned long int limit = uiop->uio_limit;
	unsigned long int oresid = uiop->uio_resid;
	int mode = ip->i_mode, error = 0, flags, pagecreate;
	int bsize = VBSIZE(vp);
	int alloc_only;
	off_t osize;
	int bcnt, index;
	daddr_t dblist[MAXBSIZE/NBPSCTR];

	if (MANDLOCK(vp, mode)
	  && (error = chklock(vp, FWRITE,
	    uiop->uio_offset, uiop->uio_resid, uiop->uio_fmode)))
		return error;
	if (uiop->uio_offset < 0)
		return EINVAL;

	ip->i_flag |= INOACC;	/* Don't update access time in getpage() */
	if (ioflag & IO_SYNC)
		ip->i_flag |= ISYNC;

	while (error == 0 && uiop->uio_resid > 0) {
		if (vp->v_type == VREG && uiop->uio_offset >= limit) {
			error = EFBIG;
			goto err;
		}
		if ((vp->v_type == VREG || vp->v_type == VDIR)
		  && uiop->uio_offset > ip->i_size
		  && ip->i_map) {
			ILOCK(ip);
			s5freemap(ip);
			IUNLOCK(ip);
		}
		off = uiop->uio_offset & MAXBMASK;
		on = uiop->uio_offset & MAXBOFFSET;
		n = MIN(MAXBSIZE-on, uiop->uio_resid);
		base = segmap_getmap(segkmap, vp, off);
		/*
		 * We must ensure that any file blocks are allocated before
		 * we perform the I/O.
		 */
		firstlbn = uiop->uio_offset >> s5vfsp->vfs_bshift;
		lastlbn = (uiop->uio_offset + n - 1) >> s5vfsp->vfs_bshift;
		if (uiop->uio_offset + n > ip->i_size)
			alloc_only = (on % PAGESIZE == 0);
		else
			alloc_only = (on % PAGESIZE == 0 && n % PAGESIZE == 0);

		error = bmapalloc(ip, firstlbn, lastlbn, alloc_only, &dblist[0]);
		if (error) {
			(void) segmap_release(segkmap, base, 0);
			goto err;
		}
		osize = ip->i_size;
		if (uiop->uio_offset + n > ip->i_size) {
			ip->i_size = uiop->uio_offset + n;
			/*
			 * If we are writing from the beginning of a
			 * page, we can just create the pages without
			 * having to read them in.
			 */
			if (on % PAGESIZE == 0) {
				segmap_pagecreate(segkmap, base+on,
				  (u_int) n, 0, &dblist[0], bsize);
				pagecreate = 1;
			} else
				pagecreate = 0;
		} else if (on % PAGESIZE == 0 && n % PAGESIZE == 0) {
			/*
			 * We're writing an exact number of pages, so we can
			 * can just create them without having to read them in.
			 */
			segmap_pagecreate(segkmap, base+on, (u_int) n, 0,
				&dblist[0], bsize);
			pagecreate = 1;
		} else
			pagecreate = 0;

		/* 
		 * This loop has a lot of activities going on
		 * so we preempt here to let other proceses have
		 * a chance.
		 */
		PREEMPT();
		error = uiomove(base+on, n, UIO_WRITE, uiop);

		if (pagecreate 
		  && uiop->uio_offset < roundup(off + on + n, PAGESIZE)) {
			/*
			 * We created pages without initializing them
			 * completely, thus we need to zero the part
			 * that wasn't set up.  This happens on most
			 * EOF write cases and if we had some sort of
			 * error during the uiomove.
			 */
			int nzero, nmoved;

			nmoved = uiop->uio_offset - (off + on);
			ASSERT(nmoved >= 0 && nmoved <= n);
			nzero = roundup(on + n, PAGESIZE) - (on + nmoved);
			ASSERT(nzero >= 0 && on + nmoved + nzero <= MAXBSIZE);
			(void) kzero(base + on + nmoved, (u_int) nzero);
		}
		if (error) {
			/*
			 * We may have already allocated file blocks as well
			 * as pages.  It's hard to undo the block allocation,
			 * but we must be sure to invalidate any pages that
			 * may have been allocated.
			 */
			(void) segmap_release(segkmap, base, SM_INVAL);
			ip->i_size = osize;
		} else {
			flags = 0;
			if (ioflag & IO_SYNC) {
				if (IS_SWAPVP(vp))
					flags = SM_WRITE|SM_FREE|SM_DONTNEED;
				else {
					ip->i_flag |= ISYN;
					flags = SM_WRITE;
				}
			} else if (n + on == MAXBSIZE || IS_SWAPVP(vp))
				/*
				 * Have written a whole chunk.  Start an
				 * asynchronous write and mark the buffer
				 * to indicate that it won't be needed
				 * again soon.
				 */
				flags = SM_WRITE|SM_ASYNC|SM_DONTNEED;
			error = segmap_release(segkmap, base, flags);
		}
		if (error == 0) {
			if (ip->i_map) {
				ILOCK(ip);
				s5freemap(ip);
				IUNLOCK(ip);
			}
			ip->i_flag |= IUPD|ICHG;
		}
	}

	ip->i_flag &= ~(ISYNC | INOACC);	
	return error;

err:
	/*
	 * If we've already done a partial write, terminate
	 * the write but return no error.
	 */
	if (oresid != uiop->uio_resid)
		error = 0;
	ip->i_flag &= ~(ISYNC|INOACC);	
	return error;

}
