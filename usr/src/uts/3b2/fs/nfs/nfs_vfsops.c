/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/nfs/nfs_vfsops.c	1.10"

/*	@(#)nfs_vfsops.c 2.91 88/10/17 SMI 	*/

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

#define NFSCLIENT

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/cred.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/pathname.h>
#include <sys/uio.h>
#include <sys/socket.h>
/* #include <sys/socketvar.h> */
#include <sys/tiuser.h>
#include <sys/sysmacros.h>
#include <sys/kmem.h>
#include <netinet/in.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/pmap_prot.h>
#include <nfs/nfs.h>
#include <nfs/nfs_clnt.h>
#include <nfs/rnode.h>
#include <nfs/mount.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/statvfs.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/cmn_err.h>

#define	satosin(sa)	((struct sockaddr_in *)(sa))

#ifdef NFSDEBUG
extern int nfsdebug;
#endif

STATIC int nfsrootvp();
extern struct vnode *makenfsnode();
#define MAPSIZE  256/NBBY
static char nfs_minmap[MAPSIZE]; /* Map for minor device allocation */

/*
 * nfs vfs operations.
 */
STATIC	int nfs_mount();
STATIC	int nfs_unmount();
STATIC	int nfs_root();
STATIC	int nfs_statvfs();
STATIC	int nfs_sync();
STATIC	int nfs_vget();
STATIC	int nfs_mountroot();
STATIC	int nfs_swapvp();
STATIC	int nfs_nosys();

struct vfsops nfs_vfsops = {
	nfs_mount,
	nfs_unmount,
	nfs_root,
	nfs_statvfs,
	nfs_sync,
	nfs_vget,
	nfs_mountroot,
	nfs_swapvp,
	nfs_nosys,	/* filler */
	nfs_nosys,	/*   "    */
	nfs_nosys,	/*   "    */
	nfs_nosys	/*   "    */
};

/*
 * Initialize the vfs structure
 */

int nfsfstyp;

void
nfsinit(vswp, fstyp)
	struct vfssw *vswp;
	int fstyp;
{
	vswp->vsw_vfsops = &nfs_vfsops;
	nfsfstyp = fstyp;
}

/*
 * nfs mount vfsop
 * Set up mount info record and attach it to vfs struct.
 */
/*ARGSUSED*/
STATIC int
/* nfs_mount(vfsp, special, path, flags, data, datalen, cred) */
nfs_mount(vfsp, mvp, uap, cred)
	struct vfs *vfsp;
	struct vnode *mvp;
	struct mounta *uap;
	struct cred *cred;
{
	/* char *path = uap->dir; */
	/* int flags = uap->flags; */
	char *data = uap->dataptr;
	int datalen = uap->datalen;
	int error;
	struct vnode *rtvp = NULL;	/* the server's root */
	struct mntinfo *mi;		/* mount info, pointed at by vfs */
	fhandle_t fh;			/* root fhandle */
	struct sockaddr_in saddr;	/* server's address */
	char shostname[HOSTNAMESZ];	/* server's hostname */
	int hlen;			/* length of hostname */
	char netname[MAXNETNAMELEN+1];	/* server's netname */
	int nlen;			/* length of netname */
	struct nfs_args args;		/* nfs mount arguments */
	struct netbuf addr;		/* TLI-converted saddr */
	/* int fd; */			/* bound TLI fd */

	/*
	 * For now, ignore remount option.
	 */
	if (vfsp->vfs_flag & VFS_REMOUNT) {
		return (0);
	}

	if (mvp->v_type != VDIR)
		return ENOTDIR;

	/*
	 *	XXX - why is this not a generic function?
	 *  XXX - it is now.
	 */
	/*
	 *	if (flags & MS_RDONLY)
	 *		vfsp->vfs_flag |= VFS_RDONLY;
	 */

	/*
	 * get arguments
	 */
	if (datalen != sizeof(args))
		error = EINVAL;
	else
		error = copyin(data, (caddr_t)&args, sizeof (args));
	if (error) {
		goto errout;
	}

