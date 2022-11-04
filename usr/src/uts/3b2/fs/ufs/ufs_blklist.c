/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/ufs/ufs_blklist.c	1.5"
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

#include "sys/fs/ufs_fs.h"
#include "sys/fs/ufs_inode.h"
#include "fs/fs_subr.h"


/*
 *  Allocate and build the block address map
 */

ufs_allocmap(ip)
register struct inode *ip;
{
	register int	*bnptr;
	register int	bsize;
	register int	nblks;
	register struct vnode *vp;

	vp = ITOV(ip);
	if (ip->i_map) 
		return(1);

	/*	Get number of blocks to be mapped.
	 */

	ASSERT(ip->i_map == 0);
	bsize = VBSIZE(vp);
	nblks = (ip->i_size + bsize - 1)/bsize;
	bnptr = ip->i_map = (int *)kmem_alloc(sizeof(int)*nblks, KM_SLEEP);
	if (bnptr == NULL)
		return(0);

	/*	Build the actual list of block numbers
	 *	for the file.
	 */

	ufs_bldblklst(bnptr, ip, nblks);

	/*	If the size is not an integral number of
	 *	pages long, then the last few block
	 *	number up to the next page boundary are
	 *	made zero so that no one will try to
	 *	read them in.  See code in fault.c/vfault.

	while (i%blkspp != 0) {
		*bnptr++ = -1;
		i++;
	}
	 */
	return(1);
}

/*	Build the list of block numbers for a file.  This is used
 *	for mapped files.
 */

ufs_bldblklst(lp, ip, nblks)
register int		*lp;
register struct inode	*ip;
register int		nblks;
{
	register int	lim;
	register int	*eptr;
	register int	i;
	register struct vnode *vp;
	int		*ufs_bldindr();
	dev_t	 dev;

	/*	Get the block numbers from the direct blocks first.
	 */

	vp = ITOV(ip);
	eptr = &lp[nblks];
	if (nblks < NDADDR)
		lim = nblks;
	else
		lim = NDADDR;
	
	for (i = 0  ;  i < lim  ;  i++)
		*lp++ = ip->i_db[i];
	
	if (lp >= eptr)
		return(1);
	
	dev = vp->v_vfsp->vfs_dev;
	i = 0;
	while (lp < eptr) {
		lp = ufs_bldindr(ip, lp, eptr, dev, ip->i_ib[i], i);
		if (lp == 0)
			return(0);
		i++;
	}
	return(1);
}

int  *
ufs_bldindr(ip, lp, eptr, dev, blknbr, indlvl)
struct inode 		*ip;
register int		*lp;
register int		*eptr;
register dev_t		dev;
int			blknbr;
int			indlvl;
{
	register struct buf *bp;
	register int	*bnptr;
	int		cnt;
	struct buf 	*bread();
	int 		bsize;
	struct vnode	*vp;
	struct fs *fs;

	vp = ITOV(ip);
	bsize = vp->v_vfsp->vfs_bsize;
	fs = getfs(vp->v_vfsp);
	bp = bread(dev, fragstoblks(fs, blknbr), bsize);
	if (u.u_error) {
		brelse(bp);
		return((int *) 0);
	}
	bnptr = bp->b_un.b_words;
	cnt = NINDIR(getfs(vp->v_vfsp));
	
	ASSERT(indlvl >= 0);
	while (cnt--  &&  lp < eptr) {
		if (indlvl == 0) {
			*lp++ = *bnptr++;
		} else {
			lp = ufs_bldindr(ip, lp, eptr, dev, *bnptr++, indlvl-1);
			if (lp == 0) {
				brelse(bp);
				return((int *) 0);
			}
		}
	}

	brelse(bp);
	return(lp);
}

/*	Free the block list attached to an inode.
 */

void
ufs_freemap(ip)
struct inode	*ip;
{
	register int	nblks;
	register	bsize;
	register struct vnode *vp;
	register int	type;

	vp = ITOV(ip);
	ASSERT(ip->i_flag & ILOCKED);
	
	type = ip->i_mode & IFMT;
	if (type != IFREG || ip->i_map == NULL)
		return;

	bsize = VBSIZE(vp);
	nblks = (ip->i_size + bsize - 1)/bsize;

	kmem_free((caddr_t)ip->i_map, nblks*sizeof(int));
	ip->i_map = NULL;
	return;
}
