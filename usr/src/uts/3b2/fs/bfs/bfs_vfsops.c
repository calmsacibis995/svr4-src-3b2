/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/bfs/bfs_vfsops.c	1.27"
#include "sys/types.h"
#include "sys/buf.h"
#include "sys/cmn_err.h"
#include "sys/conf.h"
#include "sys/debug.h"
#include "sys/errno.h"
#include "sys/file.h"
#include "sys/mount.h"
#include "sys/open.h"
#include "sys/param.h"
#include "sys/proc.h"
#include "sys/statvfs.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/uio.h"
#include "sys/var.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/kmem.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/sbd.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/fs/bfs.h"
#include "fs/fs_subr.h"

struct vnode *specvp(), *bdevvp();

#define BFS_BUFSIZE 1024

struct vnode *specvp(), *makespecvp();

/*
 * Boot file system VFS operations vector.
 */
STATIC int bfs_mount(), bfs_unmount(), bfs_root(), bfs_statvfs();
STATIC int bfs_sync(), bfs_vget();

struct vfsops bfsvfsops = {
	bfs_mount,
	bfs_unmount,
	bfs_root,
	bfs_statvfs,
	bfs_sync,
	bfs_vget,
	fs_nosys,	/* mountroot */
	fs_nosys,	/* swapvp */
	fs_nosys,	/* filler */
	fs_nosys,
	fs_nosys,
	fs_nosys,
};

extern int bfstype;

STATIC int
bfs_mount(vfsp, mvp, uap, cr)
	struct vfs *vfsp;
	struct vnode *mvp;
	struct mounta *uap;
	struct cred *cr;
{
	register dev_t dev;
	daddr_t lastblock, lastoff;
	struct vnode *bvp, *rvp;
	struct bsuper *bfs_super;
	long superbuf[3];
	struct bfs_dirent *dir;
	int rdonly = (uap->flags & MS_RDONLY);
	int error;
	register int i;

	if (!suser(cr))
		return EPERM;

	if (mvp->v_type != VDIR)
		return ENOTDIR;

	if (mvp->v_count != 1 || (mvp->v_flag & VROOT))
		return EBUSY;

	/*
	 * Lookup the requested mount device, get the special vnode.
	 */
	if (error = lookupname(uap->spec, UIO_USERSPACE, FOLLOW, NULLVPP, &bvp))
		return error;

	dev = bvp->v_rdev;

	/*
	 * If device is already mounted, return EBUSY.
	 */
	if (vfs_devsearch(dev) != NULL) {
		VN_RELE(bvp);
		return EBUSY;
	}

	if (bvp->v_type != VBLK) {
		VN_RELE(bvp);
		return ENOTBLK;
	}
	VN_RELE(bvp);

	/*
	 * Open the special device.  We will keep this vnode around for as
	 * long as the filesystem is mounted.  All subsequent fs I/O is done
	 * on this vnode.
	 */
	bvp = makespecvp(dev, VBLK);

	if (error = VOP_OPEN(&bvp, rdonly ? FREAD : FREAD|FWRITE, u.u_cred)) {
		VN_RELE(bvp);
		return error;
	}

	/*
	 * Save the required VFS info.
	 */
	vfsp->vfs_dev = dev;
	vfsp->vfs_fsid.val[0] = dev;
	vfsp->vfs_fsid.val[1] = bfstype;

	if (rdonly)
		vfsp->vfs_flag |= VFS_RDONLY;

	/*
	 * The first three words are the start and end blocks of the file data,
	 * and the magic number.
	 */
	error = vn_rdwr(UIO_READ, bvp, (caddr_t) superbuf, sizeof(superbuf),
	  BFS_SUPEROFF, UIO_SYSSPACE, 0, 0, cr, (int *)0);

	if (error) {
		VN_RELE(bvp);
		return error;
	}

	if (superbuf[0] != BFS_MAGIC) {
		VN_RELE(bvp);
		return EINVAL;
	}