	/*
	 * Get server address
	 */
	error = copyin((caddr_t)args.addr, (caddr_t)&saddr,
	    sizeof (saddr));
	if (error) {
		goto errout;
	}
	/*
	 * For now we just support AF_INET
	 */
	if (saddr.sin_family != AF_INET) {
		error = EPFNOSUPPORT;
		goto errout;
	}

	/*
	 * Get the root fhandle
	 */
	error = copyin((caddr_t)args.fh, (caddr_t)&fh, sizeof (fh));
	if (error) {
		goto errout;
	}

	/*
	 * Get server's hostname
	 */
	if (args.flags & NFSMNT_HOSTNAME) {
		error = copyinstr(args.hostname, shostname,
			sizeof (shostname), (u_int *)&hlen);
		if (error) {
			goto errout;
		}
	} else
		(void) strncpy(shostname, "unknown-host", sizeof(shostname));
		

	/*
	 * Get server's netname
	 */
	if (args.flags & NFSMNT_SECURE) {
		error = copyinstr(args.netname, netname, sizeof (netname),
			(u_int *)&nlen);
	} else {
		nlen = -1;
	}

	/*
	 * Convert saddr to a netbuf
	 */
	if ((addr.buf = (char *) kmem_alloc(sizeof(saddr), KM_SLEEP)) == (char *) NULL) {
		error = ENOMEM;
		goto errout;
	}
	addr.maxlen = addr.len = sizeof(saddr);
	bcopy((caddr_t) &saddr, addr.buf, sizeof(saddr));

	/*
	 * Get root vnode.
	 */
	error = nfsrootvp(&rtvp, vfsp, args.tlidev, &addr, &fh, shostname,
		netname, nlen, args.flags);
	if (error)
		return (error);

	/*
	 * Set option fields in mount info record
	 */
	/* LINTED pointer alignment */
	mi = vtomi(rtvp);
	mi->mi_noac = ((args.flags & NFSMNT_NOAC) != 0);
	mi->mi_nocto = ((args.flags & NFSMNT_NOCTO) != 0);
	if (args.flags & NFSMNT_RETRANS) {
		mi->mi_retrans = args.retrans;
		if (args.retrans < 0) {
			error = EINVAL;
			goto errout;
		}
	}
	if (args.flags & NFSMNT_TIMEO) {
		/*
		 * With dynamic retransmission, the mi_timeo is used only
		 * as a hint for the first one. The deviation is stored in
		 * units of hz shifted left by two, or 5msec. Since timeo
		 * was in units of 100msec, multiply by 20 to convert.
		 * rtxcur is in unscaled ticks, so multiply by 5.
		 */
		mi->mi_timeo = args.timeo;
		mi->mi_timers[3].rt_deviate = (args.timeo*HZ*2)/5;
		mi->mi_timers[3].rt_rtxcur = args.timeo*HZ/10;
		if (args.timeo <= 0) {
			error = EINVAL;
			goto errout;
		}
	}
	if (args.flags & NFSMNT_GRPID) {
		mi->mi_grpid = 1;
	}
	if (args.flags & NFSMNT_RSIZE) {
		if (args.rsize <= 0) {
			error = EINVAL;
			goto errout;
		}
		mi->mi_tsize = MIN(mi->mi_tsize, args.rsize);
		mi->mi_curread = mi->mi_tsize;
	}
	if (args.flags & NFSMNT_WSIZE) {
		if (args.wsize <= 0) {
			error = EINVAL;
			goto errout;
		}
		mi->mi_stsize = MIN(mi->mi_stsize, args.wsize);
		mi->mi_curwrite = mi->mi_stsize;
	}
	if (args.flags & NFSMNT_ACREGMIN) {
		if (args.acregmin < 0) {
			mi->mi_acregmin = ACMINMAX;
		} else if (args.acregmin == 0) {
			error = EINVAL;
			printf("nfs_mount: acregmin == 0\n");
			goto errout;
		} else {
			mi->mi_acregmin = MIN(args.acregmin, ACMINMAX);
		}
	}
	if (args.flags & NFSMNT_ACREGMAX) {
		if (args.acregmax < 0) {
			mi->mi_acregmax = ACMAXMAX;
		} else if (args.acregmax < mi->mi_acregmin) {
			error = EINVAL;
			printf("nfs_mount: acregmax < acregmin\n");
			goto errout;
		} else {
			mi->mi_acregmax = MIN(args.acregmax, ACMAXMAX);
		}
	}
	if (args.flags & NFSMNT_ACDIRMIN) {
		if (args.acdirmin < 0) {
			mi->mi_acdirmin = ACMINMAX;
		} else if (args.acdirmin == 0) {
			error = EINVAL;
			printf("nfs_mount: acdirmin == 0\n");
			goto errout;
		} else {
			mi->mi_acdirmin = MIN(args.acdirmin, ACMINMAX);
		}
	}
	if (args.flags & NFSMNT_ACDIRMAX) {
		if (args.acdirmax < 0) {
			mi->mi_acdirmax = ACMAXMAX;
		} else if (args.acdirmax < mi->mi_acdirmin) {
			error = EINVAL;
			printf("nfs_mount: acdirmax < acdirmin\n");
			goto errout;
		} else {
			mi->mi_acdirmax = MIN(args.acdirmax, ACMAXMAX);
		}
	}
	mi->mi_authflavor =
		(args.flags & NFSMNT_SECURE) ? AUTH_DES : AUTH_UNIX;

#ifdef NFSDEBUG
	printf("nfs_mount: hard %d timeo %d retries %d wsize %d rsize %d\n",
	    mi->mi_hard, mi->mi_timeo, mi->mi_retrans, mi->mi_stsize,
	    mi->mi_tsize);
	printf("           regmin %d regmax %d dirmin %d dirmax %d\n",
	    mi->mi_acregmin, mi->mi_acregmax, mi->mi_acdirmin, mi->mi_acdirmax);
#endif

errout:
	if (error) {
		if (rtvp) {
			VN_RELE(rtvp);
		}
	}
	return (error);
}

