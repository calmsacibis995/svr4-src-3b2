/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/fsflush.c	1.7"

#include "sys/types.h"
#include "sys/param.h"
#include "sys/tuneable.h"
#include "sys/inline.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/user.h"
#include "sys/var.h"
#include "sys/buf.h"
#include "sys/vfs.h"
#include "sys/cred.h"
#include "sys/kmem.h"
#include "sys/vnode.h"
#include "sys/swap.h"
#include "sys/vm.h"
#include "sys/debug.h"
#include "sys/cmn_err.h"
#include "sys/sysinfo.h"

#include "vm/hat.h"
#include "vm/page.h"
#include "vm/pvn.h"

int dopageflush = 1;	/* must be non-zero to turn page flushing on */
extern struct buf bhdrlist;

/*
 * As part of file system hardening, this daemon is waken
 * every second to flush cached data which includes the
 * buffer cache, the inode cache and mapped pages.
 */
void
fsflush()
{
	register struct buf *bp;
	register autoup;
	register struct page *pp = pages;
	int i, s, nscan, pcount, count = 0;


	autoup = v.v_autoup * HZ;
	ASSERT(v.v_autoup > 0);
	nscan = (epages-pages)/v.v_autoup;
loop:
	s = spl6();
	for (bp = bfreelist.av_forw; bp != &bfreelist; bp = bp->av_forw) {
		if ((bp->b_flags & B_DELWRI) && lbolt - bp->b_start >= autoup) {
			bp->b_flags |= B_ASYNC;
			notavail(bp);
			bwrite(bp);
			(void) splx(s);
			goto loop;
		}
	}

	/*
	 * To prevent getfreeblk from hanging for a long
	 * period of time (up to autoup seconds), free one
	 * buffer from the buffer cache when someone is
	 * waiting for a free buffer header.
	 */
	if (bhdrlist.b_flags & B_WANTED) {
		for (bp = bfreelist.av_forw;bp != &bfreelist;bp = bp->av_forw) {
			if ((bp->b_flags & B_DELWRI) == 0) {
				notavail(bp);			
				bp->av_forw = bp->av_back = NULL;
				bremhash(bp);
				kmem_free(bp->b_un.b_addr, (int)bp->b_bufsize);
				bfreelist.b_bufsize += bp->b_bufsize;
				struct_zero((caddr_t)bp, sizeof(struct buf));
				bp->b_flags |= B_KERNBUF;
				bp->av_forw = bhdrlist.av_forw;
				bhdrlist.av_forw = bp;
				bhdrlist.b_flags &= ~B_WANTED;
				wakeup((caddr_t)&bhdrlist);
				break;
			}

		}
	}

	/*
	 * Every autoup times through the loop, flush cached attribute
	 * information (e.g. inodes).
	 */
	if (count++ >= v.v_autoup) {
		count = 0;

		/*
		 * Sync back cached data.
		 */
		for (i = 1; i < nfstype; i++)
			(void)(*vfssw[i].vsw_vfsops->vfs_sync)(NULL,
				SYNC_ATTR, u.u_cred);
	}

	if (!dopageflush)
		goto out;

	/*
	 * Flush dirty pages.
	 */
	pcount = 0;
	while (pcount++ <= nscan) {
		/*
		 * Reject pages that don't make sense to free.
		 */
		if (pp->p_lock || pp->p_free || pp->p_intrans || 
		    pp->p_lckcnt > 0 || pp->p_cowcnt > 0 ||
		    pp->p_keepcnt > 0 || !pp->p_vnode ||
		    IS_SWAPVP(pp->p_vnode) ||
		    pp->p_vnode->v_type == VCHR) {
			if (++pp >= epages)
				pp = pages;
			continue;
		}

		/*
		 * hat_pagesync will turn off ref and mod bits loaded
		 * into the hardware.
		 */
		hat_pagesync(pp);

		if (pp->p_mod)
			VOP_PUTPAGE(pp->p_vnode, pp->p_offset,
			    PAGESIZE, B_ASYNC, (struct cred *)0);

		if (++pp >= epages)
			pp = pages;
	}

out:
	(void) splx(s);
	sleep((caddr_t)fsflush, PRIBIO);
	goto loop;
}


