/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/vncalls.c	1.66"
/*
 * System call routines for operations on files.  These manipulate
 * the global and per-process file table entries which refer to
 * vnodes, the system generic file abstraction.
 *
 * Many operations take a path name.  After preparing arguments, a
 * typical operation may proceed with:
 *
 *	error = lookupname(name, seg, followlink, &dvp, &vp);
 *
 * where "name" is the path name operated on, "seg" is UIO_USERSPACE
 * or UIO_SYSSPACE to indicate the address space in which the path
 * name resides, "followlink" specifies whether to follow symbolic
 * links, "dvp" is a pointer to a vnode for the directory containing
 * "name", and "vp" is a pointer to a vnode for "name".  (Both "dvp"
 * and "vp" are filled in by lookupname()).  "error" is zero for a
 * successful lookup, or a non-zero errno (from <sys/errno.h>) if an
 * error occurred.  This paradigm, in which routines return error
 * numbers to their callers and other information is returned via
 * reference parameters, now appears in many places in the kernel.
 *
 * lookupname() fetches the path name string into an internal buffer
 * using pn_get() (pathname.c) and extracts each component of the path
 * by iterative application of the file system-specific VOP_LOOKUP
 * operation until the final vnode and/or its parent are found.
 * (There is provision for multiple-component lookup as well.)  If
 * either of the addresses for dvp or vp are NULL, lookupname() assumes
 * that the caller is not interested in that vnode.  Once a vnode has
 * been found, a vnode operation (e.g. VOP_OPEN, VOP_READ) may be
 * applied to it.
 *
 * With few exceptions (made only for reasons of backward compatibility)
 * operations on vnodes are atomic, so that in general vnodes are not
 * locked at this level, and vnode locking occurs at lower levels (either
 * locally, or, perhaps, on a remote machine.  (The exceptions make use
 * of the VOP_RWLOCK and VOP_RWUNLOCK operations, and include VOP_READ,
 * VOP_WRITE, and VOP_READDIR).  In addition permission checking is
 * generally done by the specific filesystem, via its VOP_ACCESS
 * operation.  The upper (vnode) layer performs checks involving file
 * types (e.g. VREG, VDIR), since the type is static over the life of
 * the vnode.
 */

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/signal.h"
#include "sys/sbd.h"
#include "sys/immu.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/time.h"
#include "sys/fcntl.h"
#include "sys/pathname.h"
#include "sys/stat.h"
#include "sys/ttold.h"
#include "sys/var.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/file.h"
#include "sys/mode.h"
#include "sys/proc.h"
#include "sys/sysinfo.h"
#include "sys/inline.h"
#include "sys/uio.h"
#include "sys/debug.h"
#include "sys/poll.h"
#include "sys/kmem.h"
#include "sys/filio.h"
/* #ifdef MERGE */
#include "sys/locking.h"
/* #endif MERGE*/

#include "sys/disp.h"
#include "sys/mkdev.h"
#include "sys/time.h"

#include "rpc/types.h"
#define NFSSERVER
#include "nfs/nfs.h"

/*
 * Open a file.
 */
struct opena {
	char	*fname;
	int	fmode;
	int	cmode;
};

#if defined(__STDC__)
STATIC int	copen(char *, int, int, rval_t *);
#else
STATIC int	copen();
#endif

int
open(uap, rvp)
	register struct opena *uap;
	rval_t *rvp;
{
	return copen(uap->fname, (int)(uap->fmode - FOPEN), uap->cmode, rvp);
}

/*
 * Create a file.
 */
struct creata {
	char	*fname;
	int	cmode;
};

int
creat(uap, rvp)
	register struct creata *uap;
	rval_t *rvp;
{
	return copen(uap->fname, FWRITE|FCREAT|FTRUNC, uap->cmode, rvp);
}

/*
 * Common code for open() and creat().  Check permissions, allocate
 * an open file structure, and call the device open routine (if any).
 */
STATIC int
copen(fname, filemode, createmode, rvp)
	char *fname;
	int filemode;
	int createmode;
	rval_t *rvp;
{
	vnode_t *vp;
	file_t *fp;
	register int error;
	int fd;

	if ((filemode & (FREAD|FWRITE)) == 0)
		return EINVAL;

	if ((filemode & (FNONBLOCK|FNDELAY)) == (FNONBLOCK|FNDELAY))
		filemode &= ~FNDELAY;

	if (error = falloc((vnode_t *)NULL, filemode & FMASK, &fp, &fd))
		return error;
	error = vn_open(fname, UIO_USERSPACE, filemode,
	  (int)((createmode & MODEMASK) & ~u.u_cmask), &vp);
	if (error) {
		setf(fd, NULLFP);
		unfalloc(fp);
	} else {
		fp->f_vnode = vp;
		rvp->r_val1 = fd;
	}

	return error;
}

/*
 * Close a file.
 */
struct closea {
	int	fdes;
};

/* ARGSUSED */
int
close(uap, rvp)
	register struct closea *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int error;

	if (error = getf(uap->fdes, &fp))
		return error;
	setf(uap->fdes, NULLFP);
	return closef(fp);
}

/*
 * Read and write.
 */
struct rwa {
	int fdes;
	char *cbuf;
	unsigned count;
};

/*
 * Readv and writev.
 */
struct rwva {
	int fdes;
	struct iovec *iovp;
	int iovcnt;
};

#if defined(__STDC__)

STATIC int	rw(struct rwa *, rval_t *, int);
STATIC int	rwv(struct rwva *, rval_t *, int);
STATIC int	rdwr(file_t *, uio_t *, rval_t *, int);
#else

STATIC int	rw();
STATIC int	rwv();
STATIC int	rdwr();

#endif

int
read(uap, rvp)
	struct rwa *uap;
	rval_t *rvp;
{
	return rw(uap, rvp, FREAD);
}

int
write(uap, rvp)
	struct rwa *uap;
	rval_t *rvp;
{
	return rw(uap, rvp, FWRITE);
}

int
readv(uap, rvp)
	struct rwva *uap;
	rval_t *rvp;
{
	return rwv(uap, rvp, FREAD);
}

int
writev(uap, rvp)
	struct rwva *uap;
	rval_t *rvp;
{
	return rwv(uap, rvp, FWRITE);
}

STATIC int
rw(uap, rvp, mode)
	struct rwa *uap;
	rval_t *rvp;
	register int mode;
{
	struct uio auio;
	struct iovec aiov;
	file_t *fp;
	int error;

	if (error = getf(uap->fdes, &fp))
		return error;
	if ((fp->f_flag & mode) == 0)
		return EBADF;
	if (mode == FREAD)
		sysinfo.sysread++;
	else
		sysinfo.syswrite++;
	aiov.iov_base = (caddr_t)uap->cbuf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	return rdwr(fp, &auio, rvp, mode);
}

STATIC int
rwv(uap, rvp, mode)
	struct rwva *uap;
	rval_t *rvp;
	register int mode;
{
	file_t *fp;
	int error;
	struct uio auio;
	struct iovec aiov[16];		/* TO DO - don't hard-code size */

	if (error = getf(uap->fdes, &fp))
		return error;
	if ((fp->f_flag & mode) == 0)
		return EBADF;
	if (mode == FREAD)
		sysinfo.sysread++;
	else
		sysinfo.syswrite++;
	if (uap->iovcnt <= 0 || uap->iovcnt > sizeof(aiov)/sizeof(aiov[0]))
		return EINVAL;
	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	if (copyin((caddr_t)uap->iovp, (caddr_t)aiov,
	  (unsigned)(uap->iovcnt * sizeof(struct iovec))))
		return EFAULT;
	return rdwr(fp, &auio, rvp, mode);
}

