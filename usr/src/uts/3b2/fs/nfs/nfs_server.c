/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/nfs/nfs_server.c	1.12"

/*	@(#)nfs_server.c 2.87 88/04/12 SMI	*/

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

#define NFSSERVER

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/cred.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/pathname.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/sysmacros.h>
#include <sys/siginfo.h>
#include <netinet/in.h>
#include <sys/tiuser.h>
#include <sys/statvfs.h>
#include <sys/t_kuser.h>
#include <sys/kmem.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/auth_unix.h>
#include <rpc/auth_des.h>
#include <rpc/svc.h>
#include <rpc/xdr.h>
#include <nfs/nfs.h>
#include <nfs/export.h>
#include <sys/dirent.h>
#include <sys/cmn_err.h>
#include <sys/debug.h>
#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <vm/seg_kmem.h>

extern struct vnodeops nfs_vnodeops;
extern int nfsdprintf;
STATIC	void	rfs_getattr();
STATIC	void	rfs_setattr();
STATIC	void	rfs_lookup();
STATIC	void	rfs_readlink();
STATIC	void	rfs_read();
STATIC	void	rfs_write();
STATIC	void	rfs_create();
STATIC	void	rfs_remove();
STATIC	void	rfs_rename();
STATIC	void	rfs_link();
STATIC	void	rfs_symlink();
STATIC	void	rfs_mkdir();
STATIC	void	rfs_rmdir();
STATIC	void	rfs_readdir();
STATIC	void	rfs_statfs();
STATIC	void	rfs_null();
STATIC	void	rfs_error();

STATIC	void	rfs_rlfree();
STATIC	void	rfs_rdfree();
STATIC	void	rfs_rddirfree();
STATIC	void	nullfree();

STATIC	void	rfsput();
/*
 * rpc service program version range supported
 */
#define	VERSIONMIN	2
#define	VERSIONMAX	2

/*
 * Returns true iff exported filesystem is read-only
 */
#ifdef SYSV
#define rdonly(exi, req) (((exi)->exi_export.ex_flags & EX_RDONLY) || \
			  (((exi)->exi_export.ex_flags & EX_RDMOSTLY) && \
			   !hostinlist(svc_getcaller((req)->rq_xprt), \
					&(exi)->exi_export.ex_writeaddrs)))
#else
#define rdonly(exi, req) (((exi)->exi_export.ex_flags & EX_RDONLY) || \
			  (((exi)->exi_export.ex_flags & EX_RDMOSTLY) && \
			   !hostinlist((struct sockaddr *)\
					svc_getcaller((req)->rq_xprt), \
					&(exi)->exi_export.ex_writeaddrs)))
#endif

struct vnode	*fhtovp();
struct file	*getsock();
void		svcerr_progvers();
void		rfs_dispatch();

#ifdef NFSDEBUG
extern int nfsdebug;
#endif

struct {
	int	ncalls;		/* number of calls received */
	int	nbadcalls;	/* calls that failed */
	int	reqs[32];	/* count for each request */
} svstat;

/* STATIC int nfs_chars = 50000; */

struct nfs_svc_args {
	int fd;
};
STATIC int nfs_server_count = 0;

/*
 * NFS Server system call.
 * Does all of the work of running a NFS server.
 * sock is the fd of an open UDP socket.
 */
nfs_svc(uap)
	struct nfs_svc_args *uap;
{
	struct file *fp;
	SVCXPRT *xprt;
	u_long vers;
	/* int error; */

	/* disassociate this process from the terminal */
	newsession();

	if (getf(uap->fd, &fp)) {
		return EBADF;
	}

	/* Now, release client memory; we never return back to user */
	relvm(u.u_procp);

	if ((xprt = svc_tli_kcreate(fp, 0, 0)) == (SVCXPRT *)NULL) {
		if (nfsdprintf)
			printf("nfs_svc: svc_tli_kcreate failed %d, exiting\n", u.u_error);
		/* return (u.u_error); */
		exit(CLD_EXITED, 0);
	}

	for (vers = VERSIONMIN; vers <= VERSIONMAX; vers++) {
		(void) svc_register(xprt, NFS_PROGRAM, vers, rfs_dispatch,
		    FALSE);
	}
	if (setjmp(&u.u_qsav)) {
		if (--nfs_server_count == 0) {
			for (vers = VERSIONMIN; vers <= VERSIONMAX; vers++) {
				svc_unregister(NFS_PROGRAM, vers);
			}
		}
		SVC_DESTROY(xprt);
		u.u_error = EINTR;	/* not needed; may help debugging */
		/* sigclearall(u.u_procp); */	/* gone, so duplicate it's effects */
		{
			struct proc *p = u.u_procp;
			p->p_cursig = 0;
			if (p->p_curinfo) {
				kmem_free((caddr_t)p->p_curinfo, sizeof(*p->p_curinfo));
				p->p_curinfo = NULL;
			}
		}
		exit(CLD_EXITED, 0);
	} else {
		nfs_server_count++;
		svc_run(xprt);  /* never returns */
	}
	/* NOTREACHED */
}


/*
 * These are the interface routines for the server side of the
 * Networked File System.  See the NFS protocol specification
 * for a description of this interface.
 */


/*
 * Get file attributes.
 * Returns the current attributes of the file with the given fhandle.
 */
STATIC void
rfs_getattr(fhp, ns, exi)
	fhandle_t *fhp;
	register struct nfsattrstat *ns;
	struct exportinfo *exi;
{
	int error;
	register struct vnode *vp;
	struct vattr va;

#ifdef NFSDEBUG
	printf("rfs_getattr fh %x %x %d\n",
	    fhp->fh_fsid.val[0], fhp->fh_fsid.val[1], fhp->fh_len);
#endif
	vp = fhtovp(fhp, exi);
	if (vp == NULL) {
		ns->ns_status = NFSERR_STALE;
		return;
	}
	va.va_mask = AT_ALL;	/* we want all the attributes */
	error = VOP_GETATTR(vp, &va, 0, u.u_cred);
	if (!error) {
		vattr_to_nattr(&va, &ns->ns_attr);
	}
	ns->ns_status = puterrno(error);
	VN_RELE(vp);
#ifdef NFSDEBUG
	printf("rfs_getattr: returning %d\n", error);
#endif
}

/*
 * Set file attributes.
 * Sets the attributes of the file with the given fhandle.  Returns
 * the new attributes.
 */
STATIC void
rfs_setattr(args, ns, exi, req)
	struct nfssaargs *args;
	register struct nfsattrstat *ns;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	int flag;
	register struct vnode *vp;
	struct vattr va;

#ifdef NFSDEBUG
	printf("rfs_setattr fh %x %x %d\n",
	    args->saa_fh.fh_fsid.val[0], args->saa_fh.fh_fsid.val[1],
	    args->saa_fh.fh_len);