STATIC int
nfsrootvp(rtvpp, vfsp, dev, addr, fh, shostname, netname, nlen, flags)
	struct vnode **rtvpp;		/* where to return root vp */
	register struct vfs *vfsp;	/* vfs of fs, if NULL make one */
	dev_t dev;			/* TLI device number */
	struct netbuf *addr;		/* server address */
	fhandle_t *fh;			/* swap file fhandle */
	char *shostname;		/* server's hostname */
	char *netname;			/* server's netname */
	int nlen;			/* length of netname, -1 if none */
	int flags;			/* mount flags */
{
	register struct vnode *rtvp = NULL;	/* the server's root */
	register struct mntinfo *mi = NULL;	/* mount info, pointed at by vfs */
	struct vattr va;		/* root vnode attributes */
	struct nfsfattr na;		/* root vnode attributes in nfs form */
	struct statvfs sb;		/* server's file system stats */
	register int error;
	/* struct ifaddr *ifa; */
	/* struct knetconfig *kp; */

	/*
	 * Create a mount record and link it to the vfs struct.
	 */
	mi = (struct mntinfo *)kmem_zalloc(sizeof (*mi), KM_SLEEP);
	mi->mi_hard = ((flags & NFSMNT_SOFT) == 0);
	mi->mi_int = ((flags & NFSMNT_INT) != 0);
	mi->mi_addr = *addr;
	mi->mi_knetconfig = (struct knetconfig *) kmem_alloc(sizeof (struct knetconfig), KM_SLEEP);
	mi->mi_knetconfig->nc_rdev = dev;
	/*
	 *	XXX
	 *	Horrible kludge that shouldn't be here - pass in entire
	 *	knetconfig structure instead.
	 */
	mi->mi_knetconfig->nc_protofmly = AF_INET;
	mi->mi_retrans = NFS_RETRIES;
	mi->mi_timeo = NFS_TIMEO;
	mi->mi_mntno = vfs_getnum(nfs_minmap, MAPSIZE);
	bcopy(shostname, mi->mi_hostname, HOSTNAMESZ);
	mi->mi_acregmin = ACREGMIN;
	mi->mi_acregmax = ACREGMAX;
	mi->mi_acdirmin = ACDIRMIN;
	mi->mi_acdirmax = ACDIRMAX;
	mi->mi_netnamelen = nlen;
	if (nlen >= 0) {
		mi->mi_netname = (char *)kmem_alloc((u_int)nlen, KM_SLEEP);
		bcopy(netname, mi->mi_netname, (u_int)nlen);
	}

#ifdef	notdef
	/*
	 * Use heuristic to turn off transfer size adjustment
	 */
	ifa = ifa_ifwithdstaddr((struct sockaddr *)sin);
	if (ifa == (struct ifaddr *)0)
		ifa = ifa_ifwithnet((struct sockaddr *)sin);
	mi->mi_dynamic = (ifa == (struct ifaddr *)0 ||
	        ifa->ifa_ifp == (struct ifnet *)0 ||
		ifa->ifa_ifp->if_mtu < ETHERMTU );
#else /* notdef */
	mi->mi_dynamic = 0;
#endif	/* notdef */
	/*
	 * Make a vfs struct for nfs.  We do this here instead of below
	 * because rtvp needs a vfs before we can do a getattr on it.
	 */
	vfsp->vfs_fsid.val[0] = mi->mi_mntno;
	vfsp->vfs_fsid.val[1] = nfsfstyp;
	vfsp->vfs_data = (caddr_t)mi;
	vfsp->vfs_fstype = nfsfstyp;

	/*
	 * Make the root vnode, use it to get attributes,
	 * then remake it with the attributes.
	 */
	rtvp = makenfsnode(fh, (struct nfsfattr *)0, vfsp);
	if ((rtvp->v_flag & VROOT) != 0) {
		error = EINVAL;
		goto bad;
	}
	rtvp->v_flag |= VROOT;
	error = VOP_GETATTR(rtvp, &va, 0, u.u_cred);
	if (error)
		goto bad;
	VN_RELE(rtvp);
	vattr_to_nattr(&va, &na);
	rtvp = makenfsnode(fh, &na, vfsp);
	rtvp->v_flag |= VROOT;
	mi->mi_rootvp = rtvp;

	/*
	 * Get server's filesystem stats.  Use these to set transfer
	 * sizes, filesystem block size, and read-only.
	 */
	error = VFS_STATVFS(vfsp, &sb);
	if (error)
		goto bad;
	mi->mi_tsize = min(NFS_MAXDATA, (u_int)nfstsize());
	mi->mi_curread = mi->mi_tsize;

	/*
	 * Set filesystem block size to maximum data transfer size
	 */
	mi->mi_bsize = NFS_MAXDATA;
	vfsp->vfs_bsize = mi->mi_bsize;

	/*
	 * Need credentials in the rtvp so do_bio can find them.
	 */
	crhold(u.u_cred);
	/* LINTED pointer alignment */
	vtor(rtvp)->r_cred = u.u_cred;

	*rtvpp = rtvp;
	return (0);
bad:
	if (mi) {
		if (mi->mi_netnamelen >= 0) {
			kmem_free((caddr_t)mi->mi_netname, (u_int)mi->mi_netnamelen);
		}
		kmem_free((caddr_t)mi, sizeof (*mi));
	}
	if (rtvp) {
		VN_RELE(rtvp);
	}
	*rtvpp = NULL;
	return (error);
}