/*
 * Common code for read and write calls: check permissions, set base,
 * count, and offset, and switch out to VOP_READ or VOP_WRITE code.
 */
STATIC int
rdwr(fp, uio, rvp, mode)
	file_t *fp;
	register struct uio *uio;
	rval_t *rvp;
	register int mode;
{
	register vnode_t	*vp;
	enum vtype		type;
	iovec_t			*iovp;
	int			ioflag;
	int			count;
	int			i;
	register int		error;

	uio->uio_resid = 0;
	iovp = uio->uio_iov;
	for (i = 0; i < uio->uio_iovcnt; i++) {
		if (iovp->iov_len < 0)
			return EINVAL;
		uio->uio_resid += iovp->iov_len;
		if (uio->uio_resid < 0)
			return EINVAL;
		iovp++;
	}
	vp = fp->f_vnode;
	type = vp->v_type;
	if (type == VREG || type == VDIR) {
		/*
		 * Make sure that the user can write all the way up
		 * to the rlimit value.
		 */
      	        if (type == VREG && mode == FWRITE) {
      	      	        register ulong rlimit =
      	      	          u.u_rlimit[RLIMIT_FSIZE].rlim_cur - fp->f_offset;
			/* LINTED */
      	      	        if (rlimit < uio->uio_resid && rlimit > 0)
      	      	      	        uio->uio_resid = rlimit;
      	        }
	}
	count = uio->uio_resid;
	uio->uio_offset = fp->f_offset;
	uio->uio_segflg = UIO_USERSPACE;
	uio->uio_fmode = fp->f_flag;
	uio->uio_limit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur;
	ioflag = 0;
	if (fp->f_flag & FAPPEND)
		ioflag |= IO_APPEND;
	if (fp->f_flag & FSYNC)
		ioflag |= IO_SYNC;
	VOP_RWLOCK(vp);
	if (setjmp(&u.u_qsav))
		error = intrerr(uio->uio_resid == count);
	else {
		if (mode == FREAD) {
			error = VOP_READ(vp, uio, ioflag, fp->f_cred);
		} else {
			error = VOP_WRITE(vp, uio, ioflag, fp->f_cred);
		}
	}
	VOP_RWUNLOCK(vp);

	rvp->r_val1 = count - uio->uio_resid;
	u.u_ioch += (unsigned)rvp->r_val1;
	if (type == VFIFO)	/* Backward compatibility */
		fp->f_offset = rvp->r_val1;
	else
		fp->f_offset = uio->uio_offset;
	if (mode == FREAD) {
		sysinfo.readch += (unsigned)rvp->r_val1;
		if (vp->v_vfsp != NULL)
			vp->v_vfsp->vfs_bcount += 
			  rvp->r_val1 / vp->v_vfsp->vfs_bsize;

	} else {
		sysinfo.writech += (unsigned)rvp->r_val1;
		if (vp->v_vfsp != NULL)
			vp->v_vfsp->vfs_bcount += 
			  rvp->r_val1 / vp->v_vfsp->vfs_bsize;
	}
	return error;
}

/*
 * Change current working directory (".").
 */
struct chdira {
	char *fname;
};

#if defined(__STDC__)
STATIC int	chdirec(vnode_t *, vnode_t **);
#else
STATIC int	chdirec();
#endif

/* ARGSUSED */
int
chdir(uap, rvp)
	struct chdira *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	register int  error;

	if (error = lookupname(uap->fname, UIO_USERSPACE, 
	    FOLLOW, NULLVPP, &vp))
		return error;

	return chdirec(vp, &u.u_cdir);
}

/*
 * File-descriptor based version of 'chdir'.
 */
struct fchdira {
	int  fd; 
};

/* ARGSUSED */
int
fchdir(uap, rvp)
	struct fchdira *uap;
	rval_t *rvp;
{
	file_t *fp;
	vnode_t *vp;
	register int error;
	
	if (uap->fd) {
		if (error = getf(uap->fd, &fp))
			return error;
		vp = fp->f_vnode;
		VN_HOLD(vp);
	} else
		return ENOENT;

	return chdirec(vp, &u.u_cdir);
}

/*
 * Change notion of root ("/") directory.
 */
/* ARGSUSED */
int
chroot(uap, rvp)
	struct chdira *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	register int 	error;

	if (!suser(u.u_cred))
		return EPERM;
	if (error = lookupname(uap->fname, UIO_USERSPACE, 
	    FOLLOW, NULLVPP, &vp))
		return error;

	return chdirec(vp, &u.u_rdir);
}

/*
 * Chdirec() takes as an argument a vnode pointer and a vpp as an
 * out parameter.  If the vnode passed in corresponds to a 
 * directory for which the user has execute permission, then
 * vpp, if it is non-NULL, is updated to point to the vnode
 * passed in.  
 */
STATIC int
chdirec(vp, vpp)
	vnode_t *vp;
	vnode_t **vpp;
{
	register int error;

	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto bad;
	}
	PREEMPT();
	if (error = VOP_ACCESS(vp, VEXEC, 0, u.u_cred))
		goto bad;
	if (*vpp)
		VN_RELE(*vpp);
	*vpp = vp;
	return 0;

bad:
	VN_RELE(vp);
	return error;
}

/*
 * Create a special file, a regular file, or a FIFO.
 */

/* SVR3 mknod arg */
struct mknoda {
	char	*fname;		/* pathname passed by user */
	mode_t	fmode;		/* mode of pathname */
	dev_t	dev;		/* device number - b/c specials only */
};

#if defined(__STDC__)
STATIC int	cmknod(int, char *, mode_t, dev_t, rval_t *);
#else
STATIC int	cmknod();
#endif

/* SVR3 mknod */
int
mknod(uap, rvp)
	register struct mknoda *uap;
	rval_t *rvp;
{
	return cmknod(R3_MKNOD_VER, uap->fname, uap->fmode, uap->dev, rvp);
}

struct xmknoda {
	int	version;	/* version of this syscall */
	char	*fname;		/* pathname passed by user */
	mode_t	fmode;		/* mode of pathname */
	dev_t	dev;		/* device number - b/c specials only */
};

/*
 * Expanded mknod.
 */
xmknod(uap, rvp)
	register struct xmknoda *uap;
	rval_t *rvp;
{
	return cmknod(uap->version, uap->fname, uap->fmode, uap->dev, rvp);
}

/* ARGSUSED */
STATIC int
cmknod(version, fname, fmode, dev, rvp)
	int version;
	char *fname;
	mode_t fmode;
	dev_t dev;
	rval_t *rvp;
{
	vnode_t *vp;
	struct vattr vattr;
	int error;

	/*
	 * Zero type is equivalent to a regular file.
	 */
	if ((fmode & S_IFMT) == 0)
		fmode |= S_IFREG;

	/*
	 * Must be the super-user unless making a FIFO node.
	 */
	if (((fmode & S_IFMT) != S_IFIFO) 
	/* #ifdef MERGE */
	  && ((fmode & S_IFMT) != S_IFNAM)
	/* #endif MERGE */
	  && !suser(u.u_cred))
		return EPERM;
	/*
	 * Set up desired attributes and vn_create the file.
	 */
	vattr.va_type = IFTOVT(fmode);
	vattr.va_mode = (fmode & MODEMASK) & ~u.u_cmask;
	vattr.va_mask = AT_TYPE|AT_MODE;
	if (vattr.va_type == VCHR || vattr.va_type == VBLK
	  || vattr.va_type == VXNAM) {
		if (version == MKNOD_VER && vattr.va_type != VXNAM) {
			if (dev != (u_int)NODEV)
				vattr.va_rdev = dev;
			else
				return EINVAL;
		} else {
			/* dev is in old format */
			if ((emajor(dev)) > O_MAXMAJ)
				return EINVAL;
			else
				vattr.va_rdev = expdev(dev);
		}
		vattr.va_mask |= AT_RDEV;
	}
	if ((error = vn_create(fname, UIO_USERSPACE,
	  &vattr, EXCL, 0, &vp, CRMKNOD)) == 0)
		VN_RELE(vp);
	return error;
}