	/*
	 * The bfs "superbuf" is constantly referenced for every BFS operation.
	 * It contains all filesystem private info including the device special
	 * vnode.  A pointer to it is contained in the private data field of
	 * the vfs, and is thus passed to every vnodeop and vfsop, even if
	 * indirectly through vnode.v_vfsp.
	 */
	bfs_super = (struct bsuper *)
	  kmem_alloc(sizeof(struct bsuper), KM_SLEEP);
	if (bfs_super == NULL) {
		VN_RELE(bvp);
		return EBUSY;
	}

	vfsp->vfs_data = (caddr_t)bfs_super;	/* Store the superbuf pointer */
	bfs_super->bsup_start = superbuf[1];
	bfs_super->bsup_end = superbuf[2];

	/*
	 * Start out assuming that we have as much space as the size of the
	 * filesystem, and no free file entries.
	 */
	bfs_super->bsup_freeblocks =
		(bfs_super->bsup_end + 1 - bfs_super->bsup_start) / BFS_BSIZE;
	bfs_super->bsup_freedrents = 0;

	/*
	 * Store the device special vnode.
	 */
	bfs_super->bsup_devnode = bvp;

	bfs_super->bsup_fslocked = BFS_NO;
	bfs_super->bsup_writelock = NULL;
	bfs_super->bsup_ioinprog = 0;
	bfs_super->bsup_incore_vlist = (struct bfs_core_vnode *)0;

	/*
	 * Create the root vnode.
	 */
	bfs_fillvnode(&rvp, (caddr_t)BFS_INO2OFF(BFSROOTINO), vfsp);

	bfs_super->bsup_root = rvp;
	bfs_super->bsup_root->v_type = VDIR;
	bfs_super->bsup_root->v_flag = VROOT;

	/*
	 * Search through the filesystem, adding to the free inode entries each
	 * deleted inode or empty slot we find.  For each ino we find, subtract
	 * the size from the free blocks.  Also, figure out the last file.
	 */
	dir = (struct bfs_dirent *)
	  kmem_alloc(sizeof(struct bfs_dirent), KM_SLEEP);
	lastoff = 0;
	lastblock = 0;

	for (i = BFS_DIRSTART; i < bfs_super->bsup_start;
	     i += sizeof(struct bfs_dirent)) {
		if (BFS_GETINODE(bvp, i, dir, cr)) {
			VN_RELE(bvp);
			kmem_free(dir, sizeof(struct bfs_dirent));
			kmem_free(bfs_super, sizeof(struct bsuper));
			return EBUSY;
		}

		if (dir->d_ino == 0)	/* This is an empty slot */
			bfs_super->bsup_freedrents++;
		else {
			if (dir->d_eblock != 0)
				bfs_super->bsup_freeblocks -= 
					(dir->d_eblock + 1) - dir->d_sblock;
			if (dir->d_eblock > lastblock) {
				lastblock = dir->d_eblock;
				lastoff = i;
			}
		}
	}
	kmem_free((caddr_t)dir, sizeof(struct bfs_dirent));

		/* invalidate blocks with the dev were mounting on */
	binval(dev);			/* FIX FOR VM!!!!! */
	fbinval(vfsp);
	vfsp->vfs_bsize = BFS_BUFSIZE;
	vfsp->vfs_bcount = 0;
	vfsp->vfs_fstype = bfstype;

	/*
	 * In BFS we must keep track of the inode for the file which
	 * is last on the disk.  This is because when new data is written, it
	 * must be after that block.
	 */
	bfs_super->bsup_lastfile = lastoff;

	return 0;
}

/*
 * Unmount a BFS filesystem.  Release the device special vnode and free up all
 * pages that are in core.
 */

/* ARGSUSED */
STATIC int
bfs_unmount(vfsp, cr)
	struct vfs *vfsp;
	struct cred *cr;
{
	register struct vnode *bvp;
	register struct bfs_core_vnode *cvp;
	register struct bsuper *bs = (struct bsuper *)vfsp->vfs_data;

	if (!suser(cr))
		return EPERM;
	if (bs->bsup_root->v_count > 2)
		return EBUSY;