#endif
	vp = fhtovp(&args->saa_fh, exi);
	if (vp == NULL) {
		ns->ns_status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req)) {
		error = EROFS;
	} else {
		sattr_to_vattr(&args->saa_sa, &va);

		/*
		 * Allow SysV-compatible option to set access and
		 * modified times if root, owner, or write access.
		 *
		 * XXX - Until an NFS Protocol Revision, this may be
		 *       simulated by setting the client time in the
		 *       tv_sec field of the access and modified times
		 *       and setting the tv_nsec field of the modified
		 *       time to an invalid value (1,000,000).  This
		 *       may be detected by servers modified to do the
		 *       right thing, but will not be disastrous on
		 *       unmodified servers.
		 * XXX - 1,000,000 is actually a valid tv_nsec value,
		 *       but we can't change it because that's what NFS uses.
		 *       So a valid number will produce incorrect results.
		 * XXX - For now, va_mtime.tv_nsec == -1 flags this in
		 *	 VOP_SETATTR().
		 * XXX - No it doesn't. At least s5setattr() just uses what's
		 *       in vap->va_mtime|vap->va_atime.
		 */
		if ((va.va_mtime.tv_sec != -1UL) &&
		    (va.va_mtime.tv_nsec == 1000000)) {
			va.va_mtime.tv_sec = hrestime.tv_sec;
			va.va_mtime.tv_nsec = hrestime.tv_nsec;
			va.va_atime.tv_sec = va.va_mtime.tv_sec;
			va.va_atime.tv_nsec = va.va_mtime.tv_nsec;
			flag = 0;
		} else
			flag = ATTR_UTIME;

		if (va.va_mode != (mode_t) -1)
			va.va_mask |= AT_MODE;
		if (va.va_uid != (uid_t) -1)
			va.va_mask |= AT_UID;
		if (va.va_gid != (uid_t) -1)
			va.va_mask |= AT_GID;
		if (va.va_size != (ulong) -1)
			va.va_mask |= AT_SIZE;
		if (va.va_atime.tv_sec != (unsigned long) -1)
			va.va_mask |= AT_ATIME;
		if (va.va_mtime.tv_sec != (unsigned long) -1)
			va.va_mask |= AT_MTIME;

		error = VOP_SETATTR(vp, &va, flag, u.u_cred);
		if (!error) {
			va.va_mask = AT_ALL;	/* get everything */
			error = VOP_GETATTR(vp, &va, 0, u.u_cred);
			if (!error) {
				vattr_to_nattr(&va, &ns->ns_attr);
			}
		}
	}
	ns->ns_status = puterrno(error);
	VN_RELE(vp);
#ifdef NFSDEBUG
	printf("rfs_setattr: returning %d\n", error);
#endif
}

/*
 * Directory lookup.
 * Returns an fhandle and file attributes for file name in a directory.
 */
STATIC void
rfs_lookup(da, dr, exi)
	struct nfsdiropargs *da;
	register struct  nfsdiropres *dr;
	struct exportinfo *exi;
{
	int error;
	register struct vnode *dvp;
	struct vnode *vp;
	struct vattr va;

#ifdef NFSDEBUG
	printf("rfs_lookup %s fh %x %x %x %d\n",
	    da->da_name, da->da_fhandle.fh_fsid.val[0],
	    da->da_fhandle.fh_fsid.val[1], da->da_fhandle.fh_len);
#endif
	dvp = fhtovp(&da->da_fhandle, exi);
	if (dvp == NULL) {
		dr->dr_status = NFSERR_STALE;
		return;
	}

	/*
	 * do lookup.
	 */
	error = VOP_LOOKUP(dvp, da->da_name, &vp, (struct pathname *) NULL,
				0,
				(struct vnode *) 0,	/* XXX - unused? */
				u.u_cred);
	if (error) {
		vp = (struct vnode *)NULL;
	} else {
		va.va_mask = AT_ALL;	/* we want everything */
		error = VOP_GETATTR(vp, &va, 0, u.u_cred);
		if (!error) {
			vattr_to_nattr(&va, &dr->dr_attr);
			error = makefh(&dr->dr_fhandle, vp, exi);
		}
	}
	dr->dr_status = puterrno(error);
	if (vp) {
		VN_RELE(vp);
	}
	VN_RELE(dvp);
#ifdef NFSDEBUG
	printf("rfs_lookup: returning %d\n", error);
#endif
}

/*
 * Read symbolic link.
 * Returns the string in the symbolic link at the given fhandle.
 */
STATIC void
rfs_readlink(fhp, rl, exi)
	fhandle_t *fhp;
	register struct nfsrdlnres *rl;
	struct exportinfo *exi;
{
	int error;
	struct iovec iov;
	struct uio uio;
	struct vnode *vp;

#ifdef NFSDEBUG
	printf("rfs_readlink fh %x %x %d\n",
	    fhp->fh_fsid.val[0], fhp->fh_fsid.val[1], fhp->fh_len);
#endif
	vp = fhtovp(fhp, exi);
	if (vp == NULL) {
		rl->rl_status = NFSERR_STALE;
		return;
	}

	/*
	 * Allocate data for pathname.  This will be freed by rfs_rlfree.
	 */
	rl->rl_data = (char *)kmem_alloc((u_int)MAXPATHLEN, KM_SLEEP);

	/*
	 * Set up io vector to read sym link data
	 */
	iov.iov_base = rl->rl_data;
	iov.iov_len = NFS_MAXPATHLEN;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_offset = 0;
	uio.uio_resid = NFS_MAXPATHLEN;

	/*
	 * read link
	 */
	error = VOP_READLINK(vp, &uio, u.u_cred);

	/*
	 * Clean up
	 */
	if (error) {
		kmem_free((caddr_t)rl->rl_data, (u_int)NFS_MAXPATHLEN);
		rl->rl_count = 0;
		rl->rl_data = NULL;
	} else {
		rl->rl_count = NFS_MAXPATHLEN - uio.uio_resid;
	}
	rl->rl_status = puterrno(error);
	VN_RELE(vp);
#ifdef NFSDEBUG
	printf("rfs_readlink: returning '%s' %d\n",
	    rl->rl_data, error);
#endif
}

/*
 * Free data allocated by rfs_readlink
 */
STATIC void
rfs_rlfree(rl)
	struct nfsrdlnres *rl;
{

	if (rl->rl_data) {
		kmem_free((caddr_t)rl->rl_data, (u_int)NFS_MAXPATHLEN);
	}
}

int nfsreadmap = 1;

/*
 * Read data.
 * Returns some data read from the file at the given fhandle.
 */
STATIC void
rfs_read(ra, rr, exi)
	register struct nfsreadargs *ra;
	register struct nfsrdresult *rr;
	struct exportinfo *exi;
{
	register struct vnode *vp;
	register int error;
	struct vattr va;
	struct iovec iov;
	struct uio uio;
	int offset;
	char *savedatap;

#ifdef NFSDEBUG
	printf("rfs_read %d from fh %x %x %d\n",
	    ra->ra_count, ra->ra_fhandle.fh_fsid.val[0],
	    ra->ra_fhandle.fh_fsid.val[1], ra->ra_fhandle.fh_len);
#endif
	rr->rr_data = NULL;
	rr->rr_count = 0;
	vp = fhtovp(&ra->ra_fhandle, exi);
	if (vp == NULL) {
		rr->rr_status = NFSERR_STALE;
		return;
	}
	if (vp->v_type != VREG) {
		if (nfsdprintf) printf("rfs_read: attempt to read from non-file\n");
		error = EISDIR;
	} else {
		VOP_RWLOCK(vp);
		va.va_mask = AT_STAT|AT_TYPE|AT_BLKSIZE; /* everything but AT_NBLOCKS */
		error = VOP_GETATTR(vp, &va, 0, u.u_cred);
	}
	if (error) {
		goto bad;
	}

	/*
	 * This is a kludge to allow reading of files created
	 * with no read permission.  The owner of the file
	 * is always allowed to read it.
	 */
	if (u.u_cred->cr_uid != va.va_uid) {
		error = VOP_ACCESS(vp, VREAD, 0, u.u_cred);
		if (error) {
			/*
			 * Exec is the same as read over the net because
			 * of demand loading.
			 */
			error = VOP_ACCESS(vp, VEXEC, 0, u.u_cred);
		}
		if (error) {
			goto bad;
		}
	}