/*
 * Make a directory.
 */
struct mkdira {
	char *dname;
	int dmode;
};

/* ARGSUSED */
int
mkdir(uap, rvp)
	register struct mkdira *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	struct vattr vattr;
	int error;

	vattr.va_type = VDIR;
	vattr.va_mode = (uap->dmode & PERMMASK) & ~u.u_cmask;
	vattr.va_mask = AT_TYPE|AT_MODE;
	if ((error = vn_create(uap->dname, UIO_USERSPACE, &vattr,
	  EXCL, 0, &vp, CRMKDIR)) == 0)
		VN_RELE(vp);
	return error;
}

/*
 * Make a hard link.
 */
struct linka {
	char	*from;
	char	*to;
};

/* ARGSUSED */
int
link(uap, rvp)
	register struct linka *uap;
	rval_t *rvp;
{
	return vn_link(uap->from, uap->to, UIO_USERSPACE);
}

/*
 * Rename or move an existing file.
 */
struct renamea {
	char	*from;
	char	*to;
};

/* ARGSUSED */
int
rename(uap, rvp)
	struct renamea *uap;
	rval_t *rvp;
{
	return vn_rename(uap->from, uap->to, UIO_USERSPACE);
}

/*
 * Create a symbolic link.  Similar to link or rename except target
 * name is passed as string argument, not converted to vnode reference.
 */
struct symlinka {
	char	*target;
	char	*linkname;
};

/* ARGSUSED */
int
symlink(uap, rvp)
	register struct symlinka *uap;
	rval_t *rvp;
{
	vnode_t *dvp;
	struct vattr vattr;
	struct pathname tpn;
	struct pathname lpn;
	int error;

	if (error = pn_get(uap->linkname, UIO_USERSPACE, &lpn))
		return error;
	if (error = lookuppn(&lpn, NO_FOLLOW, &dvp, NULLVPP)) {
		pn_free(&lpn);
		return error;
	}
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	if ((error = pn_get(uap->target, UIO_USERSPACE, &tpn)) == 0) {
		vattr.va_type = VLNK;
		vattr.va_mode = 0777;
		vattr.va_mask = AT_TYPE|AT_MODE;
		error = VOP_SYMLINK(dvp, lpn.pn_path, &vattr,
		  tpn.pn_path, u.u_cred);
		pn_free(&tpn);
	}
out:
	pn_free(&lpn);
	VN_RELE(dvp);
	return error;
}

/*
 * Unlink (i.e. delete) a file.
 */
struct unlinka {
	char	*fname;
};

/* ARGSUSED */
int
unlink(uap, rvp)
	struct unlinka *uap;
	rval_t *rvp;
{
	return vn_remove(uap->fname, UIO_USERSPACE, RMFILE);
}

/*
 * Remove a directory.
 */
struct rmdira {
	char *dname;
};

/* ARGSUSED */
int
rmdir(uap, rvp)
	struct rmdira *uap;
	rval_t *rvp;
{
	return vn_remove(uap->dname, UIO_USERSPACE, RMDIRECTORY);
}

/*
 * Get directory entries in a file system-independent format.
 */
struct getdentsa {
	int fd;
	char *buf;
	int count;
};

int
getdents(uap, rvp)
	struct getdentsa *uap;
	rval_t *rvp;
{
	register vnode_t *vp;
	file_t *fp;
	struct uio auio;
	struct iovec aiov;
	register int error;
	int sink;

	if (error = getf(uap->fd, &fp))
		return error;
	vp = fp->f_vnode;
	if (vp->v_type != VDIR)
		return ENOTDIR;
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = fp->f_offset;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_resid = uap->count;
	VOP_RWLOCK(vp);
	error = VOP_READDIR(vp, &auio, fp->f_cred, &sink);
	VOP_RWUNLOCK(vp);
	if (error)
		return error;
	rvp->r_val1 = uap->count - auio.uio_resid;
	fp->f_offset = auio.uio_offset;
	return 0;
}

/*
 * Seek on file.
 */
struct lseeka {
	int	fdes;
	off_t	off;
	int	sbase;
};

int
lseek(uap, rvp)
	register struct lseeka *uap;
	rval_t *rvp;
{
	file_t *fp;
	register vnode_t *vp;
	struct vattr vattr;
	register int error;

	if (error = getf(uap->fdes, &fp))
		return error;
	vp = fp->f_vnode;
	if (uap->sbase == 1)
		uap->off += fp->f_offset;
	else if (uap->sbase == 2) {
		vattr.va_mask = AT_SIZE;
		if (error = VOP_GETATTR(vp, &vattr, 0, u.u_cred))
			return error;
		uap->off += vattr.va_size;
	} else if (uap->sbase != 0) {
		psignal(u.u_procp, SIGSYS);
		return EINVAL;
	}
	if ((error = VOP_SEEK(vp, fp->f_offset, &uap->off)) == 0)
		rvp->r_off = fp->f_offset = uap->off;
	return error;
}

/*
 * Determine accessibility of file.
 */
struct accessa {
	char	*fname;
	int	fmode;
};

/* ARGSUSED */
int
access(uap, rvp)
	register struct accessa *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	register cred_t *tmpcr;
	register int error, mode;

	tmpcr = crdup(u.u_cred);
	tmpcr->cr_uid = u.u_cred->cr_ruid;
	tmpcr->cr_gid = u.u_cred->cr_rgid;
	tmpcr->cr_ruid = u.u_cred->cr_uid;
	tmpcr->cr_rgid = u.u_cred->cr_gid;

	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  FOLLOW, NULLVPP, &vp)) {
		crfree(tmpcr);
		return error;
	}
	if (uap->fmode) {
		mode = (uap->fmode << 6) & (VREAD|VWRITE|VEXEC);
		error = VOP_ACCESS(vp, mode, 0, tmpcr);
	}
	crfree(tmpcr);
	VN_RELE(vp);
	return error;
}

/*
 * Get file attribute information through a file name or a file descriptor.
 */
struct stata {
	char	*fname;
	struct stat *sb;
};

#if defined(__STDC__)
STATIC int	cstat(vnode_t *, struct stat *);
#else
STATIC int	cstat();
#endif

/* ARGSUSED */
int
stat(uap, rvp)
	register struct stata *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	register int error;

	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  FOLLOW, NULLVPP, &vp))
		return error;
	error = cstat(vp, uap->sb);
	VN_RELE(vp);
	return error;
}

struct xstatarg {
	int version;
	char *fname;
	struct xstat *sb;
};

#if defined(__STDC__)
STATIC int	xcstat(vnode_t *, struct xstat *);
#else
STATIC int	xcstat();
#endif