/*
 * vfs operations
 */
STATIC int
nfs_unmount(vfsp, cr)
	struct vfs *vfsp;
	struct cred *cr;
{
	/* LINTED pointer alignment */
	struct mntinfo *mi = (struct mntinfo *)vfsp->vfs_data;

	if (!suser(cr))
		return(EPERM);

#ifdef NFSDEBUG
	printf("nfs_unmount(%x) mi = %x\n", vfsp, mi);
#endif
	rflush(vfsp);
	rinval(vfsp);

	if (mi->mi_refct != 1 || mi->mi_rootvp->v_count != 1) {
		return (EBUSY);
	}
	VN_RELE(mi->mi_rootvp);
	/* LINTED pointer alignment */
	rp_rmhash(vtor(mi->mi_rootvp));
	/* LINTED pointer alignment */
	rinactive(vtor(mi->mi_rootvp));
	vfs_putnum(nfs_minmap, mi->mi_mntno);
	if (mi->mi_netnamelen >= 0) {
		kmem_free((caddr_t)mi->mi_netname, (u_int)mi->mi_netnamelen);
	}
	kmem_free((caddr_t) mi->mi_addr.buf, mi->mi_addr.len);
	kmem_free((caddr_t) mi->mi_knetconfig, sizeof(struct knetconfig));
	kmem_free((caddr_t)mi, sizeof (*mi));
	return (0);
}