	if (ra->ra_offset >= va.va_size) {
		rr->rr_count = 0;
		vattr_to_nattr(&va, &rr->rr_attr);
		goto done;			/* hit EOF */
	}

	/*
	 * Check whether we can do this with segmap,
	 * which would save the copy through the uio.
	 */
	offset = ra->ra_offset & MAXBOFFSET;
#ifdef	SYSV
#ifdef	VNOCACHE
	if (nfsreadmap && (offset + ra->ra_count <= MAXBSIZE) &&
	    (vp->v_flag & VNOCACHE) == 0) {
#else
	if (nfsreadmap && (offset + ra->ra_count <= MAXBSIZE)) {
#endif
#else	/* SYSV */
	if (nfsreadmap && (offset + ra->ra_count <= MAXBSIZE) &&
	    (vp->v_flag & (VNOCACHE | VNOMAP)) == 0) {
#endif	/* SYSV */
		faultcode_t fault_error;

		rr->rr_map = segmap_getmap(segkmap, vp,
		    (u_int)(ra->ra_offset & MAXBMASK));
		rr->rr_data = rr->rr_map + offset;
		rr->rr_count = MIN(va.va_size - ra->ra_offset, ra->ra_count);
		/*
		 * Fault in and lock down the pages.
		 */
		fault_error = as_fault(&kas, rr->rr_data, (u_int)rr->rr_count,
		    F_SOFTLOCK, S_READ);
		if (fault_error == 0) {
			VN_HOLD(vp);
			rr->rr_vp = vp;
			vattr_to_nattr(&va, &rr->rr_attr);
			goto done;
		} else {
			if (FC_CODE(fault_error) == FC_OBJERR)
				error = FC_ERRNO(fault_error);
			else
				error = EIO;
			if (nfsdprintf) printf("rfs_read: map failed, error = %d\n", error);
			(void) segmap_release(segkmap, rr->rr_map, 0);
			/* Fall through and try just doing a read */
		}
	}
	rr->rr_map = NULL;

	/*
	 * Allocate space for data.  This will be freed by xdr_rdresult
	 * when it is called with x_op = XDR_FREE.
	 */
	rr->rr_data = kmem_alloc((u_int)ra->ra_count, KM_SLEEP);

	/*
	 * Set up io vector
	 */
	iov.iov_base = rr->rr_data;
	iov.iov_len = ra->ra_count;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_offset = ra->ra_offset;
	uio.uio_resid = ra->ra_count;
	/*
	 * For now we assume no append mode and
	 * ignore totcount (read ahead)
	 */
	error = VOP_READ(vp, &uio, IO_SYNC, u.u_cred);
	if (error) {
		goto bad;
	}
	vattr_to_nattr(&va, &rr->rr_attr);
	rr->rr_count = ra->ra_count - uio.uio_resid;
	/*
	 * free the unused part of the data allocated
	 * NO!! FIX THIS!	XXX
	 */
	if (uio.uio_resid) {
		savedatap = rr->rr_data;
		rr->rr_data = kmem_alloc ((u_int)rr->rr_count, KM_SLEEP);
		bcopy (savedatap, rr->rr_data, rr->rr_count);
		kmem_free(savedatap, (u_int)ra->ra_count);
	}
bad:
	if (error && rr->rr_data != NULL) {
		kmem_free(rr->rr_data, (u_int)ra->ra_count);
		rr->rr_data = NULL;
		rr->rr_count = 0;
	}
done:
	rr->rr_status = puterrno(error);
	if (vp->v_type == VREG)
		VOP_RWUNLOCK(vp);
	VN_RELE(vp);
#ifdef NFSDEBUG
	printf("rfs_read returning %d, count = %d\n",
	    error, rr->rr_count);
#endif
}

/*
 * Free data allocated by rfs_read.
 */
STATIC void
rfs_rdfree(rr)
	struct nfsrdresult *rr;
{

	if (rr->rr_map == NULL && rr->rr_data != NULL) {
		kmem_free(rr->rr_data, (u_int)rr->rr_count);
	}
}

/*
 * Write data to file.
 * Returns attributes of a file after writing some data to it.
 */
STATIC void
rfs_write(wa, ns, exi, req)
	struct nfswriteargs *wa;
	struct nfsattrstat *ns;
	register struct exportinfo *exi;
	struct svc_req *req;
{
	register int error;
	register struct vnode *vp;
	struct vattr va;
	struct iovec iov;
	struct uio uio;

#ifdef NFSDEBUG
	printf("rfs_write: %d bytes fh %x %x %d\n",
	    wa->wa_count, wa->wa_fhandle.fh_fsid.val[0],
	    wa->wa_fhandle.fh_fsid.val[1], wa->wa_fhandle.fh_len);
#endif
	vp = fhtovp(&wa->wa_fhandle, exi);
	if (vp == NULL) {
		ns->ns_status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req)) {
		error = EROFS;
	} else if (vp->v_type != VREG) {
		if (nfsdprintf) printf("rfs_write: attempt to write to non-file\n");
		error = EISDIR;
	} else {
		VOP_RWLOCK(vp);
		/* va.va_mask = AT_ALL;	/* we want everything */
		va.va_mask = AT_UID;	/* all we need is the uid */
		error = VOP_GETATTR(vp, &va, 0, u.u_cred);
	}
	if (!error) {
		if (u.u_cred->cr_uid != va.va_uid) {
			/*
			 * This is a kludge to allow writes of files created
			 * with read only permission.  The owner of the file
			 * is always allowed to write it.
			 */
			error = VOP_ACCESS(vp, VWRITE, 0, u.u_cred);
		}
		if (!error) {
			if (wa->wa_data) {
				iov.iov_base = wa->wa_data;
				iov.iov_len = wa->wa_count;
				uio.uio_iov = &iov;
				uio.uio_iovcnt = 1;
				uio.uio_segflg = UIO_SYSSPACE;
				uio.uio_offset = wa->wa_offset;
				uio.uio_resid = wa->wa_count;
				{	register ulong rlimit =
						u.u_rlimit[RLIMIT_FSIZE].rlim_cur - wa->wa_offset;
					if (rlimit < uio.uio_resid && rlimit > 0)
						uio.uio_resid = rlimit;
				}
				/*
				 * The limit is checked on the client. We should allow
				 * any size writes here.
				 */
				uio.uio_limit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur;
				/*
				 * for now we assume no append mode
				 */
				error = VOP_WRITE(vp, &uio, IO_SYNC, u.u_cred);
			} else { 	/* mbuf hack */
if (nfsdprintf) printf("rfs_write: hit mbuf hack...\n");
/*
 *				register struct mbuf *m;
 *				register struct iovec *iovp;
 *				register int iovcnt;
 *				static caddr_t *base10, *base40;
 *
 *				iovcnt = 0;
 *				for (m = (struct mbuf *)wa->wa_mbuf; m;
 *					m = m->m_next) {
 *					iovcnt++;
 *				}
 *				if (iovcnt < 10) {
 *					iovp = (struct iovec *)kmem_fast_alloc(
 *					    (caddr_t *)&base10,
 *					    sizeof (struct iovec) * 10, 10, KM_SLEEP);
 *				} else if (iovcnt < 40) {
 *					iovp = (struct iovec *)kmem_fast_alloc(
 *					    (caddr_t *)&base40,
 *					    sizeof (struct iovec) * 40, 10, KM_SLEEP);
 *				} else {
 *					iovp = (struct iovec *)kmem_alloc(
 *					    (u_int)(sizeof (struct iovec) *
 *					    iovcnt), KM_SLEEP);
 *				}
 *				mbuf_to_iov((struct mbuf *)wa->wa_mbuf, iovp);
 *				uio.uio_iov = iovp;
 *				uio.uio_iovcnt = iovcnt;
 *				uio.uio_segflg = UIO_SYSSPACE;
 *				uio.uio_offset = wa->wa_offset;
 *				uio.uio_resid = wa->wa_count;
 */
				/*
				 * for now we assume no append mode
				 */
/*
 *				error = VOP_WRITE(vp, &uio, IO_SYNC, u.u_cred);
 *				if (iovcnt < 10) {
 *					kmem_fast_free((caddr_t *)&base10,
 *					    (caddr_t)iovp);
 *				} else if (iovcnt < 40) {
 *					kmem_fast_free((caddr_t *)&base40,
 *					    (caddr_t)iovp);
 *				} else {
 *					kmem_free((caddr_t)iovp,
 *					    (u_int)(sizeof (struct iovec) *
 *					    iovcnt));
 *				}
 */
			}
		}
	}
	if (!error) {
		/*
		 * Get attributes again so we send the latest mod
		 * time to the client side for his cache.
		 */
		/* va.va_mask = AT_ALL;	/* now we want everything */
		error = VOP_GETATTR(vp, &va, 0, u.u_cred);
	}
	ns->ns_status = puterrno(error);
	if (!error) {
		vattr_to_nattr(&va, &ns->ns_attr);
	}
	if ((!rdonly(exi, req)) && vp->v_type == VREG)
		VOP_RWUNLOCK(vp);
	VN_RELE(vp);
#ifdef NFSDEBUG
	printf("rfs_write: returning %d\n", error);
#endif
}

/*
static
mbuf_to_iov(m, iov)
	register struct mbuf *m;
	register struct iovec *iov;
{

	while (m) {
		iov->iov_base = mtod(m, caddr_t);
		iov->iov_len = m->m_len;
		iov++;
		m = m->m_next;
	}
}
*/

/*
 * Create a file.
 * Creates a file with given attributes and returns those attributes
 * and an fhandle for the new file.
 */
STATIC void
rfs_create(args, dr, exi, req)
	struct nfscreatargs *args;
	struct  nfsdiropres *dr;
	struct exportinfo *exi;
	struct svc_req *req;
{
	register int error;
	struct vattr va;
	struct vnode *vp;
	register struct vnode *dvp;

#ifdef NFSDEBUG
	printf("rfs_create: %s dfh %x %x %d\n",
	    args->ca_da.da_name, args->ca_da.da_fhandle.fh_fsid.val[0],
	    args->ca_da.da_fhandle.fh_fsid.val[1], args->ca_da.da_fhandle.fh_len);
#endif
	sattr_to_vattr(&args->ca_sa, &va);
	/*
	 * This is a completely gross hack to make mknod
	 * work over the wire until we can wack the protocol
	 */
#define IFMT		0170000		/* type of file */
#define IFCHR		0020000		/* character special */
#define IFBLK		0060000		/* block special */
#define	IFSOCK		0140000		/* socket */
	if ((va.va_mode & IFMT) == IFCHR) {
		va.va_type = VCHR;
		if (va.va_size == (u_long)NFS_FIFO_DEV)
			va.va_type = VFIFO;	/* xtra kludge for namedpipe */
		else
			va.va_rdev = (dev_t)va.va_size;
		va.va_size = 0;
		va.va_mode &= ~IFMT;
	} else if ((va.va_mode & IFMT) == IFBLK) {
		va.va_type = VBLK;
		va.va_rdev = (dev_t)va.va_size;
		va.va_size = 0;
		va.va_mode &= ~IFMT;
#ifndef	SYSV
	/*
	 *	System V doesn't believe in other file systems with other
	 *	file types.  Fix this.
	 *	XXX
	 */
	} else if ((va.va_mode & IFMT) == IFSOCK) {
		va.va_type = VSOCK;
#endif
	} else {
		va.va_type = VREG;
	}

