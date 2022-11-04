/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/specfs/specvfsops.c	1.12"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/buf.h"
#include "sys/cmn_err.h"
#include "sys/debug.h"
#include "sys/errno.h"
#include "sys/vfs.h"
#include "sys/swap.h"
#include "sys/vnode.h"
#include "sys/cred.h"
#include "sys/fs/snode.h"

#include "fs/fs_subr.h"

STATIC	int spec_sync();

struct vfsops spec_vfsops = {
	fs_nosys,		/* mount */
	fs_nosys,		/* unmount */
	fs_nosys,		/* root */
	fs_nosys,		/* statvfs */
	spec_sync,
	fs_nosys,		/* vget */
	fs_nosys,		/* mountroot */
	fs_nosys,		/* swapvp */
	fs_nosys,		/* filler */
	fs_nosys,
	fs_nosys,
	fs_nosys,
};

/*
 * Run though all the snodes and force write-back
 * of all dirty pages on the block devices.
 */
/* ARGSUSED */
STATIC int
spec_sync(vfsp, flag, cr)
	struct vfs *vfsp;
	short flag;
	cred_t *cr;
{
	static int spec_lock;
	register struct snode **spp, *sp;
	register struct vnode *vp;

	if (spec_lock)
		return 0;

	spec_lock++;

	if (!(flag & SYNC_ATTR)) {
		for (spp = stable; spp < &stable[STABLESIZE]; spp++) {
			for (sp = *spp; sp != NULL; sp = sp->s_next) {
				vp = STOV(sp);
				/*
				 * Don't bother sync'ing a vp if it's
				 * part of a virtual swap device.
				 */
				if (IS_SWAPVP(vp))
					continue;
				if (vp->v_type == VBLK && vp->v_pages)
					(void) VOP_PUTPAGE(vp, 0, 0, B_ASYNC,
					    (cred_t *)0);
			}
		}
	}
	spec_lock = 0;
	return 0;
}