/* ARGSUSED */
int
xstat(uap, rvp)
	register struct xstatarg *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	register int error;

	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  FOLLOW, NULLVPP, &vp))
		return error;

	/*
	 * Check version.
	 */
	switch (uap->version) {

	case STAT_VER:
		/* SVR4 stat */
		error = xcstat(vp, uap->sb);
		break;

	case R3_STAT_VER:
		/* SVR3 stat */
		error = cstat(vp, (struct stat *) uap->sb);
		break;

	default:
		error = EINVAL;
	}

	VN_RELE(vp);
	return error;
}

struct lstata {
	char	*fname;
	struct stat *sb;
};

/* ARGSUSED */
int
lstat(uap, rvp)
	register struct lstata *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	register int error;

	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  NO_FOLLOW, NULLVPP, &vp))
		return error;
	error = cstat(vp, uap->sb);
	VN_RELE(vp);
	return error;
}

/* ARGSUSED */
int
lxstat(uap, rvp)
	register struct xstatarg *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	register int error;

	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  NO_FOLLOW, NULLVPP, &vp))
		return error;

	/*
	 * Check version.
	 */
	switch (uap->version) {

	case STAT_VER:
		/* SVR4 stat */
		error = xcstat(vp, uap->sb);
		break;

	case R3_STAT_VER:
		/* SVR3 stat */
		error = cstat(vp, (struct stat *) uap->sb);
		break;

	default:
		error = EINVAL;
	}

	VN_RELE(vp);
	return error;
}

struct fstata {
	int	fdes;
	struct stat *sb;
};

/* ARGSUSED */
int
fstat(uap, rvp)
	register struct fstata *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int error;

	if (error = getf(uap->fdes, &fp))
		return error;
	return cstat(fp->f_vnode, uap->sb);
}

struct fxstatarg {
	int	version;
	int	fdes;
	struct xstat *sb;
};

/* ARGSUSED */
int
fxstat(uap, rvp)
	register struct fxstatarg *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int error;

	/*
	 * Check version number.
	 */
	switch (uap->version) {
	case STAT_VER:
		break;
	default:
		return EINVAL;
	}

	if (error = getf(uap->fdes, &fp))
		return error;

	switch (uap->version) {
	case STAT_VER:
		/* SVR4 stat */
		error = xcstat(fp->f_vnode, uap->sb);
		break;

	case R3_STAT_VER:
		/* SVR3 stat */
		error = cstat(fp->f_vnode, (struct stat *) uap->sb);
		break;

	default:
		error = EINVAL;
	}

	return error;
}

/*
 * Common code for stat(), lstat(), and fstat().
 */
STATIC int
cstat(vp, ubp)
	register vnode_t *vp;
	struct stat *ubp;
{
	struct stat sb;
	struct vattr vattr;
	register int error;

	vattr.va_mask = AT_STAT;
	if (error = VOP_GETATTR(vp, &vattr, 0, u.u_cred))
		return error;
	sb.st_mode = (o_mode_t) (VTTOIF(vattr.va_type) | vattr.va_mode);
	/*
	 * Check for large values.
	 */
	if (vattr.va_uid > USHRT_MAX || vattr.va_gid > USHRT_MAX
	  || vattr.va_nodeid > USHRT_MAX || vattr.va_nlink > SHRT_MAX )
		return EOVERFLOW;
	sb.st_uid = (o_uid_t) vattr.va_uid;
	sb.st_gid = (o_gid_t) vattr.va_gid;
	/*
	 * Need to convert expanded dev to old dev format.
	 */
	if (vattr.va_fsid & 0x8000)
		sb.st_dev = (o_dev_t) vattr.va_fsid;
	else
		sb.st_dev = (o_dev_t) cmpdev(vattr.va_fsid);
	sb.st_ino = (o_ino_t) vattr.va_nodeid;
	sb.st_nlink = (o_nlink_t) vattr.va_nlink;
	sb.st_size = vattr.va_size;
	sb.st_atime = vattr.va_atime.tv_sec;
	sb.st_mtime = vattr.va_mtime.tv_sec;
	sb.st_ctime = vattr.va_ctime.tv_sec;
	sb.st_rdev = (o_dev_t)cmpdev(vattr.va_rdev);

	PREEMPT();

	if (copyout((caddr_t)&sb, (caddr_t)ubp, sizeof(sb)))
		error = EFAULT;
	return error;
}

/*
 * Common code for xstat(), lxstat(), and fxstat().
 */
STATIC int
xcstat(vp, ubp)
	register vnode_t *vp;
	struct xstat *ubp;
{
	struct xstat sb;
	struct vattr vattr;
	register int error;
	register struct vfssw *vswp;

	vattr.va_mask = AT_STAT|AT_NBLOCKS|AT_BLKSIZE;
	if (error = VOP_GETATTR(vp, &vattr, 0, u.u_cred))
		return error;

	struct_zero((caddr_t)&sb, sizeof(sb));

	sb.st_mode = VTTOIF(vattr.va_type) | vattr.va_mode;
	sb.st_uid = vattr.va_uid;
	sb.st_gid = vattr.va_gid;
	sb.st_dev = vattr.va_fsid;
	sb.st_ino = vattr.va_nodeid;
	sb.st_nlink = vattr.va_nlink;
	sb.st_size = vattr.va_size;
	sb.st_atime = vattr.va_atime;
	sb.st_mtime = vattr.va_mtime;
	sb.st_ctime = vattr.va_ctime;
	sb.st_rdev = vattr.va_rdev;
	sb.st_blksize = vattr.va_blksize;
	sb.st_blocks = vattr.va_nblocks;
	if (vp->v_vfsp) {
		vswp = &vfssw[vp->v_vfsp->vfs_fstype];
		if (vswp->vsw_name && *vswp->vsw_name)
			strcpy(sb.st_fstype, vswp->vsw_name);

	}
	if (copyout((caddr_t)&sb, (caddr_t)ubp, sizeof(sb)))
		error = EFAULT;
	return error;
}

/*
 * Read the contents of a symbolic link.
 */
struct readlinka {
	char	*name;
	char	*buf;
	int	count;
};

int
readlink(uap, rvp)
	register struct readlinka *uap;
	rval_t *rvp;
{
	vnode_t *vp;
	struct iovec aiov;
	struct uio auio;
	int error;

	if (error = lookupname(uap->name, UIO_USERSPACE,
	  NO_FOLLOW, NULLVPP, &vp))
		return error;

	if (vp->v_type != VLNK) {
		error = EINVAL;
		goto out;
	}
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_resid = uap->count;
	error = VOP_READLINK(vp, &auio, u.u_cred);
out:
	VN_RELE(vp);
	rvp->r_val1 = uap->count - auio.uio_resid;
	return error;
}

#if defined(__STDC__)
int	namesetattr(char *, enum symfollow, vattr_t *, int);
int	fdsetattr(int, vattr_t *);
#else
int	namesetattr();
int	fdsetattr();
#endif

/*
 * Change mode of file given path name.
 */
struct chmoda {
	char	*fname;
	int	fmode;
};

/* ARGSUSED */
int
chmod(uap, rvp)
	register struct chmoda *uap;
	rval_t *rvp;
{
	struct vattr vattr;

	vattr.va_mode = uap->fmode & MODEMASK;
	vattr.va_mask = AT_MODE;
	return namesetattr(uap->fname, FOLLOW, &vattr, 0);
}

/*
 * Change mode of file given file descriptor.
 */
struct fchmoda {
	int	fd;
	int	fmode;
};

/* ARGSUSED */
int
fchmod(uap, rvp)
	register struct fchmoda *uap;
	rval_t *rvp;
{
	struct vattr vattr;

	vattr.va_mode = uap->fmode & MODEMASK;
	vattr.va_mask = AT_MODE;
	return fdsetattr(uap->fd, &vattr);
}