	/*
	 * XXX - Should get exclusive flag and use it.
	 */
	dvp = fhtovp(&args->ca_da.da_fhandle, exi);
	if (dvp == NULL) {
		dr->dr_status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req)) {
		error = EROFS;
	} else {
		/*
		 * Very strange. If we are going to trunc the file
		 * (va_size == 0) we check the cache first to be sure
		 * this is not a delayed retransmission, otherwise we
		 * do the create and if it fails check the cache to see
		 * if it was a retransmission.
		 * XXX this really points out a protocol bug:
		 * The server should always do exclusive create.
		 */
		if (va.va_size == 0 && svc_clts_kdup(req)) {
			error = VOP_LOOKUP(dvp, args->ca_da.da_name, &vp,
					(struct pathname *) NULL, 0,
					(struct vnode *) 0, /* XXX - unused? */
					u.u_cred);
		} else {
			/* set the va_mask bits before create */
			va.va_mask = AT_TYPE|AT_MODE;
			if (va.va_size != -1)
				va.va_mask |= AT_SIZE;
			if (va.va_uid != -1)
				va.va_mask |= AT_UID;
			if (va.va_gid != -1)
				va.va_mask |= AT_GID;
			if (va.va_atime.tv_sec != -1)
				va.va_mask |= AT_ATIME;
			if (va.va_mtime.tv_sec != -1)
				va.va_mask |= AT_MTIME;

			error = VOP_CREATE(dvp, args->ca_da.da_name,
			    &va, NONEXCL, VWRITE, &vp, u.u_cred);
			if (error && svc_clts_kdup(req)) {
				error = VOP_LOOKUP(dvp, args->ca_da.da_name,
				    &vp, (struct pathname *) NULL, 0,
				    (struct vnode *) 0,	/* XXX - unused? */
				    u.u_cred);
			} else if (!error) {
				svc_clts_kdupsave(req);
			}
		}
	}
	if (!error) {
		va.va_mask = AT_ALL;
		error = VOP_GETATTR(vp, &va, 0, u.u_cred);
		if (!error) {
			vattr_to_nattr(&va, &dr->dr_attr);
			error = makefh(&dr->dr_fhandle, vp, exi);
		}
		VN_RELE(vp);
	}
	dr->dr_status = puterrno(error);
	VN_RELE(dvp);
#ifdef NFSDEBUG
	printf("rfs_create: returning %d\n", error);
#endif
}

/*
 * Remove a file.
 * Remove named file from parent directory.
 */
STATIC void
rfs_remove(da, status, exi, req)
	struct nfsdiropargs *da;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	register struct vnode *vp;

#ifdef NFSDEBUG
	printf("rfs_remove %s dfh %x %x %d\n",
	    da->da_name, da->da_fhandle.fh_fsid.val[0],
	    da->da_fhandle.fh_fsid.val[1], da->da_fhandle.fh_len);
#endif
	vp = fhtovp(&da->da_fhandle, exi);
	if (vp == NULL) {
		*status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req)) {
		error = EROFS;
	} else {
		error = VOP_REMOVE(vp, da->da_name, u.u_cred);
		if (error == ENOENT) {
			/*
			 * check for dup request
			 */
			if (svc_clts_kdup(req)) {
				error = 0;
			}
		} else if (!error) {
			svc_clts_kdupsave(req);
		}
	}
	*status = puterrno(error);
	VN_RELE(vp);
#ifdef NFSDEBUG
	printf("rfs_remove: %s returning %d\n",
	    da->da_name, error);
#endif
}

