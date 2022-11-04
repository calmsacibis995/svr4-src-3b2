/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/vnode.c	1.19"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/uio.h"
#include "sys/file.h"
#include "sys/pathname.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/stat.h"
#include "sys/mode.h"
#include "sys/conf.h"
#include "sys/immu.h"
#include "sys/proc.h"
#include "sys/sysmacros.h"
#include "sys/cmn_err.h"
#include "sys/debug.h"

#include "sys/disp.h"
#include "sys/systm.h"

/*
 * Convert stat(2) formats to vnode types and vice versa.  (Knows about
 * numerical order of S_IFMT and vnode types.)
 */
enum vtype iftovt_tab[] = {
	VNON, VFIFO, VCHR, VNON, VDIR, VXNAM, VBLK, VNON,
	VREG, VNON, VLNK, VNON, VNON, VNON, VNON, VNON
};

u_short vttoif_tab[] = {
	0, S_IFREG, S_IFDIR, S_IFBLK, S_IFCHR, S_IFLNK, S_IFIFO, S_IFNAM, 0
};

/*
 * Read or write a vnode.  Called from kernel code.
 */
int
vn_rdwr(rw, vp, base, len, offset, seg, ioflag, ulimit, cr, residp)
	enum uio_rw rw;
	struct vnode *vp;
	caddr_t base;
	int len;
	off_t offset;
	enum uio_seg seg;
	int ioflag;
	long ulimit;		/* meaningful only if rw is UIO_WRITE */
	cred_t *cr;
	int *residp;
{
	struct uio uio;
	struct iovec iov;
	int error;

	if (rw == UIO_WRITE && (vp->v_vfsp->vfs_flag & VFS_RDONLY))
		return EROFS;

	iov.iov_base = base;
	iov.iov_len = len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = offset;
	uio.uio_segflg = (short)seg;
	uio.uio_resid = len;
	uio.uio_limit = ulimit;
	VOP_RWLOCK(vp);
	if (rw == UIO_WRITE) {
		uio.uio_fmode = FWRITE;
		error = VOP_WRITE(vp, &uio, ioflag, cr);
		if (error == EFBIG)
			psignal(u.u_procp, SIGXFSZ);
	} else {
		uio.uio_fmode = FREAD;
		error = VOP_READ(vp, &uio, ioflag, cr);
	}
	VOP_RWUNLOCK(vp);
	if (residp)
		*residp = uio.uio_resid;
	else if (uio.uio_resid)
		error = EIO;
	return error;
}

/*
 * Release a vnode.  Decrements reference count and calls
 * VOP_INACTIVE on last reference.
 */
void
vn_rele(vp)
	register struct vnode *vp;
{
	ASSERT(vp->v_count != 0);
	if (--vp->v_count == 0)
		VOP_INACTIVE(vp, u.u_cred);
}

/*
 * Open/create a vnode.
 * This may be callable by the kernel, the only known use
 * of user context being that the current user credentials
 * are used for permissions.
 */
int
vn_open(pnamep, seg, filemode, createmode, vpp)
	char *pnamep;
	enum uio_seg seg;
	register int filemode;
	int createmode;
	struct vnode **vpp;
{
	struct vnode *vp;
	register int mode;
	register int error;

	mode = 0;
	if (filemode & FREAD)
		mode |= VREAD;
	if (filemode & (FWRITE|FTRUNC))
		mode |= VWRITE;
 
	if (filemode & FCREAT) {
		struct vattr vattr;
		enum vcexcl excl;

		/*
		 * Wish to create a file.
		 */
		vattr.va_type = VREG;
		vattr.va_mode = createmode;
		vattr.va_mask = AT_TYPE|AT_MODE;
		if (filemode & FTRUNC) {
			vattr.va_size = 0;
			vattr.va_mask |= AT_SIZE;
		}
		if (filemode & FEXCL)
			excl = EXCL;
		else
			excl = NONEXCL;
		filemode &= ~(FCREAT|FTRUNC|FEXCL);
		
		/* 
		 * vn_create can take a while, so preempt.
		 */
		PREEMPT();
		if (error =
		  vn_create(pnamep, seg, &vattr, excl, mode, &vp, CRCREAT))
			return error;
		PREEMPT();
	} else {
		/*
		 * Wish to open a file.  Just look it up.
		 */
		if (error = lookupname(pnamep, seg, FOLLOW, NULLVPP, &vp))
			return error;
		/*
		 * Can't write directories, active texts, or
		 * read-only filesystems.  Can't truncate files
		 * on which mandatory locking is in effect.
		 */
		if (filemode & (FWRITE|FTRUNC)) {
			struct vattr vattr;

			if (vp->v_type == VDIR) {
				error = EISDIR;
				goto out;
			}
			if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
				error = EROFS;
				goto out;
			}
			/*
			 * If there's shared text associated with
			 * the vnode, try to free it up once.
			 * If we fail, we can't allow writing.
			 */
			if (vp->v_flag & VTEXT) {
				xrele(vp);
				if (vp->v_flag & VTEXT) {
					error = ETXTBSY;
					goto out;
				}
			}
			/*
			 * Can't truncate files on which mandatory locking
			 * is in effect.
			 */
			if ((filemode & FTRUNC) && vp->v_filocks != NULL) {
				vattr.va_mask = AT_MODE;
				if ((error =
				    VOP_GETATTR(vp, &vattr, 0, u.u_cred)) == 0
				  && MANDLOCK(vp, vattr.va_mode))
					error = EAGAIN;
			}
			if (error)
				goto out;
		}
		/*
		 * Check permissions.
		 */
		if (error = VOP_ACCESS(vp, mode, 0, u.u_cred))
			goto out;
	}
	/*
	 * Do opening protocol.
	 */
	error = VOP_OPEN(&vp, filemode, u.u_cred);
	/*
	 * Truncate if required.
	 */
	if (error == 0 && (filemode & FTRUNC)) {
		struct vattr vattr;

		vattr.va_size = 0;
		vattr.va_mask = AT_SIZE;
		error = VOP_SETATTR(vp, &vattr, 0, u.u_cred);
	}