/*
 * find root of nfs
 */
STATIC int
nfs_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{

	/* LINTED pointer alignment */
	*vpp = ((struct mntinfo *)vfsp->vfs_data)->mi_rootvp;
	(*vpp)->v_count++;
#ifdef NFSDEBUG
	printf("nfs_root(0x%x) = %x\n", vfsp, *vpp);
#endif
	return (0);
}

/*
 * Get file system statistics.
 */
STATIC int
nfs_statvfs(vfsp, sbp)
	register struct vfs *vfsp;
	struct statvfs *sbp;
{
	struct nfsstatfs fs;
	struct mntinfo *mi;
	fhandle_t *fh;
	int error = 0;

	/* LINTED pointer alignment */
	mi = vftomi(vfsp);
	/* LINTED pointer alignment */
	fh = vtofh(mi->mi_rootvp);
#ifdef NFSDEBUG
	printf("nfs_statvfs vfs %x\n", vfsp);
#endif
	error = rfscall(mi, RFS_STATFS, xdr_fhandle,
	    (caddr_t)fh, xdr_statfs, (caddr_t)&fs, u.u_cred);
	if (!error) {
		error = geterrno(fs.fs_status);
	}
	if (!error) {
		if (mi->mi_stsize) {
			mi->mi_stsize = MIN(mi->mi_stsize, fs.fs_tsize);
		} else {
			mi->mi_stsize = fs.fs_tsize;
			mi->mi_curwrite = mi->mi_stsize;
		}
		sbp->f_bsize = fs.fs_bsize;
		/* sbp->f_frsize = 0; */	/* XXX - does 0 mean unsupported? */
		sbp->f_frsize = fs.fs_bsize;
		sbp->f_blocks = fs.fs_blocks;
		sbp->f_bfree = fs.fs_bfree;
		sbp->f_bavail = fs.fs_bavail;
		sbp->f_files = -1UL;
		sbp->f_ffree = -1UL;
#ifdef	notneeded
		/*
		 * XXX - This is wrong, should be a real fsid
		 */
		bcopy((caddr_t)&vfsp->vfs_fsid,
		    (caddr_t)&sbp->f_fsid, sizeof (fsid_t));
#endif
		strncpy(sbp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name, FSTYPSZ);
		sbp->f_flag = vf_to_stf(vfsp->vfs_flag);
		sbp->f_namemax = -1UL;
	}
#ifdef NFSDEBUG
	printf("nfs_statvfs returning %d\n", error);
#endif
	return (error);
}

/*
 * Flush dirty nfs files for file system vfsp.
 * If vfsp == NULL, all nfs files are flushed.
 */
/*ARGSUSED*/
STATIC int
nfs_sync(vfsp, flag, cr)
	struct vfs *vfsp;
	short flag;
	struct cred *cr;
{
	static int nfslock;

	if (nfslock == 0 && !(flag & SYNC_ATTR)) {
#ifdef NFSDEBUG
		printf("nfs_sync\n");
#endif
		nfslock++;
		rflush(vfsp);
		nfslock = 0;
	}
	return (0);
}

/*ARGSUSED*/
STATIC int
nfs_vget(vfsp, vpp, fidp)
	vfs_t	*vfsp;
	vnode_t	**vpp;
	fid_t	*fidp;
{
	cmn_err (CE_WARN, "nfs_vget called\n");
	return (ENOSYS);
}

/*ARGSUSED*/
STATIC int
nfs_mountroot(vfsp, why)
	vfs_t		*vfsp;
	whymountroot_t	why;
{
	cmn_err (CE_WARN, "nfs_mountroot called\n");
	return (ENOSYS);
}

/*ARGSUSED*/
STATIC int
nfs_swapvp(vfsp, vpp, nm)
	vfs_t	*vfsp;
	vnode_t	**vpp;
	char	*nm;
{
	cmn_err (CE_WARN, "nfs_swapvp called\n");
	return (ENOSYS);
}

STATIC int
nfs_nosys()
{
	return(ENOSYS);
}