/*
 * rename a file
 * Give a file (from) a new name (to).
 */
STATIC void
rfs_rename(args, status, exi, req)
	struct nfsrnmargs *args;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	register struct vnode *fromvp;
	register struct vnode *tovp;

#ifdef NFSDEBUG
	printf("rfs_rename %s ffh %x %x %d -> %s tfh %x %x %d\n",
	    args->rna_from.da_name,
	    args->rna_from.da_fhandle.fh_fsid.val[0],
	    args->rna_from.da_fhandle.fh_fsid.val[1],
	    args->rna_from.da_fhandle.fh_len,
	    args->rna_to.da_name,
	    args->rna_to.da_fhandle.fh_fsid.val[0],
	    args->rna_to.da_fhandle.fh_fsid.val[1],
	    args->rna_to.da_fhandle.fh_len);
#endif
	fromvp = fhtovp(&args->rna_from.da_fhandle, exi);
	if (fromvp == NULL) {
		*status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req)) {
		error = EROFS;
		goto fromerr;
	}
	tovp = fhtovp(&args->rna_to.da_fhandle, exi);
	if (tovp == NULL) {
		*status = NFSERR_STALE;
		VN_RELE(fromvp);
		return;
	}
	error = VOP_RENAME(fromvp, args->rna_from.da_name,
		tovp, args->rna_to.da_name, u.u_cred);
	VN_RELE(tovp);
fromerr:
	VN_RELE(fromvp);
	*status = puterrno(error);
	if (error == 0) {
		svc_clts_kdupsave(req);
	} else if (svc_clts_kdup(req)) {
		*status = NFS_OK;
	}
#ifdef NFSDEBUG
	printf("rfs_rename: returning %d\n", error);
#endif
}

/*
 * Link to a file.
 * Create a file (to) which is a hard link to the given file (from).
 */
STATIC void
rfs_link(args, status, exi, req)
	struct nfslinkargs *args;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	register struct vnode *fromvp;
	register struct vnode *tovp;

#ifdef NFSDEBUG
	printf("rfs_link ffh %x %x %d -> %s tfh %x %x %d\n",
	    args->la_from.fh_fsid.val[0], args->la_from.fh_fsid.val[1],
	    args->la_from.fh_len, args->la_to.da_name,
	    args->la_to.da_fhandle.fh_fsid.val[0],
	    args->la_to.da_fhandle.fh_fsid.val[1],
	    args->la_to.da_fhandle.fh_len);
#endif
	fromvp = fhtovp(&args->la_from, exi);
	if (fromvp == NULL) {
		*status = NFSERR_STALE;
		return;
	}
	tovp = fhtovp(&args->la_to.da_fhandle, exi);
	if (tovp == NULL) {
		*status = NFSERR_STALE;
		VN_RELE(fromvp);
		return;
	}
	if (rdonly(exi, req)) {
		error = EROFS;
	} else {
		error = VOP_LINK(tovp, fromvp, args->la_to.da_name, u.u_cred);
		if (error == EEXIST) {
			/*
			 * check for dup request
			 */
			if (svc_clts_kdup(req)) {
				error = 0;
			}
		} else if (!error) {
			svc_clts_kdupsave(req);
		}
	}
	*status = puterrno(error);
	VN_RELE(fromvp);
	VN_RELE(tovp);
#ifdef NFSDEBUG
	printf("rfs_link: returning %d\n", error);
#endif
}

/*
 * Symbolicly link to a file.
 * Create a file (to) with the given attributes which is a symbolic link
 * to the given path name (to).
 */
STATIC void
rfs_symlink(args, status, exi, req)
	struct nfsslargs *args;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	struct vattr va;
	register struct vnode *vp;

#ifdef NFSDEBUG
	printf("rfs_symlink %s ffh %x %x %d -> %s\n",
	    args->sla_from.da_name,
	    args->sla_from.da_fhandle.fh_fsid.val[0],
	    args->sla_from.da_fhandle.fh_fsid.val[1],
	    args->sla_from.da_fhandle.fh_len,
	    args->sla_tnm);
#endif
	sattr_to_vattr(&args->sla_sa, &va);
	va.va_type = VLNK;
	vp = fhtovp(&args->sla_from.da_fhandle, exi);
	if (vp == NULL) {
		*status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req)) {
		error = EROFS;
	} else {
		error = VOP_SYMLINK(vp, args->sla_from.da_name,
		    &va, args->sla_tnm, u.u_cred);
		if (error == EEXIST) {
			/*
			 * check for dup request
			 */
			if (svc_clts_kdup(req)) {
				error = 0;
			}
		} else if (!error) {
			svc_clts_kdupsave(req);
		}
	}
	*status = puterrno(error);
	VN_RELE(vp);
#ifdef NFSDEBUG
	printf("rfs_symlink: returning %d\n", error);
#endif
}

/*
 * Make a directory.
 * Create a directory with the given name, parent directory, and attributes.
 * Returns a file handle and attributes for the new directory.
 */
STATIC void
rfs_mkdir(args, dr, exi, req)
	struct nfscreatargs *args;
	struct  nfsdiropres *dr;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	struct vattr va;
	struct vnode *dvp;
	register struct vnode *vp;

#ifdef NFSDEBUG
	printf("rfs_mkdir %s fh %x %x %d\n",
	    args->ca_da.da_name, args->ca_da.da_fhandle.fh_fsid.val[0],
	    args->ca_da.da_fhandle.fh_fsid.val[1], args->ca_da.da_fhandle.fh_len);
#endif
	sattr_to_vattr(&args->ca_sa, &va);
	va.va_type = VDIR;
	/*
	 * Should get exclusive flag and pass it on here
	 */
	vp = fhtovp(&args->ca_da.da_fhandle, exi);
	if (vp == NULL) {
		dr->dr_status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req)) {
		error = EROFS;
	} else {
		error = VOP_MKDIR(vp, args->ca_da.da_name, &va, &dvp, u.u_cred);
		if (error == EEXIST) {
			/*
			 * check for dup request
			 */
			if (svc_clts_kdup(req)) {
				error = VOP_LOOKUP(vp, args->ca_da.da_name,
					  &dvp,
					  (struct pathname *) NULL, 0,
					  (struct vnode *) 0,	/* XXX - unused? */
					  u.u_cred);
				if (!error) {
					va.va_mask = AT_ALL;
					error = VOP_GETATTR(vp, &va, 0, u.u_cred);
				}
			}
		}
		if (!error) {
			vattr_to_nattr(&va, &dr->dr_attr);
			error = makefh(&dr->dr_fhandle, dvp, exi);
			VN_RELE(dvp);
		}
		if (!error) {
			svc_clts_kdupsave(req);
		}
	}
	dr->dr_status = puterrno(error);
	VN_RELE(vp);
#ifdef NFSDEBUG
	printf("rfs_mkdir: returning %d\n", error);
#endif
}

/*
 * Remove a directory.
 * Remove the given directory name from the given parent directory.
 */
STATIC void
rfs_rmdir(da, status, exi, req)
	struct nfsdiropargs *da;
	enum nfsstat *status;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	register struct vnode *vp;

#ifdef NFSDEBUG
	printf("rfs_rmdir %s fh %x %x %d\n",
	    da->da_name, da->da_fhandle.fh_fsid.val[0],
	    da->da_fhandle.fh_fsid.val[1], da->da_fhandle.fh_len);