out:
	if (error) {
		VN_RELE(vp);
	} else
		*vpp = vp;
	return error;
}

/*
 * Create a vnode (makenode).
 */
int
vn_create(pnamep, seg, vap, excl, mode, vpp, why)
	char *pnamep;
	enum uio_seg seg;
	struct vattr *vap;
	enum vcexcl excl;
	int mode;
	struct vnode **vpp;
	enum create why;
{
	struct vnode *dvp;	/* ptr to parent dir vnode */
	struct pathname pn;
	register int error;

	ASSERT((vap->va_mask & (AT_TYPE|AT_MODE)) == AT_TYPE|AT_MODE);

	/*
	 * Lookup directory.
	 * If new object is a file, call lower level to create it.
	 * Note that it is up to the lower level to enforce exclusive
	 * creation, if the file is already there.
	 * This allows the lower level to do whatever
	 * locking or protocol that is needed to prevent races.
	 * If the new object is directory call lower level to make
	 * the new directory, with "." and "..".
	 */
	if (error = pn_get(pnamep, seg, &pn))
		return error;
	dvp = NULL;
	*vpp = NULL;
	/*
	 * lookup will find the parent directory for the vnode.
	 * When it is done the pn holds the name of the entry
	 * in the directory.
	 * If this is a non-exclusive create we also find the node itself.
	 */
	if (excl == EXCL) 
		error = lookuppn(&pn, NO_FOLLOW, &dvp, NULLVPP); 
	else 
		error = lookuppn(&pn, FOLLOW, &dvp, vpp); 
	if (error) {
		pn_free(&pn);
		return error;
	}

	/*
	 * Make sure filesystem is writeable.
	 */
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		if (*vpp)
			VN_RELE(*vpp);
		error = EROFS;
	} else if (excl == NONEXCL && *vpp != NULL) {
		/*
		 * The file is already there.
		 * If we are writing, and there's a shared text
		 * associated with the vnode, try to free it up once.
		 * If we fail, we can't allow writing.
		 */
		if ((mode & VWRITE) && ((*vpp)->v_flag & VTEXT)) {
			xrele(*vpp);
			if ((*vpp)->v_flag & VTEXT)
				error = ETXTBSY;
		}
		/*
		 * We throw the vnode away to let VOP_CREATE truncate the
		 * file in a non-racy manner.
		 */
		VN_RELE(*vpp);
	}

	if (error == 0) {
		if (why != CRMKNOD)
			vap->va_mode &= ~VSVTX;
		/*
		 * Call mkdir() if specified, otherwise create().
		 */
		if (why == CRMKDIR)
			error =
			  VOP_MKDIR(dvp, pn.pn_path, vap, vpp, u.u_cred);
		else
			error = VOP_CREATE(dvp, pn.pn_path, vap,
			  excl, mode, vpp, u.u_cred);
	}
	pn_free(&pn);
	VN_RELE(dvp);
	return error;
}

/*
 * Link.
 */
int
vn_link(from, to, seg)
	char *from;
	char *to;
	enum uio_seg seg;
{
	struct vnode *fvp;		/* from vnode ptr */
	struct vnode *tdvp;		/* to directory vnode ptr */
	struct pathname pn;
	register int error;
	struct vattr vattr;
	long fsid;

	fvp = tdvp = NULL;
	if (error = pn_get(to, seg, &pn))
		return error;
	if (error = lookupname(from, seg, NO_FOLLOW, NULLVPP, &fvp))
		goto out;