/*
 * Change ownership of file given file name.
 */
struct chowna {
	char	*fname;
	int	uid;
	int	gid;
};

/* ARGSUSED */
int
chown(uap, rvp)
	register struct chowna *uap;
	rval_t *rvp;
{
	struct vattr vattr;

	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	vattr.va_mask = 0;
	if (vattr.va_uid != -1)
		vattr.va_mask |= AT_UID;
	if (vattr.va_gid != -1)
		vattr.va_mask |= AT_GID;
	return namesetattr(uap->fname, FOLLOW, &vattr, 0);
}

/* ARGSUSED */
int
lchown(uap, rvp)
	register struct chowna *uap;
	rval_t *rvp;
{
	struct vattr vattr;

	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	vattr.va_mask = 0;
	if (vattr.va_uid != -1)
		vattr.va_mask |= AT_UID;
	if (vattr.va_gid != -1)
		vattr.va_mask |= AT_GID;
	return namesetattr(uap->fname, NO_FOLLOW, &vattr, 0);
}

/*
 * Change ownership of file given file descriptor.
 */
struct fchowna {
	int	fd;
	int	uid;
	int	gid;
};

/* ARGSUSED */
int
fchown(uap, rvp)
	register struct fchowna *uap;
	rval_t *rvp;
{
	struct vattr vattr;

	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	vattr.va_mask = 0;
	if (vattr.va_uid != -1)
		vattr.va_mask |= AT_UID;
	if (vattr.va_gid != -1)
		vattr.va_mask |= AT_GID;
	return fdsetattr(uap->fd, &vattr);
}

/* #ifdef MERGE */
/* 
 * Change file size.
 */
struct chsizea {
	int fdes;
	int size;
};

/* ARGSUSED */
chsize(uap, rvp)
	register struct chsizea *uap;
	rval_t *rvp;
{
	register vnode_t *vp;
	int error;
	file_t *fp;
	struct flock bf;

	if (uap->size < 0L || uap->size > u.u_rlimit[RLIMIT_FSIZE].rlim_cur)
		return EINVAL;
	if (error = getf(uap->fdes, &fp))
		return error;
	if ((fp->f_flag & FWRITE) == 0)
		return EBADF;
	vp = fp->f_vnode;
	if (vp->v_type != VREG)
		return EINVAL;         /* could have better error */
	if (vp->v_flag & VTEXT) {
		xrele(vp);		/* try once to free text */
		if (vp->v_flag & VTEXT)
			return ETXTBSY;
	}
	bf.l_whence = 0;
	bf.l_start = uap->size;
	bf.l_type = F_WRLCK;
	bf.l_len = 0;
	return VOP_SPACE(vp, F_FREESP, &bf, fp->f_flag, fp->f_offset,
	  fp->f_cred);
}

/* 
 * Read check.
 */
struct rdchka {
	int fdes;
};

/* ARGSUSED */
rdchk(uap, rvp)
	register struct rdchka *uap;
	rval_t *rvp;
{
	register vnode_t *vp;
	file_t *fp;
	vattr_t vattr;
	register int error;

	if (error = getf(uap->fdes, &fp))
		return error;
	if ((fp->f_flag & FREAD) == 0)
		return EBADF;
	vp = fp->f_vnode;
	if (vp->v_type == VCHR || vp->v_type == VFIFO) {
		vattr.va_mask = AT_SIZE;
		if (error = VOP_GETATTR(vp, &vattr, 0, u.u_cred))
			return error;
		/* LINTED */
		rvp->r_val1 = (vattr.va_size > 0);
	} else
		rvp->r_val1 = 1;

	return 0;
}

/*
 * XENIX locking() system call.  Locking() is a system call subtype called
 * through the cxenix sysent entry.
 *
 * The following is a summary of how locking() calls map onto fcntl():
 *
 *	locking() 	new fcntl()	acts like fcntl()	with flock 
 *	 'mode'		  'cmd'		     'cmd'		 'l_type'
 *	---------	-----------     -----------------	-------------
 *
 *	LK_UNLCK	F_LK_UNLCK	F_SETLK			F_UNLCK
 *	LK_LOCK		F_LK_LOCK	F_SETLKW		F_WRLCK
 *	LK_NBLCK	F_LK_NBLCK	F_SETLK			F_WRLCK
 *	LK_RLCK		F_LK_RLCK	F_SETLKW		F_RDLCK
 *	LK_NBRLCK	F_LK_NBRLCK	F_SETLW			F_RDLCK
 *
 */
struct lockinga {
	int  fdes;
	int  mode;
	long size;
};

/* ARGSUSED */
int
locking(uap, rvp)
	struct lockinga *uap;
	rval_t *rvp;
{
	file_t *fp;
	struct flock bf;
	register int error, cmd;

	if (error = getf(uap->fdes, &fp))
		return error;

	/*
	 * Map the locking() mode onto the fcntl() cmd.
	 */
	switch (uap->mode) {
	case LK_UNLCK:
		cmd = F_SETLK;
		bf.l_type = F_UNLCK;
		break;
	case LK_LOCK:
		cmd = F_SETLKW;
		bf.l_type = F_WRLCK;
		break;
	case LK_NBLCK:
		cmd = F_SETLK;
		bf.l_type = F_WRLCK;
		break;
	case LK_RLCK:
		cmd = F_SETLKW;
		bf.l_type = F_RDLCK;
		break;
	case LK_NBRLCK:
		cmd = F_SETLK;
		bf.l_type = F_RDLCK;
		break;
	default:
		return EINVAL;
	}

	bf.l_whence = 1;
	if (uap->size < 0) {
		bf.l_start = uap->size;
		bf.l_len = -(uap->size);
	} else {
		bf.l_start = 0L;
		bf.l_len = uap->size;
	}

	if ((error = VOP_FRLOCK(fp->f_vnode, cmd, &bf, fp->f_flag,
	  fp->f_offset, fp->f_cred)) != 0) {
		if (error == EAGAIN)
			error = EACCES;
	} else if (uap->mode != LK_UNLCK) {
		/*
		 * Turn on lock-enforcement bit.
		 */
		fp->f_vnode->v_flag |= VXLOCKED;
	}

	return error;
}
/* #endif MERGE */

/*
 * Set access/modify times on named file.
 */
struct utimea {
	char	*fname;
	time_t	*tptr;
};

/* ARGSUSED */
int
utime(uap, rvp)
	register struct utimea *uap;
	rval_t *rvp;
{
	time_t tv[2];
	struct vattr vattr;
	int flags = 0;

	if (uap->tptr != NULL) {
		if (copyin((caddr_t)uap->tptr,(caddr_t)tv, sizeof(tv)))
			return EFAULT;
		flags |= ATTR_UTIME;
	} else {
		tv[0] = hrestime.tv_sec;
		tv[1] = hrestime.tv_sec;
	}
	vattr.va_atime.tv_sec = tv[0];
	vattr.va_atime.tv_nsec = 0;
	vattr.va_mtime.tv_sec = tv[1];
	vattr.va_mtime.tv_nsec = 0;
	vattr.va_mask = AT_ATIME|AT_MTIME;
	return namesetattr(uap->fname, FOLLOW, &vattr, flags);
}

/*
 * Common routine for modifying attributes of named files.
 */
int
namesetattr(fnamep, followlink, vap, flags)
	char *fnamep;
	enum symfollow followlink;
	struct vattr *vap;
	int flags;
{
	vnode_t *vp;
	register int error;

	if (error = lookupname(fnamep, UIO_USERSPACE, followlink,
	  NULLVPP, &vp))
		return error;	
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
		error = EROFS;
	else
		error = VOP_SETATTR(vp, vap, flags, u.u_cred);
	VN_RELE(vp);
	return error;
}