#endif
	vp = fhtovp(&da->da_fhandle, exi);
	if (vp == NULL) {
		*status = NFSERR_STALE;
		return;
	}
	if (rdonly(exi, req)) {
		error = EROFS;
	} else {
		/*
		 *	VOP_RMDIR now takes a new third argument (the current
		 *	directory of the process).  That's because someone
		 *	wants to return EINVAL if one tries to remove ".".
		 *	Of course, NFS servers have no idea what their
		 *	clients' current directories are.  We fake it by
		 *	supplying a vnode known to exist and illegal to
		 *	remove.
		 */
		error = VOP_RMDIR(vp, da->da_name, rootdir, u.u_cred);
		if (error == ENOENT) {
			/*
			 * check for dup request
			 */
			if (svc_clts_kdup(req)) {
				error = 0;
			}
		} else if (!error) {
			svc_clts_kdupsave(req);
		} else if (error == NFSERR_EXIST) {	
			/*kludge for incompatible errnos */
			error = NFSERR_NOTEMPTY;
		}
	}
	*status = puterrno(error);
	VN_RELE(vp);
#ifdef NFSDEBUG
	printf("nfs_rmdir returning %d\n", error);
#endif
}

/*ARGSUSED*/
STATIC void
rfs_readdir(rda, rd, exi, req)
	struct nfsrddirargs *rda;
	register struct nfsrddirres  *rd;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	struct iovec iov;
	struct uio uio;
	register struct vnode *vp;

#ifdef NFSDEBUG
	printf("rfs_readdir fh %x %x %d count %d\n",
	    rda->rda_fh.fh_fsid.val[0], rda->rda_fh.fh_fsid.val[1],
	    rda->rda_fh.fh_len, rda->rda_count);
#endif
	vp = fhtovp(&rda->rda_fh, exi);
	if (vp == NULL) {
		rd->rd_status = NFSERR_STALE;
		return;
	}
	VOP_RWLOCK(vp);
	/*
	 * check read access of dir.  we have to do this here because
	 * the opendir doesn't go over the wire.
	 */
	error = VOP_ACCESS(vp, VREAD, 0, u.u_cred);
	if (error) {
		goto bad;
	}

	if (rda->rda_count == 0) {
		rd->rd_size = 0;
		rd->rd_eof = FALSE;
		rd->rd_entries = NULL;
		rd->rd_bufsize = 0;
		goto bad;
	}

	rda->rda_count = MIN(rda->rda_count, NFS_MAXDATA);

	/*
	 * Allocate data for entries.  This will be freed by rfs_rdfree.
	 */
	rd->rd_entries = (struct dirent *)kmem_alloc((u_int)rda->rda_count, KM_SLEEP);
	rd->rd_bufsize = rda->rda_count;

	/*
	 * Set up io vector to read directory data
	 */
	iov.iov_base = (caddr_t)rd->rd_entries;
	iov.iov_len = rda->rda_count;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_offset = rda->rda_offset;
	uio.uio_resid = rda->rda_count;

	/*
	 * read directory
	 */
	error = VOP_READDIR(vp, &uio, u.u_cred, (int *)NULL);

	/*
	 * Clean up
	 */
	if (error) {
		rd->rd_size = 0;
		goto bad;
	}

	/*
	 * set size and eof
	 */
	if (rda->rda_count && uio.uio_resid == rda->rda_count) {
		rd->rd_size = 0;
		rd->rd_eof = TRUE;
	} else {
		rd->rd_size = rda->rda_count - uio.uio_resid;
		rd->rd_eof = FALSE;
	}

bad:
	rd->rd_status = puterrno(error);
	VOP_RWUNLOCK(vp);
	VN_RELE(vp);
#ifdef NFSDEBUG
	printf("rfs_readdir: returning %d\n", error);
#endif
}

STATIC void
rfs_rddirfree(rd)
	struct nfsrddirres *rd;
{

	kmem_free((caddr_t)rd->rd_entries, (u_int)rd->rd_bufsize);
}

/*ARGSUSED*/
STATIC void
rfs_statfs(fh, fs, exi, req)
	fhandle_t *fh;
	register struct nfsstatfs *fs;
	struct exportinfo *exi;
	struct svc_req *req;
{
	int error;
	struct statvfs sb;
	register struct vnode *vp;

#ifdef NFSDEBUG
	printf("rfs_statfs fh %x %x %d\n",
	    fh->fh_fsid.val[0], fh->fh_fsid.val[1], fh->fh_len);
#endif
	vp = fhtovp(fh, exi);
	if (vp == NULL) {
		fs->fs_status = NFSERR_STALE;
		return;
	}
	error = VFS_STATVFS(vp->v_vfsp, &sb);
	fs->fs_status = puterrno(error);
	if (!error) {
		fs->fs_tsize = nfstsize();
		fs->fs_bsize = sb.f_bsize;
		fs->fs_blocks = sb.f_blocks;
		fs->fs_bfree = sb.f_bfree;
		fs->fs_bavail = sb.f_bavail;
	}
	VN_RELE(vp);
#ifdef NFSDEBUG
	printf("rfs_statfs returning %d\n", error);
#endif
}

/*ARGSUSED*/
STATIC void
rfs_null(argp, resp)
	caddr_t *argp;
	caddr_t *resp;
{
	/* do nothing */
	/* return (0); */
}

/*ARGSUSED*/
STATIC void
rfs_error(argp, resp)
	caddr_t *argp;
	caddr_t *resp;
{
	/* return (EOPNOTSUPP); */
}

STATIC void
nullfree()
{
}

#ifdef RFSCALLTRACE
char rfscallnames[][20] = {
	"RFS_NULL",	"RFS_GETATTR",	"RFS_SETATTR",	"RFS_ROOT",
	"RFS_LOOKUP",	"RFS_READLINK",	"RFS_READ",	"RFS_WRITECACHE",
	"RFS_WRITE",	"RFS_CREATE",	"REMOVE",	"RFS_RENAME",
	"RFS_LINK",	"RFS_SYMLINK",	"RFS_MKDIR",	"RFS_RMDIR",
	"RFS_READDIR",	"RFS_STATFS"
};
#endif

/*
 * rfs dispatch table
 * Indexed by version, proc
 */