	if (error = lookuppn(&pn, NO_FOLLOW, &tdvp, NULLVPP))
		goto out;

	/*
	 * Make sure both source vnode and target directory vnode are
	 * in the same vfs and that it is writeable.
	 */
	if (error = VOP_GETATTR(fvp, &vattr, 0, u.u_cred))
		goto out;
	fsid = vattr.va_fsid;
	if (error = VOP_GETATTR(tdvp, &vattr, 0, u.u_cred))
		goto out;
	if (fsid != vattr.va_fsid) {
		error = EXDEV;
		goto out;
	}
	if (tdvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/*
	 * Do the link.
	 */
	error = VOP_LINK(tdvp, fvp, pn.pn_path, u.u_cred);
out:
	pn_free(&pn);
	if (fvp)
		VN_RELE(fvp);
	if (tdvp)
		VN_RELE(tdvp);
	return error;
}

/*
 * Rename.
 */
int
vn_rename(from, to, seg)
	char *from;
	char *to;
	int seg;
{
	struct vnode *fdvp;		/* from directory vnode ptr */
	struct vnode *fvp;		/* from vnode ptr */
	struct vnode *tdvp;		/* to directory vnode ptr */
	struct pathname fpn;		/* from pathname */
	struct pathname tpn;		/* to pathname */
	register int error;

	fdvp = tdvp = fvp = NULL;
	/*
	 * Get to and from pathnames.
	 */
	if (error = pn_get(from, seg, &fpn))
		return error;
	if (error = pn_get(to, seg, &tpn)) {
		pn_free(&fpn);
		return error;
	}
	/*
	 * Lookup to and from directories.
	 */
	if (error = lookuppn(&fpn, NO_FOLLOW, &fdvp, &fvp))
		goto out;
	/*
	 * Make sure there is an entry.
	 */
	if (fvp == NULL) {
		error = ENOENT;
		goto out;
	}
	if (error = lookuppn(&tpn, NO_FOLLOW, &tdvp, NULLVPP))
		goto out;
	/*
	 * Make sure both the from vnode and the to directory are
	 * in the same vfs and that it is writable.
	 */
	if (fvp->v_vfsp != tdvp->v_vfsp) {
		error = EXDEV;
		goto out;
	}
	if (tdvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/*
	 * Do the rename.
	 */
	error = VOP_RENAME(fdvp, fpn.pn_path, tdvp, tpn.pn_path, u.u_cred);
out:
	pn_free(&fpn);
	pn_free(&tpn);
	if (fvp)
		VN_RELE(fvp);
	if (fdvp)
		VN_RELE(fdvp);
	if (tdvp)
		VN_RELE(tdvp);
	return error;
}

/*
 * Remove a file or directory.
 */
int
vn_remove(fnamep, seg, dirflag)
	char *fnamep;
	enum uio_seg seg;
	enum rm dirflag;
{
	struct vnode *vp;		/* entry vnode */
	struct vnode *dvp;		/* ptr to parent dir vnode */
	struct pathname pn;		/* name of entry */
	enum vtype vtype;
	register int error;

	if (error = pn_get(fnamep, seg, &pn))
		return error;
	vp = NULL;
	if (error = lookuppn(&pn, NO_FOLLOW, &dvp, &vp)) {
		pn_free(&pn);
		return error;
	}

	/*
	 * Make sure there is an entry.
	 */
	if (vp == NULL) {
		error = ENOENT;
		goto out;
	}

	/*
	 * Make sure filesystem is writeable.
	 */
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}

	/*
	 * Don't unlink the root of a mounted filesystem, unless
	 * it's marked unlinkable.
	 */
	if (vp->v_flag & VROOT) {
		if (vp->v_vfsp->vfs_flag & VFS_UNLINKABLE) {
			error = dounmount(vp->v_vfsp, u.u_cred);
			vp->v_vfsp = NULL;
		} else
			error = EBUSY;
		if (error)
			goto out;
	}

	/*
	 * Release vnode before removing.
	 */
	vtype = vp->v_type;
	VN_RELE(vp);
	vp = NULL;
	if (dirflag == RMDIRECTORY) {
		/*
		 * Caller is using rmdir(2), which can only be applied to
		 * directories.
		 */
		if (vtype != VDIR)
			error = ENOTDIR;
		else
			error = VOP_RMDIR(dvp, pn.pn_path, u.u_cdir, u.u_cred);
	} else {
		/*
		 * Unlink(2) can be applied to anything.
		 */
		error = VOP_REMOVE(dvp, pn.pn_path, u.u_cred);
	}
		
out:
	pn_free(&pn);
	if (vp != NULL)
		VN_RELE(vp);
	VN_RELE(dvp);
	return error;
}