/*
 * Common routine for modifying attributes of files referenced
 * by descriptor.
 */
int
fdsetattr(fd, vap)
	int fd;
	struct vattr *vap;
{
	file_t *fp;
	register vnode_t *vp;
	register int error;

	if ((error = getf(fd, &fp)) == 0) {
		vp = fp->f_vnode;
		if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
			return EROFS;
		error = VOP_SETATTR(vp, vap, 0, u.u_cred);
	}
	return error;
}

/*
 * Flush output pending for file.
 */
struct fsynca {
	int fd;
};

/* ARGSUSED */
int
fsync(uap, rvp)
	struct fsynca *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int error;

	if ((error = getf(uap->fd, &fp)) == 0)
		error = VOP_FSYNC(fp->f_vnode, fp->f_cred);
	return error;
}

/*
 * File control.
 */

struct fcntla {
	int fdes;
	int cmd;
	int arg;
};

struct f_cnvt_arg {
	fhandle_t *fh;
	int filemode;
};

int
fcntl(uap, rvp)
	register struct fcntla *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int i, error;
	vnode_t *vp;
	off_t offset;
	int flag, fd;
	struct flock bf;
	struct o_flock obf;
	/* the next 7 are for NFS file/record locks */
	struct f_cnvt_arg a;
	register struct f_cnvt_arg *ap;
	fhandle_t tfh;
	register int filemode;
	int mode;
	struct vnode *myfhtovp();
	extern struct fileops vnodefops;
	
	if (error = getf(uap->fdes, &fp))
		return error;
	vp = fp->f_vnode;
	flag = fp->f_flag;
	offset = fp->f_offset;

	switch (uap->cmd) {

	case F_DUPFD:
		if ((i = uap->arg) < 0 
		  || i >= u.u_rlimit[RLIMIT_NOFILE].rlim_cur)
			error = EINVAL;
		else if ((error = ufalloc(i, &fd)) == 0) {
			setf(fd, fp);
			fp->f_count++;
			rvp->r_val1 = fd;
			break;
		}
		break;

	case F_GETFD:
		rvp->r_val1 = getpof(uap->fdes);
		break;

	case F_SETFD:
		(void) setpof(uap->fdes, (char)uap->arg);
		break;

	case F_GETFL:
		rvp->r_val1 = fp->f_flag+FOPEN;
		break;

	case F_SETFL:
		if ((uap->arg & (FNONBLOCK|FNDELAY)) == (FNONBLOCK|FNDELAY))
			uap->arg &= ~FNDELAY;
		if ((error = VOP_SETFL(vp, flag, uap->arg, fp->f_cred)) == 0) {
			uap->arg &= FMASK;
			fp->f_flag &= (FREAD|FWRITE);
			fp->f_flag |= (uap->arg-FOPEN) & ~(FREAD|FWRITE);
		}
		break;

	case F_GETLK:
	case F_O_GETLK:
	case F_SETLK:
	case F_SETLKW:
		/*
		 * Copy in input fields only.
		 */
		if (copyin((caddr_t)uap->arg, (caddr_t)&bf, sizeof obf)) {
			error = EFAULT;
			break;
		}
		if (bf.l_type != F_UNLCK) {
			u.u_procp->p_flag |= SLKDONE;
			setpof (uap->fdes, getpof(uap->fdes) | UF_FDLOCK);
		}
		if (error =
		  VOP_FRLOCK(vp, uap->cmd, &bf, flag, offset, fp->f_cred)) {
			/*
			 * Translation for backward compatibility.
			 */
			if (error == EAGAIN)
				error = EACCES;
			break;
		}
		if (uap->cmd == F_O_GETLK) {
			/*
			 * Return an SVR3 flock structure to the user.
			 */
			obf.l_type = bf.l_type;
			obf.l_whence = bf.l_whence;
			obf.l_start = bf.l_start;
			obf.l_len = bf.l_len;
			if (bf.l_sysid > SHRT_MAX || bf.l_pid > SHRT_MAX) {
				/*
				 * One or both values for the above fields
				 * is too large to store in an SVR3 flock
				 * structure.
				 */
				error = EOVERFLOW;
				break;
			}
			obf.l_sysid = (short) bf.l_sysid;
			obf.l_pid = (o_pid_t) bf.l_pid;
			if (copyout((caddr_t)&obf, (caddr_t)uap->arg,
			  sizeof obf))
				error = EFAULT;
		} else if (uap->cmd == F_GETLK) {
			/*
			 * Copy out SVR4 flock.
			 */
			int i;

			for (i = 0; i < 4; i++)
				/* Initialize pad until it's allocated. */
				bf.pad[i] = 0;
		    	if (copyout((caddr_t)&bf, (caddr_t)uap->arg, sizeof bf))
			  	error = EFAULT;
		}
		break;

	case F_RGETLK:
	case F_RSETLK:
	case F_RSETLKW:
		/*
		 * EFT only interface, applications cannot use
		 * this interface when _STYPES is defined.
		 * This interface supports an expanded
		 * flock struct--see fcntl.h.
		 */
		if (copyin((caddr_t)uap->arg, (caddr_t)&bf, sizeof bf)) {
			error = EFAULT;
			break;
		}
		if (bf.l_type != F_UNLCK) {
			u.u_procp->p_flag |= SLKDONE;
		}
		if (error =
		  VOP_FRLOCK(vp, uap->cmd, &bf, flag, offset, fp->f_cred)) {
			/*
			 * Translation for backward compatibility.
			 */
			if (error == EAGAIN)
				error = EACCES;
			break;
		}
		if (uap->cmd == F_RGETLK
		  && copyout((caddr_t)&bf, (caddr_t)uap->arg, sizeof bf))
			  error = EFAULT;
		break;

	/*
	 *      F_CNVT fcntl:  given a pointer to an fhandle_t and a mode, open
	 *      the file corresponding to the fhandle_t with the given mode and
	 *      return a file descriptor.  Note:  uap->fd is unused.
	 */
	 case F_CNVT:
		if (!suser(u.u_cred)) {
			error = EPERM;
			break;
		}
 
		if (error = copyin((caddr_t) uap->arg, (caddr_t) &a, sizeof(a))) {
			break;
		}
		else
			ap = &a;
		if (error = copyin((caddr_t) ap->fh, (caddr_t) &tfh, sizeof(tfh))) {
			break;
		}
 
		filemode = ap->filemode - FOPEN;
		if (filemode & FCREAT) {
			error = EINVAL;
			break;
		}
		mode = 0;
		if (filemode & FREAD)
			mode |= VREAD;
		if (filemode & (FWRITE | FTRUNC))
			mode |= VWRITE;
 
		/*
		 *      Adapted from copen:
		 */
		error = falloc((struct vnode *)NULL, filemode & FMASK,
			&fp, &fd);
		if (error)
			return error;
 
		/*
		 *      This takes the place of lookupname in copen.  Note that
		 *      we can't use the normal fhtovp function because we want
		 *      this to work on files that may not have been exported.
		 */
		if ((vp = myfhtovp(&tfh)) == (struct vnode *) NULL) {
			error = ESTALE;
			goto out;
		}
 
		/*
		 *      Adapted from vn_open:
		 */
		if (filemode & (FWRITE | FTRUNC)) {
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
				if ((error = VOP_GETATTR(vp, &vattr, 0, u.u_cred)) == 0
				  && MANDLOCK(vp, vattr.va_mode))
					error = EAGAIN;
			}
			if (error)
				goto out;
		}
		/*
		 * Check permissions.
		 * Must have read and write permission to open a file for
		 * private access.
		 */
		if (error = VOP_ACCESS(vp, mode, 0, u.u_cred)) {
			goto out;
		}
		error = VOP_OPEN(&vp, filemode, u.u_cred);
		if ((error == 0) && (filemode & FTRUNC)) {
			struct vattr vattr;
 
			vattr.va_size = 0;
			vattr.va_mask = AT_SIZE;
			error = VOP_SETATTR(vp, &vattr, 0, u.u_cred);
	       }
	       if (error)
			goto out;
 
		/*
		 *      Adapted from copen:
		 */
		fp->f_vnode = vp;
		rvp->r_val1 = fd;
		break;