struct rfsdisp {
	void	  (*dis_proc)();	/* proc to call */
	xdrproc_t dis_xdrargs;		/* xdr routine to get args */
	int	  dis_argsz;		/* sizeof args */
	xdrproc_t dis_xdrres;		/* xdr routine to put results */
	int	  dis_ressz;		/* size of results */
	void	  (*dis_resfree)();	/* frees space allocated by proc */
} rfsdisptab[][RFS_NPROC]  = {
	{
	/*
	 * VERSION 2
	 * Changed rddirres to have eof at end instead of beginning
	 */
	/* RFS_NULL = 0 */
	{rfs_null, xdr_void, 0,
	    xdr_void, 0, nullfree},
	/* RFS_GETATTR = 1 */
	{rfs_getattr, xdr_fhandle, sizeof (fhandle_t),
	    xdr_attrstat, sizeof (struct nfsattrstat), nullfree},
	/* RFS_SETATTR = 2 */
	{rfs_setattr, xdr_saargs, sizeof (struct nfssaargs),
	    xdr_attrstat, sizeof (struct nfsattrstat), nullfree},
	/* RFS_ROOT = 3 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0,
	    xdr_void, 0, nullfree},
	/* RFS_LOOKUP = 4 */
	{rfs_lookup, xdr_diropargs, sizeof (struct nfsdiropargs),
	    xdr_diropres, sizeof (struct nfsdiropres), nullfree},
	/* RFS_READLINK = 5 */
	{rfs_readlink, xdr_fhandle, sizeof (fhandle_t),
	    xdr_rdlnres, sizeof (struct nfsrdlnres), rfs_rlfree},
	/* RFS_READ = 6 */
	{rfs_read, xdr_readargs, sizeof (struct nfsreadargs),
	    xdr_rdresult, sizeof (struct nfsrdresult), rfs_rdfree},
	/* RFS_WRITECACHE = 7 *** NO LONGER SUPPORTED *** */
	{rfs_error, xdr_void, 0,
	    xdr_void, 0, nullfree},
	/* RFS_WRITE = 8 */
	{rfs_write, xdr_writeargs, sizeof (struct nfswriteargs),
	    xdr_attrstat, sizeof (struct nfsattrstat), nullfree},
	/* RFS_CREATE = 9 */
	{rfs_create, xdr_creatargs, sizeof (struct nfscreatargs),
	    xdr_diropres, sizeof (struct nfsdiropres), nullfree},
	/* RFS_REMOVE = 10 */
	{rfs_remove, xdr_diropargs, sizeof (struct nfsdiropargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree},
	/* RFS_RENAME = 11 */
	{rfs_rename, xdr_rnmargs, sizeof (struct nfsrnmargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree},
	/* RFS_LINK = 12 */
	{rfs_link, xdr_linkargs, sizeof (struct nfslinkargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree},
	/* RFS_SYMLINK = 13 */
	{rfs_symlink, xdr_slargs, sizeof (struct nfsslargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree},
	/* RFS_MKDIR = 14 */
	{rfs_mkdir, xdr_creatargs, sizeof (struct nfscreatargs),
	    xdr_diropres, sizeof (struct nfsdiropres), nullfree},
	/* RFS_RMDIR = 15 */
	{rfs_rmdir, xdr_diropargs, sizeof (struct nfsdiropargs),
	    xdr_enum, sizeof (enum nfsstat), nullfree},
	/* RFS_READDIR = 16 */
	{rfs_readdir, xdr_rddirargs, sizeof (struct nfsrddirargs),
	    xdr_putrddirres, sizeof (struct nfsrddirres), rfs_rddirfree},
	/* RFS_STATFS = 17 */
	{rfs_statfs, xdr_fhandle, sizeof (fhandle_t),
	    xdr_statfs, sizeof (struct nfsstatfs), nullfree},
	}
};

struct rfsspace {
	struct rfsspace *rs_next;
	caddr_t		rs_dummy;
};

struct rfsspace *rfsfreesp = NULL;

int rfssize = 0;

caddr_t
rfsget()
{
	int i;
	struct rfsdisp *dis;
	caddr_t ret;

	if (rfssize == 0) {
		for (i = 0; i < 1 + VERSIONMAX - VERSIONMIN; i++) {
			for (dis = &rfsdisptab[i][0];
			    dis < &rfsdisptab[i][RFS_NPROC];
			    dis++) {
				rfssize = MAX(rfssize, dis->dis_argsz);
				rfssize = MAX(rfssize, dis->dis_ressz);
			}
		}
	}

	if (rfsfreesp) {
		ret = (caddr_t)rfsfreesp;
		rfsfreesp = rfsfreesp->rs_next;
	} else {
		ret = kmem_alloc((u_int)rfssize, KM_SLEEP);
	}
	return (ret);
}

STATIC void
rfsput(rs)
	struct rfsspace *rs;
{

	rs->rs_next = rfsfreesp;
	rfsfreesp = rs;
}


/*
 * If nfs_portmon is set, then clients are required to use
 * privileged ports (ports < IPPORT_RESERVED) in order to get NFS services.
 */
int nfs_portmon = 0;


void
rfs_dispatch(req, xprt)
	struct svc_req *req;
	register SVCXPRT *xprt;
{
	int which;
	int vers;
	caddr_t	args = NULL;
	caddr_t	res = NULL;
	register struct rfsdisp *disp;
	struct cred *tmpcr;
	struct cred *newcr = NULL;
	int error;
	struct exportinfo *exi;

	svstat.ncalls++;
	error = 0;
	which = req->rq_proc;
	if (which < 0 || which >= RFS_NPROC) {
#ifdef NFSDEBUG
		printf("rfs_dispatch: bad proc %d\n", which);
#endif
		svcerr_noproc(req->rq_xprt);
		error++;
		printf("nfs_server: bad proc number\n");
		goto done;
	}
	vers = req->rq_vers;
	if (vers < VERSIONMIN || vers > VERSIONMAX) {
#ifdef NFSDEBUG
		printf("rfs_dispatch: bad vers %d low %d high %d\n",
		    vers, VERSIONMIN, VERSIONMAX);
#endif
		svcerr_progvers(req->rq_xprt, (u_long)VERSIONMIN,
		    (u_long)VERSIONMAX);
		error++;
		printf("nfs_server: bad version number\n");
		goto done;
	}
	vers -= VERSIONMIN;
	disp = &rfsdisptab[vers][which];

	/*
	 * Clean up as if a system call just started
	 */
	u.u_error = 0;

	/*
	 * Allocate args struct and deserialize into it.
	 */
	args = rfsget();
	bzero(args, (u_int)rfssize);
	if ( ! SVC_GETARGS(xprt, disp->dis_xdrargs, args)) {
		svcerr_decode(xprt);
		error++;
		printf("nfs_server: bad getargs\n");
		goto done;
	}

	/*
	 * Find export information and check authentication,
	 * setting the credential if everything is ok.
	 */
	if (which != RFS_NULL) {
		/*
		 * XXX: this isn't really quite correct. Instead of doing
		 * this blind cast, we should extract out the fhandle for
		 * each NFS call. What's more, some procedures (like rename)
		 * have more than one fhandle passed in, and we should check
		 * that the two fhandles point to the same exported path.
		 */
		/* LINTED pointer alignment */
		fhandle_t *fh = (fhandle_t *) args;

		newcr = crget();
		tmpcr = u.u_cred;
		u.u_cred = newcr;
		/* LINTED pointer alignment */
		exi = findexport(&fh->fh_fsid, (struct fid *) &fh->fh_xlen);
		if (exi != NULL && !checkauth(exi, req, newcr)) {
			svcerr_weakauth(xprt);
			error++;
			printf("nfs_server: weak authentication\n");
			goto done;
		}
	}

	/*
	 * Allocate results struct.
	 */
	res = rfsget();
	bzero(res, (u_int)rfssize);

	svstat.reqs[which]++;

#ifdef RFSCALLTRACE
	cmn_err (CE_CONT, "rfs_dispatch: %s", rfscallnames[which]);
	if (which == RFS_WRITE)
		cmn_err (CE_CONT, ": offset %x, count %x\n",
			((struct nfswriteargs *) args)->wa_offset,
			((struct nfswriteargs *) args)->wa_count);
	else
		cmn_err (CE_CONT, "\n");
#endif
	/*
	 * Call service routine with arg struct and results struct
	 */
	(*disp->dis_proc)(args, res, exi, req);

done:
	/*
	 * Free arguments struct
	 */
	if (!SVC_FREEARGS(xprt, disp->dis_xdrargs, args) ) {
		printf("nfs_server: bad freeargs\n");
		error++;
	}
	if (args != NULL) {
		/* LINTED pointer alignment */
		rfsput((struct rfsspace *)args);
	}

	/*
	 * Serialize and send results struct
	 */
	if (!error) {
		if (!svc_sendreply(xprt, disp->dis_xdrres, res)) {
			printf("nfs_server: bad sendreply\n");
			error++;
		}
	}

	/*
	 * Free results struct
	 */
	if (res != NULL) {
		if ( disp->dis_resfree != nullfree ) {
			(*disp->dis_resfree)(res);
		}
		/* LINTED pointer alignment */
		rfsput((struct rfsspace *)res);
	}
	/*
	 * restore original credentials
	 */
	if (newcr) {
		u.u_cred = tmpcr;
		crfree(newcr);
	}
	svstat.nbadcalls += error;
}

void
sattr_to_vattr(sa, vap)
	register struct nfssattr *sa;
	register struct vattr *vap;
{
	vap->va_mask = 0;
	vap->va_mode = sa->sa_mode;
	vap->va_uid = sa->sa_uid;
	vap->va_gid = sa->sa_gid;
	vap->va_size = sa->sa_size;
	vap->va_atime.tv_sec  = sa->sa_atime.tv_sec;
	vap->va_atime.tv_nsec = sa->sa_atime.tv_usec*1000;
	vap->va_mtime.tv_sec  = sa->sa_mtime.tv_sec;
	vap->va_mtime.tv_nsec = sa->sa_mtime.tv_usec*1000;
}

/*
 * Convert an fhandle into a vnode.
 * Uses the file id (fh_len + fh_data) in the fhandle to get the vnode.
 * WARNING: users of this routine must do a VN_RELE on the vnode when they
 * are done with it.
 */
struct vnode *
fhtovp(fh, exi)
	fhandle_t *fh;
	struct exportinfo *exi;
{
	register struct vfs *vfsp;
	struct vnode *vp;
	int error;

	if (exi == NULL) {
		return (NULL);	/* not exported */
	}
	vfsp = getvfs(&fh->fh_fsid);
	if (vfsp == NULL) {
		return (NULL);
	}
	/* LINTED pointer alignment */
	error = VFS_VGET(vfsp, &vp, (struct fid *)&(fh->fh_len));
	if (error || vp == NULL) {
#ifdef NFSDEBUG
		printf("fhtovp(%x) couldn't vget\n", fh);
#endif
		return (NULL);
	}
	return (vp);
}

/*
 * Determine if two addresses are equal
 * Only AF_INET supported for now
 */
eqaddr(addr1, addr2)
#ifdef	SYSV
	struct netbuf *addr1;
	struct netbuf *addr2;
#else
	struct sockaddr *addr1;
	struct sockaddr *addr2;
#endif
{
#ifdef	SYSV
	return((addr1->len == addr2->len) && !bcmp(addr1->buf, addr2->buf, addr1->len));
#else
	if (addr1->sa_family != addr2->sa_family) {
		return (0);
	}
	switch (addr1->sa_family) {
	case AF_INET:
		return (((struct taddr_in *) addr1)->sin_addr.s_addr ==
			((struct taddr_in *) addr2)->sin_addr.s_addr);
	case AF_NS:
		/* coming soon? */
		break;
	}
	return (0);
#endif
}

hostinlist(sa, addrs)
#ifdef	SYSV
	struct netbuf *sa;
#else
	struct sockaddr *sa;
#endif
	struct exaddrlist *addrs;
{
	int i;

	for (i = 0; i < addrs->naddrs; i++) {
		if (eqaddr(sa, &addrs->addrvec[i])) {
			return (1);
		}
	}
	return (0);
}

/*
 * Check to see if the given name corresponds to a
 * root user of the exported filesystem.
 */
rootname(ex, netname)
	struct export *ex;
	char *netname;
{
	int i;
	int namelen;

	namelen = strlen(netname) + 1;
	for (i = 0; i < ex->ex_des.nnames; i++) {
		if (bcmp(netname, ex->ex_des.rootnames[i], namelen) == 0) {
			return (1);
		}
	}
	return (0);
}

checkauth(exi, req, cred)
	struct exportinfo *exi;
	struct svc_req *req;
	struct cred *cred;
{
	struct authunix_parms *aup;
	struct authdes_cred *adc;
	int flavor;
	short grouplen;

#ifdef	SYSV
	/*
	 *      Check for privileged port number
	 *      N.B.:  this assumes that we know the format of a netbuf.
	 */
	if (nfs_portmon) {
		 /* LINTED pointer alignment */
		 struct sockaddr *ca = (struct sockaddr *) svc_getrpccaller(req->rq_xprt)->buf;

		 if ((ca->sa_family == AF_INET) &&
		     (ntohs(((struct sockaddr_in *) ca)->sin_port) >= IPPORT_RESERVED)) {
			printf("NFS request from unprivileged port.\n");
			return (0);
		}
	}
#else
	/*
	 * Check for privileged port number
	 */
	if (nfs_portmon &&
	    ntohs(req->rq_xprt->xp_raddr.sin_port) >= IPPORT_RESERVED) {
		printf("NFS request from unprivileged port.\n");
		return (0);
	}
#endif

	/*
	 * Set uid, gid, and gids to auth params
	 */
	flavor = req->rq_cred.oa_flavor;
	if (flavor != exi->exi_export.ex_auth) {
		flavor = AUTH_NULL;
	}
	switch (flavor) {
	case AUTH_NULL:
		cred->cr_uid = exi->exi_export.ex_anon;
		cred->cr_gid = exi->exi_export.ex_anon;
		break;

	case AUTH_UNIX:
		/* LINTED pointer alignment */
		aup = (struct authunix_parms *)req->rq_clntcred;
#ifdef	SYSV
		if (aup->aup_uid == 0 &&
		    !hostinlist(svc_getrpccaller(req->rq_xprt), &exi->exi_export.ex_unix.rootaddrs)) {
			cred->cr_uid = exi->exi_export.ex_anon;
			cred->cr_gid = exi->exi_export.ex_anon;
			cred->cr_ngroups = 0;
		} else {
			cred->cr_uid = aup->aup_uid;
			cred->cr_gid = aup->aup_gid;
			bcopy((caddr_t)aup->aup_gids, (caddr_t)cred->cr_groups,
			    aup->aup_len * sizeof (cred->cr_groups[0]));
			cred->cr_ngroups = aup->aup_len;
		}
#else
		if (aup->aup_uid == 0 &&
		    !hostinlist((struct sockaddr *)svc_getcaller(req->rq_xprt),
				&exi->exi_export.ex_unix.rootaddrs)) {
			cred->cr_uid = exi->exi_export.ex_anon;
			cred->cr_gid = exi->exi_export.ex_anon;
			cred->cr_ngroups = 0;
		} else {
			cred->cr_uid = aup->aup_uid;
			cred->cr_gid = aup->aup_gid;
			bcopy((caddr_t)aup->aup_gids, (caddr_t)cred->cr_groups,
			    aup->aup_len * sizeof (cred->cr_groups[0]));
			cred->cr_ngroups = aup->aup_len;
		}
#endif
		break;


	case AUTH_DES:
		/* LINTED pointer alignment */
		adc = (struct authdes_cred *)req->rq_clntcred;
		if (adc->adc_fullname.window > exi->exi_export.ex_des.window) {
			return (0);
		}
		if (!authdes_getucred(adc, &cred->cr_uid, &cred->cr_gid,
		    &grouplen, cred->cr_groups)) {
			if (rootname(&exi->exi_export,
			    adc->adc_fullname.name)) {
				cred->cr_uid = 0;
			} else {
				cred->cr_uid = exi->exi_export.ex_anon;
			}
			cred->cr_gid = exi->exi_export.ex_anon;
			grouplen = 0;
		}
		cred->cr_ngroups = grouplen;
		break;

	default:
		return (0);
	}
	return (cred->cr_uid != (uid_t) -1);
}