	for (cvp = bs->bsup_incore_vlist; cvp != (struct bfs_core_vnode *)0;
	     cvp = cvp->core_next) {
		if (cvp->core_vnode->v_data != (caddr_t)BFS_INO2OFF(BFSROOTINO))
			return EBUSY;
	}

	bvp = BFS_DEVNODE(vfsp);
	VOP_CLOSE(bvp, (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE, 1,
	  (off_t) BFS_INO2OFF(BFSROOTINO), cr);
	kmem_free(vfsp->vfs_data,sizeof(struct bsuper));
	VN_RELE(bvp);
	return 0;
}

/*
 * Return a pointer to the root vnode.  We simply take this from the superbuf.
 */

STATIC int
bfs_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{
	register struct bsuper *bs = (struct bsuper *)vfsp->vfs_data;

	*vpp = (struct vnode *)bs->bsup_root;
	VN_HOLD(*vpp);
	return 0;
}


int bfs_debugmcc;

/* ARGSUSED */
STATIC int
bfs_sync(vfsp, flag, cr)
	struct vfs *vfsp;
	short flag;
	struct cred *cr;
{
#ifdef BFS_DEBUG
	register struct bsuper *bs = (struct bsuper *)vfsp->vfs_data;
	if ( bfs_debugmcc ) {
		cmn_err(CE_CONT, "\n*****Superblock in memory\n");
		cmn_err(CE_CONT, "bsup_start    x:%x:    d:%d:\n",
			bs->bsup_start, bs->bsup_start);
		cmn_err(CE_CONT, "bsup_end      x:%x:    d:%d:\n",
			bs->bsup_end, bs->bsup_end);
		cmn_err(CE_CONT, "freeblocks    x:%x:    d:%d:\n",
			bs->bsup_freeblocks, bs->bsup_freeblocks);
		cmn_err(CE_CONT, "freedrents    x:%x:    d:%d:\n",
			bs->bsup_freedrents, bs->bsup_freedrents);
		cmn_err(CE_CONT, "lastfile      x:%x:    d:%d:\n",
			bs->bsup_lastfile, bs->bsup_lastfile);
		cmn_err(CE_CONT, "compacted     x:%x:    d:%d:\n",
			bs->bsup_compacted, bs->bsup_compacted);
	}
#endif
	return 0;		/* All BFS I/O is synchronous anyway. */
}


/*
 * Given an fid, create or find a vnode.  In BFS, we can build any vnode given
 * the dirent offset, which is the only thing described by the fid.
 */
STATIC int
bfs_vget(vfsp, vpp, fidp)
	struct vfs *vfsp;
	struct vnode **vpp;
	struct fid *fidp;
{

	register struct bfs_fid_overlay *overlay;
	struct vnode *vp;

	overlay = (struct bfs_fid_overlay *)fidp;
	vp = (struct vnode *)kmem_alloc(sizeof(struct vnode), KM_SLEEP);

	bfs_fillvnode(&vp, (caddr_t)overlay->o_offset, vfsp);

	*vpp = vp;
	return 0;
}

/*
 * Return "filesystem independent" information about this VFS.
 */
STATIC int
bfs_statvfs(vfsp, sp)
	struct vfs *vfsp;
	register struct statvfs *sp;
{
	register struct bsuper *bs = (struct bsuper *)vfsp->vfs_data;

	bzero((caddr_t)sp, sizeof(*sp));
	sp->f_bsize = sp->f_frsize = BFS_BSIZE;
	sp->f_blocks = (bs->bsup_end + 1) / BFS_BSIZE;
	sp->f_bfree = sp->f_bavail = bs->bsup_freeblocks;
	sp->f_files = (bs->bsup_start-BFS_DIRSTART) / sizeof(struct bfs_dirent);
	sp->f_ffree = sp->f_favail = bs->bsup_freedrents;
	sp->f_fsid = vfsp->vfs_dev;
	strcpy(sp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name);
	sp->f_flag = vf_to_stf(vfsp->vfs_flag);
	sp->f_namemax = BFS_MAXFNLEN;
	strcpy(sp->f_fstr, "/stand");
	return 0;
}