out:
		crfree(fp->f_cred);
		fp->f_count = 0;
		if (vp)
			VN_RELE(vp);
		break;

	case F_CHKFL:
		/*
		 * This is for internal use only, to allow the vnode layer
		 * to validate a flags setting before applying it.  User
		 * programs can't issue it.
		 */
		error = EINVAL;
		break;

	case F_ALLOCSP:
	case F_FREESP:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else if (vp->v_type != VREG)
			error = EINVAL;
		/*
		 * For compatibility we overlay an SVR3 flock on an SVR4
		 * flock.  This works because the input field offsets 
		 * in "struct flock" were preserved.
		 */
		else if (copyin((caddr_t)uap->arg, (caddr_t)&bf, sizeof obf))
			error = EFAULT;
		else
			error =
			 VOP_SPACE(vp, uap->cmd, &bf, flag, offset, fp->f_cred);
		break;

	case F_BLOCKS:
	case F_BLKSIZE:
	{
		struct vattr vattr;		
		long int n;

		vattr.va_mask = AT_BLKSIZE | AT_NBLOCKS;
		if (error = VOP_GETATTR(vp, &vattr, 0, u.u_cred))
			break;
		n = (uap->cmd == F_BLOCKS) ?
		  vattr.va_nblocks : vattr.va_blksize;
		if (copyout((caddr_t)&n, (caddr_t)uap->arg, sizeof(long)))
			error = EFAULT;
		break;
	}

	default:
		error = EINVAL;
		break;
	}

	return error;
}

/*
 * We require a version of fhtovp that simply converts an fhandle_t to
 * a vnode without any ancillary checking (e.g., whether it's exported).
 */
STATIC struct vnode *
myfhtovp(fh)
	fhandle_t *fh;
{
	int error;
	struct vnode *vp;
	register struct vfs *vfsp;

	vfsp = getvfs(&fh->fh_fsid);
	if (vfsp == (struct vfs *) NULL) {
		return ((struct vnode *) NULL);
	}
	error = VFS_VGET(vfsp, &vp, (struct fid *)&(fh->fh_len));
	if (error || vp == (struct vnode *) NULL) {
		return ((struct vnode *) NULL);
	}
	return (vp);
}

/*
 * Duplicate a file descriptor.
 */
struct dupa {
	int	fdes;
};

int
dup(uap, rvp)
	register struct dupa *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int error;
	int fd;

	if (error = getf(uap->fdes, &fp))
		return error;
	if (error = ufalloc(0, &fd))
		return error;
	setf(fd, fp);
	fp->f_count++;
	rvp->r_val1 = fd;
	return 0;
}

/*
 * I/O control.
 */
struct ioctla {
	int fdes;
	int cmd;
	int arg;
};

int
ioctl(uap, rvp)
	register struct ioctla *uap;
	rval_t *rvp;
{
	file_t *fp;
	register int error;
	register vnode_t *vp;
	struct vattr vattr;
	off_t offset;
	int flag;

	if (error = getf(uap->fdes, &fp))
		return error;
	if (setjmp(&u.u_qsav))
		return intrerr(1);
	vp = fp->f_vnode;

	if (vp->v_type == VREG || vp->v_type == VDIR) {
		/*
		 * Handle these two ioctls for regular files and
		 * directories.  All others will usually be failed
		 * with ENOTTY by the VFS-dependent code.  System V
		 * always failed all ioctls on regular files, but SunOS
		 * supported these.
		 */
		switch (uap->cmd) {
		case FIONREAD:
			if (error = VOP_GETATTR(vp, &vattr, 0, u.u_cred))
				return error;
			offset = vattr.va_size - fp->f_offset;
			if (copyout((caddr_t)uap->arg, (caddr_t)&offset,
			  sizeof(offset)))
				return EFAULT;
			return 0;

		case FIONBIO:
			if (copyin((caddr_t)uap->arg, (caddr_t)&flag, 
			  sizeof(int)))
				return EFAULT;
			if (flag)
				fp->f_flag |= FNDELAY;
			else
				fp->f_flag &= ~FNDELAY;
			return 0;

		default:
			break;
		}
	}
	error = VOP_IOCTL(fp->f_vnode, uap->cmd, uap->arg,
	    fp->f_flag, fp->f_cred, &rvp->r_val1);
	if (error == 0) {
		switch (uap->cmd) {
		case FIONBIO:
			if (copyin((caddr_t)uap->arg, (caddr_t)&flag,
			  sizeof(int)))
				return EFAULT;		/* XXX */
			if (flag)
				fp->f_flag |= FNDELAY;
			else
				fp->f_flag &= ~FNDELAY;
			break;

		default:
			break;
	    }
	}
	return error;
}

/*
 * Old stty and gtty.  (Still.)
 */
struct sgttya {
	int	fdes;
	int	arg;
};

int
stty(uap, rvp)
	register struct sgttya *uap;
	rval_t *rvp;
{
	struct ioctla na;

	na.fdes = uap->fdes;
	na.cmd = TIOCSETP;
	na.arg = uap->arg;
	return ioctl(&na, rvp);
}

int
gtty(uap, rvp)
	register struct sgttya *uap;
	rval_t *rvp;
{
	struct ioctla na;

	na.fdes = uap->fdes;
	na.cmd = TIOCGETP;
	na.arg = uap->arg;
	return ioctl(&na, rvp);
}

/*
 * Poll file descriptors for interesting events.
 */
int pollwait;

struct polla {
	struct pollfd *fdp;
	unsigned long nfds;
	long	timo;
};

