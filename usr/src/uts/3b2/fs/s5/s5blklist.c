/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/s5/s5blklist.c	1.8"
#include "sys/types.h"
#include "sys/buf.h"
#include "sys/cmn_err.h"
#include "sys/conf.h"
#include "sys/cred.h"
#include "sys/debug.h"
#include "sys/errno.h"
#include "sys/fcntl.h"
#include "sys/file.h"
#include "sys/flock.h"
#include "sys/param.h"
#include "sys/stat.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/var.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/mode.h"
#include "sys/user.h"
#include "sys/kmem.h"

#include "vm/pvn.h"

#include "sys/proc.h"	/* XXX -- needed for user-context kludge in ILOCK */
#include "sys/disp.h"	/* XXX */

#include "sys/fs/s5param.h"
#include "sys/fs/s5fblk.h"
#include "sys/fs/s5filsys.h"
#include "sys/fs/s5ino.h"
#include "sys/fs/s5inode.h"
#include "sys/fs/s5macros.h"

#include "fs/fs_subr.h"

STATIC int	s5bldblklst();
STATIC int	*s5bldindr();

/*
 * Allocate and build the block address map.
 */
int
s5allocmap(ip)
	register struct inode *ip;
{
	register int	*bnptr;
	register int	bsize;
	register int	nblks;
	register struct vnode *vp = ITOV(ip);

	if (ip->i_map) 
		return 1;

	/*
	 * Get number of blocks to be mapped.
	 */
	ASSERT(ip->i_map == 0);
	bsize = VBSIZE(vp);
	nblks = (ip->i_size + bsize - 1)/bsize;
	bnptr = (int *)kmem_alloc(sizeof(int)*nblks, KM_NOSLEEP);
	if (bnptr == NULL)
		return 0;

	/*
	 * Build the actual list of block numbers for the file.
	 */
	(void) s5bldblklst(bnptr, ip, nblks);
	ip->i_map = bnptr;

	return 1;
}

/*
 * Build the list of block numbers for a file.  This is used
 * for mapped files.
 */
STATIC
int
s5bldblklst(lp, ip, nblks)
	register int		*lp;
	register struct inode	*ip;
	register int		nblks;
{
	register int	lim;
	register int	*eptr;
	register int	i;
	register struct vnode *vp = ITOV(ip);
	dev_t	 dev;

	/*
	 * Get the block numbers from the direct blocks first.
	 */
	eptr = &lp[nblks];
	lim = (nblks < NADDR-3) ? nblks : NADDR-3;
	
	for (i = 0; i < lim; i++)
		*lp++ = ip->i_addr[i];
	
	if (lp >= eptr)
		return 1;
	
	dev = vp->v_vfsp->vfs_dev;
	while (lp < eptr) {
		lp = s5bldindr(ip, lp, eptr, dev, ip->i_addr[i], i-(NADDR-3));
		if (lp == 0)
			return 0;
		i++;
	}
	return 1;
}

STATIC
int *
s5bldindr(ip, lp, eptr, dev, blknbr, indlvl)
	struct inode 		*ip;
	register int		*lp;
	register int		*eptr;
	register dev_t	dev;
	int			blknbr;
	int			indlvl;
{
	register struct buf *bp;
	register int	*bnptr;
	int		cnt;
	struct s5vfs	*s5vfsp;
	int 		bsize;
	struct vnode	*vp = ITOV(ip);

	bsize = VBSIZE(vp);
	bp = bread(dev, blknbr, bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return NULL;
	}
	bnptr = bp->b_un.b_words;
	s5vfsp = S5VFS(vp->v_vfsp);
	cnt = s5vfsp->vfs_nindir;
	
	ASSERT(indlvl >= 0);
	while (cnt-- && lp < eptr) {
		if (indlvl == 0)
			*lp++ = *bnptr++;
		else {
			lp = s5bldindr(ip, lp, eptr, dev, *bnptr++, indlvl-1);
			if (lp == 0) {
				brelse(bp);
				return NULL;
			}
		}
	}

	brelse(bp);
	return lp;
}

/*
 * Free the block list attached to an inode.
 */
s5freemap(ip)
	struct inode	*ip;
{
	register int	nblks;
	register	bsize;
	register struct vnode *vp = ITOV(ip);
	register int	*bnptr;

	ASSERT(ip->i_flag & ILOCKED);

	if (vp->v_type != VREG || ip->i_map == NULL)
		return 0;

	bsize = VBSIZE(vp);
	nblks = (ip->i_size + bsize - 1)/bsize;

	bnptr = ip->i_map;
	ip->i_map = NULL;
	kmem_free((caddr_t)bnptr, nblks*sizeof(int));
	return 0;
}