int
poll(uap, rvp)
	register struct polla *uap;
	rval_t *rvp;
{
	register int i, s;
	register fdcnt = 0;
	struct pollfd *pollp = NULL;
	struct pollfd parray[NFPCHUNK];
	time_t t;
	int lastd;
	int rem;
	int id;
	int psize;
	int dsize;
	file_t *fp;
	struct pollhead *php;
	struct pollhead *savehp = NULL;
	struct polldat *darray;
	struct polldat *curdat;
	int error = 0;
	proc_t *p = u.u_procp;
	extern time_t lbolt;

	if (uap->nfds < 0 || uap->nfds > u.u_nofiles)
		return EINVAL;
	t = lbolt;

	/*
	 * Allocate space for the pollfd array and space for the
	 * polldat structures used by polladd().  Then copy in
	 * the pollfd array from user space.
	 */
	if (uap->nfds != 0) {
		psize = uap->nfds * sizeof(struct pollfd);
		if (uap->nfds <= NFPCHUNK)
			pollp = parray;
		else if ((pollp = kmem_alloc(psize, KM_NOSLEEP)) == NULL)
				return EAGAIN;
		dsize = uap->nfds * sizeof(struct polldat);
		if ((darray = kmem_zalloc(dsize, KM_NOSLEEP)) == NULL) {
			if (pollp != parray)
				kmem_free((caddr_t)pollp, psize);
			return EAGAIN;
		}
		if (copyin((caddr_t)uap->fdp, (caddr_t)pollp, psize)) {
			error = EFAULT;
			goto pollout;
		}

		/*
		 * Chain the polldat array together.
		 */
		lastd = uap->nfds - 1;
		if (lastd > 0) {
			darray[lastd].pd_chain = darray;
			for (i = 0; i < lastd; i++) {
				darray[i].pd_chain = &darray[i+1];
			}
		} else {
			darray[0].pd_chain = darray;
		}
		curdat = darray;
	}

	/*
	 * Retry scan of fds until an event is found or until the
	 * timeout is reached.
	 */
retry:		

	/*
	 * Polling the fds is a relatively long process.  Set up the
	 * SINPOLL flag so that we can see if something happened
	 * to an fd after we checked it but before we go to sleep.
	 */
	p->p_pollflag = SINPOLL;
	if (savehp) {			/* clean up from last iteration */
		polldel(savehp, --curdat);
		savehp = NULL;
	}
	curdat = darray;
	for (i = 0; i < uap->nfds; i++) {
		php = NULL;
		if (pollp[i].fd < 0) 
			pollp[i].revents = 0;
		else if (pollp[i].fd >= u.u_nofiles || getf(pollp[i].fd, &fp))
			pollp[i].revents = POLLNVAL;
		else {
			error = VOP_POLL(fp->f_vnode, pollp[i].events, fdcnt,
			    &pollp[i].revents, &php);
			if (error)
				goto pollout;
		}
		if (pollp[i].revents)
			fdcnt++;
		else if (fdcnt == 0 && php) {
			s = splhi();
			if (!(p->p_pollflag & SINPOLL))
				curdat = darray;
			polladd(php, pollp[i].events, pollrun,
			  (long)p, curdat++);
			splx(s);
			savehp = php;
		}
	}
	if (fdcnt) 
		goto pollout;

	/*
	 * If you get here, the poll of fds was unsuccessful.
	 * First make sure your timeout hasn't been reached.
	 * If not then sleep and wait until some fd becomes
	 * readable, writeable, or gets an exception.
	 */
	rem = uap->timo < 0 ? 1 : uap->timo - ((lbolt - t)*1000)/HZ;
	if (rem <= 0)
		goto pollout;

	s = splhi();

	/*
	 * If anything has happened on an fd since it was checked, it will
	 * have turned off SINPOLL.  Check this and rescan if so.
	 */
	if (!(p->p_pollflag & SINPOLL)) {
		splx(s);
		goto retry;
	}
	p->p_pollflag &= ~SINPOLL;

	if (uap->timo > 0) {
		/*
		 * Turn rem into milliseconds and round up.
		 */
		rem = ((rem/1000) * HZ) + ((((rem%1000) * HZ) + 999) / 1000);
		p->p_pollflag |= SPOLLTIME;
		id = timeout((void(*)())polltime, (caddr_t)p, rem);
	}

	/*
	 * The sleep will usually be awakened either by this poll's timeout 
	 * (which will have cleared SPOLLTIME), or by the pollwakeup function 
	 * called from either the VFS, the driver, or the stream head.
	 */
	if (sleep((caddr_t)&pollwait, (PZERO+1)|PCATCH)) {
		if (uap->timo > 0)
			untimeout(id);
		splx(s);
		error = EINTR;
		goto pollout;
	}
	splx(s);

	/*
	 * If SPOLLTIME is still set, you were awakened because an event
	 * occurred (data arrived, can write now, or exceptional condition).
	 * If so go back up and poll fds again. Otherwise, you've timed
	 * out so you will fall through and return.
	 */
	if (uap->timo > 0) {
		if (p->p_pollflag & SPOLLTIME) {
			untimeout(id);
			goto retry;
		}
	} else
		goto retry;

pollout:

	/*
	 * Poll cleanup code.
	 */
	p->p_pollflag = 0;
	if (savehp)
		polldel(savehp, --curdat);
	if (error == 0) {
		/*
		 * Copy out the events and return the fdcnt to the user.
		 */
		rvp->r_val1 = fdcnt;
		if (uap->nfds != 0)
			if (copyout((caddr_t)pollp, (caddr_t)uap->fdp, psize))
				error = EFAULT;
	}
	if (uap->nfds != 0) {
		kmem_free((caddr_t)darray, dsize);
		if (pollp != parray)
			kmem_free((caddr_t)pollp, psize);
	}
	return error;
}

/*
 * This function is placed in the callout table to time out a process
 * waiting on poll.  If the poll completes, this function is removed
 * from the table.  Its argument is a flag to the caller indicating a
 * timeout occurred.
 */
void
polltime(p)
	register proc_t *p;
{
	if (p->p_wchan == (caddr_t)&pollwait) {
		setrun(p);
		p->p_pollflag &= ~SPOLLTIME;
	}
}

/*
 * This function is called to inform a process that
 * an event being polled for has occurred.
 */
void
pollrun(p)
	register proc_t *p;
{
	register int s;

	s = splhi();
	if (p->p_wchan == (caddr_t)&pollwait) {
		if (p->p_stat == SSLEEP)
			setrun(p);
		else
			unsleep(p);
	}
	p->p_pollflag &= ~SINPOLL;
	splx(s);
}

int pollcoll = 0;	/* collision counter (temporary) */

/*
 * This function allocates a polldat structure, fills in the given
 * data, and places it on the given pollhead list.  This routine MUST
 * be called at splhi() to avoid races.
 */
void
polladd(php, events, fn, arg, pdp)
	register struct pollhead *php;
	short events;
	void (*fn)();
	long arg;
	register struct polldat *pdp;
{
	pdp->pd_events = events;
	pdp->pd_fn = fn;
	pdp->pd_arg = arg;
	if (php->ph_list) {
		pdp->pd_next = php->ph_list;
		php->ph_list->pd_prev = pdp;
		if (php->ph_events & events)
			pollcoll++;
	} else {
		pdp->pd_next = NULL;
	}
	pdp->pd_prev = (struct polldat *)php;
	pdp->pd_headp = php;
	php->ph_list = pdp;
	php->ph_events |= events;
}

/*
 * This function frees all the polldat structures related by
 * the sibling chain pointer.  These were all the polldats
 * allocated as the result of one poll system call.  Because
 * of race conditions, pdp may not be on php's list.
 */
void
polldel(php, pdp)
	register struct pollhead *php;
	register struct polldat *pdp;
{
	register struct polldat *p;
	register struct polldat *startp;
	register int s;

	s = splhi();
	for (p = php->ph_list; p; p = p->pd_next) {
		if (p == pdp) {
			startp = p;
			do {
				if (p->pd_headp != NULL) {
					if (p->pd_next)
						p->pd_next->pd_prev =
						  p->pd_prev;
					p->pd_prev->pd_next = p->pd_next;

					/*
					 * Recalculate the events on the list.
					 * The list is short - trust me.
					 * Note reuse of pdp here.
					 */
					p->pd_headp->ph_events = 0;
					for (pdp = p->pd_headp->ph_list;
					  pdp; pdp = pdp->pd_next)
						p->pd_headp->ph_events |=
						  pdp->pd_events;
					p->pd_next = NULL;
					p->pd_prev = NULL;
					p->pd_headp = NULL;
				}
				p = p->pd_chain;
			} while (p != startp);
			splx(s);
			return;
		}
	}
	splx(s);
}
