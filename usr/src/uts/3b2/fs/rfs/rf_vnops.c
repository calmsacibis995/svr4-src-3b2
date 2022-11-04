#ident	"@(#)fs:fs/rfs/rf_vnops.c	1.21.1.1 UNOFFICIAL"
#include "sys/list.h"
#include "sys/types.h"
#include "sys/sysinfo.h"
#include "sys/fs/rf_acct.h"
#include "sys/errno.h"
#include "sys/param.h"
#include "sys/cmn_err.h"
#include "sys/vnode.h"
#include "sys/vfs.h"
#include "sys/debug.h"
#include "sys/cred.h"
#include "sys/uio.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/sbd.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/stream.h"
#include "sys/statvfs.h"
#include "vm/seg.h"
#include "rf_admin.h"
#include "sys/rf_comm.h"
#include "sys/nserve.h"
#include "sys/rf_cirmgr.h"
#include "sys/rf_messg.h"
#include "sys/buf.h"
#include "sys/stat.h"
#include "sys/statfs.h"
#include "sys/fcntl.h"
#include "sys/file.h"
#include "sys/pathname.h"
#include "sys/mode.h"
#include "sys/proc.h"
#include "sys/sysmacros.h"
#include "sys/mman.h"
#include "vm/page.h"
#include "vm/pvn.h"
#include "vm/rm.h"
#include "vm/seg_vn.h"
#include "sys/fs/rf_vfs.h"
#include "fs/fs_subr.h"
#include "sys/hetero.h"
#include "rf_canon.h"
#include "rfcl_subr.h"
#include "du.h"
#include "rf_cache.h"
#include "rf_auth.h"
#include "sys/idtab.h"
#include "sys/inline.h"
#include "sys/kmem.h"
#include "sys/systm.h"
#include "vm/seg_map.h"

/*
 * RFS vnode ops and vector.
 */

/* imports */
extern void	map_addr();
extern int	as_unmap();
extern int	as_map();
extern void	bp_mapin();

STATIC int	rf_open();
STATIC int	rf_close();
STATIC int	rf_read();
STATIC int	rf_write();
STATIC int	rf_ioctl();
STATIC int	rf_getattr();
STATIC int	rf_setattr();
STATIC int	rf_access();
STATIC int	rf_lookup();
STATIC int	rf_create();
STATIC int	rf_remove();
int		rf_link();
STATIC int	rf_rename();
STATIC int	rf_mkdir();
STATIC int	rf_rmdir();
STATIC int	rf_readdir();
STATIC int	rf_symlink();
STATIC int	rf_readlink();
STATIC int	rf_fsync();
STATIC void	rf_inactive();
STATIC int	rf_fid();
STATIC void	rf_rwlock();
STATIC void	rf_rwunlock();
STATIC int	rf_seek();
STATIC int	rf_cmp();
STATIC int	rf_frlock();
STATIC int	rf_space();
STATIC int	rf_setfl();
STATIC int	rf_getpage();
STATIC int	rf_putpage();
STATIC int	rf_map();
STATIC int	rf_addmap();
STATIC int	rf_delmap();
STATIC int	rf_poll();
int		rf_nosys();

struct vnodeops rf_vnodeops = {
	rf_open,
	rf_close,
	rf_read,
	rf_write,
	rf_ioctl,
	rf_setfl,
	rf_getattr,
	rf_setattr,
	rf_access,
	rf_lookup,
	rf_create,
	rf_remove,
	rf_link,
	rf_rename,
	rf_mkdir,
	rf_rmdir,
	rf_readdir,
	rf_symlink,
	rf_readlink,
	rf_fsync,
	rf_inactive,
	rf_fid,
	rf_rwlock,
	rf_rwunlock,
	rf_seek,
	rf_cmp,
	rf_frlock,
	rf_space,
	rf_nosys,	/* realvp */
	rf_getpage,
	rf_putpage,
	rf_map,
	rf_addmap,
	rf_delmap,
	rf_poll,
	rf_nosys,	/* dump */
	rf_nosys,	/* filler */
	rf_nosys,	/* filler */
	rf_nosys,	/* filler */
	rf_nosys,	/* filler */
	rf_nosys,	/* filler */
	rf_nosys	/* filler */
};

STATIC int	rf_read_bypass();
STATIC int	rf_read_cache();
STATIC int	rf_getapage();
STATIC int	rf_pushpages();
STATIC int	rflkc_acc_hit();
STATIC int	rf_ioctl_resp();
STATIC int	rf_lookup_pass();
STATIC int	rf_lookup_fail();
STATIC int	rf_dofcntl();
STATIC int	rf_symlnk_msg();
STATIC int	rf_symlnk_resp();
STATIC int	rf_readdir_resp();
STATIC int	rf_readdir_fcanon();

/*
 * rfcl_lookup_cache is a single element cache to hold a rf_attr structure and
 * VOP_ACCESS return values related to a vnode.  Servers return these attributes
 * on every successful lookup.
 */
rfcl_lookup_cache_t	rfcl_lookup_cache;

/* ARGSUSED */
STATIC void
rf_inactive(vp, cr)
	register vnode_t	*vp;
	cred_t			*cr;
{
	sndd_t			*sdp = VTOSD(vp);

	rfcl_fsinfo.fsivop_other++;
	ASSERT(!vp->v_count);
	ASSERT(!(sdp->sd_stat & SDLOCKED));
	/*
	 * NOTE:  assertion assumes this is not called when
	 * disposing of rf_vfs
	 */
	ASSERT(VFTORF(vp->v_vfsp)->rfvfs_refcnt > 1);
	/*
         * If SDINTER, then the RF_INACTIVE request was piggybacked on a
	 * RF_CLOSE.  The third argument to rfcl_sdrele will be 0 in this
	 * case, directing it to not go remote again.
	 */
	ASSERT(!(sdp->sd_stat & SDINTER) ||
          QPTOGP(sdp->sd_queue)->version > RFS1DOT0);
	rfcl_sdrele(&sdp, cr, !(sdp->sd_stat & SDINTER));
}

/* ARGSUSED */
STATIC int
rf_fid(vp, fidpp)
	vnode_t *vp;
	fid_t **fidpp;
{
	rfcl_fsinfo.fsivop_other++;
	cmn_err(CE_WARN,"rf_fid called\n");
	return ENOSYS;
}

STATIC void
rf_rwlock(vp)
	vnode_t *vp;
{
	rfcl_fsinfo.fsivop_other++;
	SDLOCK(VTOSD(vp));
}

STATIC void
rf_rwunlock(vp)
	vnode_t *vp;
{
	rfcl_fsinfo.fsivop_other++;
	SDUNLOCK(VTOSD(vp));
}

/* ARGSUSED */
STATIC int
rf_seek(vp, ooff, noffp)
	vnode_t *vp;
	off_t ooff;
	off_t *noffp;
{
	rfcl_fsinfo.fsivop_other++;
	/*
	 * No need to go remote because the offset was already
	 * adjusted with VOP_GETATTR, and that is the documented
	 * behavior of RFS files.
	 * TO DO:  is that good enough?
	 */
	return 0;
}

/* Ducmp compares two vnodes to see if they refer to the same file.
 */
STATIC int
rf_cmp(vp1,vp2)
	vnode_t *vp1, *vp2;
{
	rfcl_fsinfo.fsivop_other++;
	return vp1 == vp2;
}

STATIC int
rf_poll(vp, events, anyyet, reventsp, phpp)
	vnode_t *vp;
	register short events;
	int anyyet;
	register short *reventsp;
	struct pollhead **phpp;
{
	rfcl_fsinfo.fsivop_other++;
	return fs_poll(vp, events, anyyet, reventsp, phpp);
}

/*
 * Stub for filler routines.
 */
int
rf_nosys()
{
	rfcl_fsinfo.fsivop_other++;
	return ENOSYS;
}

STATIC int
rf_fsync(vp, crp)
	vnode_t		*vp;
	cred_t		*crp;
{
	sndd_t		*sdp = VTOSD(vp);
	mblk_t		*bp = NULL;
	int		error;

	rfcl_fsinfo.fsivop_other++;
	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	/*
	 * The pre-SVR4 system call protocol does not recognize this
	 * operation.
	 */
	if (QPTOGP(sdp->sd_queue)->version < RFS2DOT0) {
		return ENOSYS;
	}
	/*
	 * First synchronously flush all dirty pages back to server.
	 * (There will be some only if the file is stored into via
	 * a mapping, because client caching uses a write-through
	 * policy.)
	 */
	(void)VOP_PUTPAGE(vp, (uint)0, (uint)0, 0, crp);
	/*
	 * Send an RFFSYNC request.
	 */
	if ((error = rfcl_op(sdp, crp, RFFSYNC, &init_rq_arg, &bp,
	  TRUE)) == 0) {
		error = RF_RESP(bp)->rp_errno;
		rf_freemsg(bp);
	}
	return error;
}

STATIC int
rf_access(vp, mode, flags, crp)
	vnode_t		*vp;
	int		mode;
	int		flags;
	cred_t		*crp;
{
	register int	error = 0;
	union rq_arg	rqarg = init_rq_arg;
	mblk_t		*bp = NULL;
	register 	sndd_t *sdp = VTOSD(vp);

	rfcl_fsinfo.fsivop_other++;
	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	if (mode & VWRITE && vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		return EROFS;
	}
	if (QPTOGP(sdp->sd_queue)->version < RFS2DOT0) {
		return du_access(vp, mode, flags, crp);
	}
	if (rflkc_acc_hit(vp, crp)) {
		/*
		 * This is a hit in the rfcl_lookup_cache.  Compose the result
		 * from there.
		 */
		rfc_info.rfci_ac_hit++;
		if (mode & VREAD) {
			error = rfcl_lookup_cache.lkc_read_err;
		}
		if (mode & VWRITE && !error) {
			error = rfcl_lookup_cache.lkc_write_err;
		}
		if (mode & VEXEC && !error) {
			error = rfcl_lookup_cache.lkc_exec_err;
		}
		return error;
	}
	/*
	 * Cache miss.
	 */
	rfc_info.rfci_ac_miss++;
	rqarg.rqmode_op.fmode = mode;
	if ((error = rfcl_op(sdp, crp, RFACCESS, &rqarg, &bp, TRUE)) == 0) {
		rf_freemsg(bp);
	}
	return error;
}

/*
 * Perform close op on the remote file denoted by vp.
 * Returns 0 for success, nonzero errno for failure.
 */
STATIC int
rf_close(vp, flags, cnt, off, crp)
	vnode_t			*vp;
	int			flags;
	int			cnt;
	int			off;
	cred_t			*crp;
{
	register int		error;
	int			nacked = 1;
	mblk_t			*bp = NULL;
	register rf_request_t	*reqp;
	rcvd_t			*rdp;
	register sndd_t		*chansdp = VTOSD(vp);
	register int		vcver = QPTOGP(chansdp->sd_queue)->version;
	size_t			rqsize = RF_MIN_REQ(vcver);
	int			lastclose;

	rfcl_fsinfo.fsivop_close++;
	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	/* create an rd on which to receive the response */
	if ((error = rcvd_create(FALSE, RDSPECIFIC, &rdp)) != 0) {
		return error;
	}
	rdp->rd_sdp = chansdp;

        /*
         * If this is the last close of a SVR4 file, piggyback the RFINACTIVE.
	 */
	if ((lastclose = cnt <= 1 && vcver > RFS1DOT0 && vp->v_count == 1)
	  != 0) {
		if (vp->v_pages != NULL) {
			/*
                         * When pages are at stake, lock chansdp so that any
                         * new references to this file have a chance at using
                         * the pages.  Because SDLOCK can cause a sleep,
                         * v_count is rechecked after locking.
                         */
			SDLOCK(chansdp);
			if (vp->v_count > 1) {
                                SDUNLOCK(chansdp);
                                lastclose = FALSE;
			}
		} else {
			/*
                         * There are no pages cached, using SDINTER rather
                         * than locking allows other processes to obtain
                         * new references to the file without waiting for
                         * this process.
                         */
			chansdp->sd_stat |= SDINTER;
		}
	}

	while (nacked) {
		(void)rf_allocmsg(rqsize, (size_t)0, BPRI_MED, FALSE,
		  NULLCADDR, NULLFRP, &bp);
		ASSERT(bp);
		rfcl_reqsetup(bp, chansdp, crp, RFCLOSE, ULIMIT);
		reqp = RF_REQ(bp);
		reqp->rq_close.fmode = (long)flags;
		reqp->rq_close.foffset = (long)off;
		reqp->rq_close.count = (long)cnt;
		if ((reqp->rq_close.lastclose = lastclose) != 0) {
			reqp->rq_close.vcount = chansdp->sd_remcnt;
		}
		error = rfcl_xac(&bp, rqsize, rdp, vcver, FALSE, &nacked);
	}
	rcvd_free(&rdp);
	if (lastclose && !(chansdp->sd_stat & SDINTER)) {
		chansdp->sd_stat |= SDINTER;
		SDUNLOCK(chansdp);
	}
	if (!error) {
		if (vp->v_type == VREG &&
		  vcver == RFS1DOT0 &&
		  chansdp->sd_stat & SDCACHE &&
		  vp->v_count == 1) {
			/*
			 * For caching across closes.  rfcl_xac handles the
			 * RFS2DOT0 case.
			 */
			chansdp->sd_vcode = RF_RESP(bp)->rp_rval;
		}
		error = RF_RESP(bp)->rp_errno;
		rf_freemsg(bp);
	}
	return error;
}

/*
 * RFS handles VOP_CREATE by propagating its args to the server.
 */
STATIC int
rf_create(dvp, nm, vap, ex, mode, vpp, crp)
	vnode_t		*dvp;	/* parent dir */
	char		*nm;	/* name of file to create */
	vattr_t		*vap;	/* attributes for new file */
	vcexcl_t	ex;	/* exclusive create or not */
	int		mode;	/* open mode */
	vnode_t		**vpp;	/* out param refers to new file */
	cred_t		*crp;
{
	register int	error;
	int		nacked;
	int		ntries;
	mblk_t		*bp;
	rcvd_t		*rdp;
	sndd_t		*giftsdp;
	register sndd_t	*chansdp = VTOSD(dvp);
	register gdp_t	*gp = QPTOGP(chansdp->sd_queue);
	register int	vcver = gp->version;
	register size_t	headsz;
	register size_t	datasz;
	register size_t totalsz;
	int		canon = gp->hetero != NO_CONV;

	*vpp = NULL;		/* vpp is set only in success cases */
	bp = NULL;
	giftsdp = NULL;
	rdp = NULL;

	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	rfcl_fsinfo.fsivop_create++;
	if (vcver < RFS2DOT0) {
		return du_create(dvp, nm, vap, ex, mode, vpp, crp);
	}
	/* create an rd on which to receive the response */
	if ((error = rcvd_create(TRUE, RDSPECIFIC, &rdp)) != 0) {
		goto out;
	}
	rdp->rd_sdp = chansdp;
	/*
	 * Get a send descriptor to hold the remote reference.
	 */
	if ((error = sndd_create(TRUE, &giftsdp)) != 0) {
		goto out;
	}
	/*
	 * Calculate the datasz, up datasz by worst case factor
	 * in heterogeneous environment
	 */
	headsz = RF_MIN_REQ(vcver);
	datasz = gp->hetero != NO_CONV ? sizeof(struct rqmkdent) + MKDENT_XP :
	  sizeof(struct rqmkdent);
	totalsz = headsz + datasz;
	for (ntries = nacked = 1; ntries < RFCL_MAXTRIES && nacked; ntries++) {
		register rf_request_t		*reqp;
		register struct rqmkdent	*rqdp;
		char				*data;
		struct rqmkdent			rqmkdent;

		if ((error = rf_allocmsg(headsz, datasz, BPRI_LO, TRUE,
		  NULLCADDR, NULLFRP, &bp)) != 0) {
			break;
		}
		rfcl_reqsetup(bp, chansdp, crp, RFCREATE, ULIMIT);
		reqp = RF_REQ(bp);
		data = rf_msgdata(bp, headsz);
		if (canon) {
			rqdp = &rqmkdent;
		} else {
			rqdp = (struct rqmkdent *)data;
		}
		(void)strcpy(rqdp->nm, nm);
		vtorf_attr(&rqdp->attr, vap);
		if (canon) {
			datasz = rf_tcanon(MKDENT_FMT, (caddr_t)rqdp, data);
		}
		reqp->rq_create.ex = (long)ex;
		reqp->rq_create.fmode = mode;
		error = rfcl_xac(&bp, totalsz, rdp, vcver, FALSE, &nacked);
	}
	if (!error && (error = RF_RESP(bp)->rp_errno) == 0) {
		rf_message_t	*msg = RF_MSG(bp);

		if (!(msg->m_stat & RF_GIFT)) {
			gdp_discon("rf_create: no file reference",
			  QPTOGP((queue_t *)msg->m_queue));
			error = EPROTO;
		} else {
			sndd_set(giftsdp, (queue_t *)msg->m_queue,
			  msg->m_giftid);
			if ((error = rfcl_findsndd(&giftsdp, crp, bp,
			  dvp->v_vfsp)) != 0) {
				*vpp = NULLVP;
			}
				*vpp = SDTOV(giftsdp);
		}
	}
out:
	rf_freemsg(bp);
	rcvd_free(&rdp);
	if (error) {
		sndd_free(&giftsdp);
	}
	return error;
}

STATIC int
rf_space(vp, cmd, arg, flag, offset, crp)
	vnode_t	*vp;
	int	cmd;
	caddr_t	arg;
	int	flag;
	off_t	offset;
	cred_t	*crp;
{
	rfcl_fsinfo.fsivop_other++;
	return rf_dofcntl(RFSPACE, vp, cmd, arg, flag, offset, crp);
}

STATIC int
rf_frlock(vp, cmd, arg, flag, offset, crp)
	vnode_t	*vp;
	int	cmd;
	caddr_t	arg;
	int	flag;
	off_t	offset;
	cred_t	*crp;
{
	rfcl_fsinfo.fsivop_other++;
	return rf_dofcntl(RFFRLOCK, vp, cmd, arg, flag, offset, crp);
}

STATIC int
rf_setfl(vp, oflags, nflags, crp)
	vnode_t	*vp;
	int	oflags;
	int	nflags;
	cred_t	*crp;
{
	rfcl_fsinfo.fsivop_other++;
	return rf_dofcntl(RFSETFL, vp, F_SETFL, (caddr_t)oflags,
	nflags, 0, crp);
}

STATIC int
rf_dofcntl(op, vp, cmd, arg, flag, offset, crp)
	int			op;
	vnode_t			*vp;
	int			cmd;
	caddr_t			arg;
	int			flag;
	off_t			offset;
	cred_t			*crp;
{
	register int		ntries;
	register int		error = 0;
	int			nacked;
	mblk_t			*bp = NULL;
	rcvd_t			*rdp;
	register sndd_t		*chansdp  = VTOSD(vp);
	register gdp_t		*gp = QPTOGP(chansdp->sd_queue);
	size_t			totalsz;
	size_t			hdrsz = RF_MIN_REQ(gp->version);
	caddr_t			data;
	int			canon = gp->hetero != NO_CONV;
	struct flock		*flp = (struct flock *)arg;
	size_t			datasz;

	datasz = canon ? sizeof(struct flock) + FLOCK_XP - MINXPAND :
		  sizeof(struct flock);
	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	if (gp->version < RFS2DOT0) {
		return du_fcntl(op, vp, cmd, arg, flag, offset, crp);
	}
	/* create an rd on which to receive the response */
	if ((error = rcvd_create(TRUE, RDSPECIFIC, &rdp)) != 0) {
		return error;
	}
	rdp->rd_sdp = chansdp;
	totalsz =  hdrsz + datasz;
	for (ntries = nacked = 1; ntries < RFCL_MAXTRIES && nacked; ntries++) {
		register rf_request_t	*reqp;

		if ((error = rf_allocmsg(hdrsz, datasz, BPRI_LO, TRUE,
		  NULLCADDR, NULLFRP, &bp)) != 0) {
			break;
		}
		rfcl_reqsetup(bp, chansdp, crp, op, ULIMIT);
		reqp = RF_REQ(bp);
		reqp->rq_fcntl.cmd = cmd;
		reqp->rq_fcntl.fcntl = (long)arg;
		reqp->rq_fcntl.offset = offset;
		reqp->rq_fcntl.fflag = flag;
		/*
		 * Send record locking data with request.
		 */
		switch (cmd) {
		case F_GETLK:
		case F_O_GETLK:
		case F_SETLK:
		case F_SETLKW:
		case F_FREESP:
			data = rf_msgdata(bp, hdrsz);
			if (canon) {
				reqp->rq_fcntl.prewrite = rf_tcanon(FLOCK_FMT,
				  (caddr_t)arg, data);
			} else {
				*(flock_t *)data = *(flock_t*)arg;
				reqp->rq_fcntl.prewrite = sizeof(flock_t);
			}
		}
		error = rfcl_xac(&bp, totalsz, rdp, gp->version,
		  FALSE, &nacked);
	}
	hdrsz = RF_MIN_RESP(gp->version);
	rcvd_free(&rdp);
	if (!error && (error = RF_RESP(bp)->rp_errno) == 0 &&
	  op == RFFRLOCK && (cmd == F_GETLK || cmd == F_O_GETLK) &&
	  (error = RF_PULLUP(bp, hdrsz, datasz)) == 0) {
		data = rf_msgdata(bp, hdrsz);
		if (gp->hetero != NO_CONV) {
			if (!rf_fcanon(FLOCK_FMT, data, data + datasz,
			  (caddr_t)flp)) {
				gdp_discon("rf_dofcntl bad data from server",
				  gp);
				error = EPROTO;
			}
		} else {
			flp[0] = ((struct flock *)data)[0];
		}
	}
	rf_freemsg(bp);
	return error;
}

/*
 * Send an RFGETATTR request, if successful, copies the rf_attr_t from the
 * response into *vap.
 */
STATIC int
rf_getattr(vp, vap, flags, crp)
	vnode_t			*vp;
	vattr_t			*vap;
	int			flags;
	cred_t			*crp;
{
	sndd_t			*sdp = VTOSD(vp);
	register gdp_t		*gp = QPTOGP(sdp->sd_queue);
	register int		error;
	union rq_arg		rqarg = init_rq_arg;
	mblk_t			*bp = NULL;
	size_t			hdrsz;
	size_t			datasz;

	rfcl_fsinfo.fsivop_other++;
	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	if (gp->version < RFS2DOT0) {
		return du_getattr(vp, vap, flags, crp);
	}

	/*
	 * Use the rf_attr_t in rfcl_lookup_cache if it is valid (its crp is
	 * non-NULL) and matches this vp.
	 */
	if (rfcl_lookup_cache.lkc_crp && rfcl_lookup_cache.lkc_vp == vp) {
		rftov_attr(vap, &rfcl_lookup_cache.lkc_attr);
		if (flags & ATTR_EXEC) {
			/* shut out unauthorized setuid programs */
			vap->va_uid = gluid(gp, rfcl_lookup_cache.lkc_xuid);
			vap->va_gid = glgid(gp, rfcl_lookup_cache.lkc_xgid);
		}
		rfc_info.rfci_vc_hit++;
		return 0;
	} else {
		rfc_info.rfci_vc_miss++;
	}
	rqarg.rqgetattr.mask = vap->va_mask;

	hdrsz = RF_MIN_RESP(gp->version);
	datasz = gp->hetero == NO_CONV ? sizeof(rf_attr_t) :
	  sizeof(rf_attr_t) + ATTR_XP - MINXPAND;
	if ((error = rfcl_op(sdp, crp, RFGETATTR, &rqarg, &bp, TRUE)) == 0 &&
	  (error = RF_RESP(bp)->rp_errno) == 0 &&
	  (error = RF_PULLUP(bp, hdrsz, datasz)) == 0) {

		caddr_t		rpdata = rf_msgdata(bp, hdrsz);

		/*
		 * Extract the rf_attr_t from the response.
		 */
		if (gp->hetero != NO_CONV &&
		  !rf_fcanon(ATTR_FMT, rpdata, rpdata + datasz, rpdata)) {
			gdp_discon("rf_getattr bad data from server", gp);
			error = EPROTO;
		} else {
			rftov_attr(vap, (rf_attr_t *)rpdata);
			if (flags & ATTR_EXEC) {
				rf_common_t	*cop = RF_COM(bp);

				/* shut out unauthorized setuid programs */

				vap->va_uid = gluid(gp, cop->co_uid);
				vap->va_gid = glgid(gp, cop->co_gid);
			}
			vap->va_vcode = 0;

			/*
			 * NOTE:  If va_fsid becomes other than the dev number,
			 * the following will be obsolete.  It sets the high
			 * byte of the low word(sic) of that field to the one's
			 * complement of the gdp index of the circuit to the
			 * server holding the file.
			 */

#ifdef SHORT_DEVS
			hibyte(loword(vap->va_fsid)) = ~(gp - gdp);
#else
			hiword(vap->va_fsid) = ~(gp - gdp);
#endif
			ASSERT((long)vap->va_fsid < 0);
		}
	}
	rf_freemsg(bp);
	return error;
}

/* Pass user-supplied ioctl arg to server owning file referenced by vp.
 * Unpredictably, from the point of view of rf_ioctl, RFCOPYIN/OUT
 * interchanges may occur.
 * Returns 0 for success, nonzero errno for failure.
 */
STATIC int
rf_ioctl(vp, cmd, arg, flag, crp)
	vnode_t		*vp;
	int		cmd;
	caddr_t		arg;
	int		flag;
	cred_t		*crp;
{
	register int	error;
	mblk_t		*bp = NULL;
	int		nacked;
	register int	ntries;
	rf_request_t	*reqp;
	rcvd_t		*rdp;
	sndd_t		*replysdp;
	sndd_t		*giftsdp;
	register 	sndd_t *chansdp = VTOSD(vp);
	register int	vcver = QPTOGP(chansdp->sd_queue)->version;

	rfcl_fsinfo.fsivop_other++;
	if (RF_SERVER()) {
		return EMULTIHOP;
	}

	bp = NULL;
	rdp = NULL;
	replysdp = NULL;

	/* create an rd on which to receive the response */
	if ((error = rcvd_create(TRUE, RDSPECIFIC, &rdp)) != 0) {
		goto out;
	}
	rdp->rd_sdp = chansdp;
	/* Create a sndd for copysync responses, if any */
	if (error = sndd_create(TRUE, &replysdp)) {
		goto out;
	}

	/* Create a sndd for a gift, if any (see comment in the resp routine) */
	if ((error = sndd_create(TRUE, &giftsdp)) != 0) {
		rcvd_free(&rdp);
		sndd_free(&replysdp);
		return error;
	}
	for (ntries = nacked = 1; ntries < RFCL_MAXTRIES && nacked; ntries++) {
		if ((error = rf_allocmsg(RF_MIN_REQ(vcver), (size_t)0, BPRI_LO,
		  TRUE, NULLCADDR, NULLFRP, &bp)) != 0) {
			break;
		}
		rfcl_reqsetup(bp, chansdp, crp, RFIOCTL, ULIMIT);
		reqp = RF_REQ(bp);
		reqp->rq_ioctl.cmd = cmd;
		reqp->rq_ioctl.arg = (long)arg;
		reqp->rq_ioctl.fflag = flag;
		error = rfcl_xac(&bp, RF_MIN_REQ(vcver), rdp, vcver, FALSE,
		  &nacked);
	}
	if (!error) {
		error = rf_ioctl_resp(rdp, vcver, bp, giftsdp, replysdp, crp);
	}
out:
	rcvd_free(&rdp);
	if (error) {
		sndd_free(&replysdp);
	}
	return error;
}

/*
 * rf_ioctl_resp must be ready for a wide variety of responses, and is
 * of necessity very trusting of the server.  Since the user's cmd
 * and arg are interpreted on the server, it is impossible to fully predict
 * the amount(s) or direction(s) of data movement.  The rfcl_writemove/rf_cpout
 * routines are used as needed to handle DUCPIN/DUCPOUT responses.
 * May reuse and always frees bp.  Either frees giftsdp or completes its
 * definition.  Returns any error encountered, even rp_errno in the
 * bracketing response.
 */
STATIC int
rf_ioctl_resp(rdp, vcver, bp, giftsdp, replysdp, crp)
	rcvd_t			*rdp;
	int			vcver;
	mblk_t			*bp;
	sndd_t			*giftsdp;
	sndd_t			*replysdp;
	cred_t			*crp;
{
	register rf_response_t	*resp;
	register rf_message_t	*msg;
	register rf_common_t	*cop;
	register sndd_t		*chansdp = rdp->rd_sdp;
	uio_t		 	uio;
	iovec_t			iov;
	register int		error = 0;
	int			uio_error = 0;	/* save any uiomove error */
	rf_rwa_t	rf_rwa;

	uio.uio_segflg = UIO_USERSPACE;
	uio.uio_offset = 0;
	uio.uio_iovcnt = 1;
	uio.uio_iov = &iov;
	rf_rwa.uiop = &uio;
	rf_rwa.cached = FALSE;
	rf_rwa.wr_ioflag = 0;
	rf_rwa.wr_kern = FALSE;

	for (;;) {
		resp = RF_RESP(bp);
		msg = RF_MSG(bp);
		cop = RF_COM(bp);
		if (cop->co_opcode == RFIOCTL) {
			if (!rf_sigisempty(resp, vcver)) {
				rf_postrpsigs(resp, vcver, u.u_procp);
			}
			error = resp->rp_errno;
			break;
		} else if (cop->co_opcode == RFCOPYIN) {
			uio.uio_resid = iov.iov_len = resp->rp_count;
			iov.iov_base = (caddr_t)resp->rp_xfer.buf;
			if (error = rfcl_writemove(&bp, &rf_rwa, replysdp,
			  chansdp, &uio_error)) {
				/*
				 * rfcl_writemove sent error to server already
				 */
				break;
			}
		} else if (cop->co_opcode == RFCOPYOUT) {
			uio.uio_resid = iov.iov_len = resp->rp_count;
			iov.iov_base = (caddr_t)resp->rp_xfer.buf;
			if (error = rfcl_readmove(&bp, &uio, replysdp,
			  &uio_error)) {
				break;
			}
		} else {
			gdp_discon("rf_ioctl_pass bad opcode",
			  QPTOGP(chansdp->sd_queue));
			error = EPROTO;
			break;
		}
		/*
		 * as long as RFCOPYIN/DUCOPYOUT responses are received,
		 * remain in this for loop, receive new messages here.
		 */
		if (error = rf_rcvmsg(rdp, &bp)) {
			msg = NULL;
			break;
		}
	} /* forever */
	/*
	 * This is a kludge to allow file systems with ioctls that
	 * open files unexpectedly to work.  The only known file
	 * system to do this is /proc.
	 * If a gift has been returned this means a file has been
	 * opened on the server.  We must allocate an entry in the
	 * file table for it.
	 */
	if (!error && msg && msg->m_stat & RF_GIFT) {
		struct file	*fp;	/* sink */
		int		fd;	/* sink */

		sndd_set(giftsdp, (queue_t *)msg->m_queue, msg->m_giftid);
		if (!(error = rfcl_findsndd(&giftsdp, crp, bp,
		  SDTOV(chansdp)->v_vfsp))) {
			struct vnode *vp = SDTOV(giftsdp);

			if (error = falloc(vp, FREAD|FMASK, &fp, &fd)) {
				VN_RELE(vp);
			}
		}
	} else {
		sndd_free(&giftsdp);
	}
	if (!error && vcver >= RFS2DOT0 && SDTOV(chansdp)->v_type == VREG) {
		error = rfc_v2vcodeck(chansdp, bp);
	}
	rf_freemsg(bp);
	return error ? error : uio_error;
}

/*
 * rf_link is used by both system call protocol (RFLINK == old DULINK1) and
 * by ops protocol.
 */
int
rf_link(tdvp, fvp, nm, crp)
	vnode_t		*tdvp;
	vnode_t		*fvp;
	char		*nm;
	cred_t		*crp;
{
	register int	error;
	int		nacked;
	int		ntries;
	mblk_t		*bp = NULL;
	rcvd_t		*rdp;
	register sndd_t	*chansdp = VTOSD(tdvp);
	register sndd_t	*fsdp = VTOSD(fvp);
	register int	vcver = QPTOGP(chansdp->sd_queue)->version;
	register size_t	headsz = RF_MIN_REQ(vcver);
	register size_t	datasz = strlen(nm) + 1;
	register size_t	totalsz = headsz + datasz;

	rfcl_fsinfo.fsivop_other++;
	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	if (fsdp->sd_queue != chansdp->sd_queue) {
		return EXDEV;
	}
	/* create an rd on which to receive the response */
	if ((error = rcvd_create(TRUE, RDSPECIFIC, &rdp)) != 0) {
		return error;
	}
	rdp->rd_sdp = chansdp;
	for (ntries = nacked = 1; ntries < RFCL_MAXTRIES && nacked; ntries++) {
		if ((error = rf_allocmsg(headsz, datasz, BPRI_LO, TRUE,
		  NULLCADDR, NULLFRP, &bp)) != 0) {
			break;
		}
		rfcl_reqsetup(bp, chansdp, crp, RFLINK, ULIMIT);
		RF_REQ(bp)->rq_rflink.link = fsdp->sd_connid;
		(void)strcpy(rf_msgdata(bp, headsz), nm);
		error = rfcl_xac(&bp, totalsz, rdp, vcver, FALSE, &nacked);
	}
	rcvd_free(&rdp);
	if (!error) {
		error = RF_RESP(bp)->rp_errno;
		rf_freemsg(bp);
	}
	return error;
}

STATIC int
rf_lookup(dvp, comp, vpp, pnp, flags, rdirvp, crp)
	vnode_t 	*dvp;		/* current directory */
	char 		*comp;		/* current component of name */
	vnode_t 	**vpp;		/* return vnode pointer */
	pathname_t 	*pnp;		/* pathname structure */
	int 		flags;		/* is containing directory needed? */
	vnode_t 	*rdirvp;	/* root vnode for this process */
	cred_t 		*crp;		/* credentials structure */
{
	register int	vcver = QPTOGP(VTOSD(dvp)->sd_queue)->version;
	register int	error = 0;
	register sndd_t	*chansdp = VTOSD(dvp);
	register int	nsubmounts = dvp->v_vfsp->vfs_nsubmounts;
	mblk_t		*bp = NULL;
	rcvd_t		*rdp = NULL;
	sndd_t		*giftsdp = NULL;
	size_t		complen = strlen(comp);
	int		ntries;
	int		nacked;
	size_t		datasz;
	size_t		headsz;
	size_t		totalsz;
	size_t		datalen = QPTOGP(VTOSD(dvp)->sd_queue)->datasz;

	rfcl_fsinfo.fsivop_lookup++;
	*vpp = NULLVP;			/* only reset for success */
	if (RF_SERVER()) {
		error = EMULTIHOP;
		goto failed;
	}
	if (vcver < RFS2DOT0) {
		return du_lookup(dvp, comp, vpp, pnp, flags, rdirvp, crp);
	}
	/* create an rd on which to receive the response */
	if ((error = rcvd_create(TRUE, RDSPECIFIC, &rdp)) != 0) {
		goto failed;
	}
	rdp->rd_sdp = chansdp;
	datasz = complen + 1;
	if (nsubmounts) {
		/*
		 * Single component lookup to allow local crossing of
		 * submounts.
		 */
		flags &= ~LOOKUP_DIR;
	} else {
		datasz += pnp->pn_pathlen;
		if (pnp->pn_pathlen == 0) {
			/*
			 * If we're looking up the last component, the
			 * directory was found by a previous call to
			 * VOP_LOOKUP.  We suppress looking up the
			 * parent again, which would cause lookuppn
			 * to mistake the last component as an alias
			 * for the parent directory.  This is probably
			 * symptomatic of a mismatch between the spec
			 * of VOP_LOOKUP and how RFS handles pathnames.
			 */
			flags &= ~LOOKUP_DIR;
		}
	}
	if (datasz > MAXPATHLEN) {
		error = ENAMETOOLONG;
		goto failed;
	}
	if (datasz > datalen) {
		error = ENOMEM;
		goto failed;
	}
	headsz = RF_MIN_REQ(vcver);
	totalsz = headsz + datasz;
	if ((error = sndd_create(TRUE, &giftsdp)) != 0) {
		goto failed;
	}
	for (ntries = nacked = 1; ntries < RFCL_MAXTRIES && nacked; ntries++) {
		rf_request_t	*rqp;
		caddr_t		data;

		if ((error = rf_allocmsg(headsz, datasz, BPRI_LO, TRUE,
		  NULLCADDR, NULLFRP, &bp)) != 0) {
			goto failed;
		}
		rfcl_reqsetup(bp, chansdp, crp, RFLOOKUP, ULIMIT);
		rqp = RF_REQ(bp);
		data = rf_msgdata(bp, headsz);
		if (rdirvp && ISRFSVP(rdirvp) &&
		  VTOSD(rdirvp)->sd_connid == chansdp->sd_connid) {
			rqp->rq_rrdir = VTOSD(rdirvp)->sd_connid;
		} else {
			rqp->rq_rrdir = 0;
		}
		(void)strcpy(data, comp);
		if (!nsubmounts) {
			/*
			 * Pathname, rather than component, lookup.
			 */
			(void)strcpy(data + complen, pnp->pn_path);
		}
		rqp->rq_lookup.flags = flags;
		error = rfcl_xac(&bp, totalsz, rdp, vcver, FALSE, &nacked);
	}
	rcvd_free(&rdp);
	if (!error) {
		if ((error = RF_RESP(bp)->rp_errno) == 0) {
			if ((error = rf_lookup_pass(dvp, vpp, pnp, flags,
			  rdirvp, crp, bp, giftsdp, nsubmounts)) != 0) {
				pn_setlast(pnp);
			}
		} else {
			error = rf_lookup_fail(dvp, pnp, crp, bp, giftsdp,
			  error, nsubmounts);
		}
		rf_freemsg(bp);
	} else {
		pn_setlast(pnp);
	}
	return error;
failed:
	pn_setlast(pnp);
	rf_freemsg(bp);
	rcvd_free(&rdp);
	sndd_free(&giftsdp);
	return error;
}

/*
 * Either RFDOTDOT, RFPATHREVAL, or completed successfully.
 * In the non-error cases, update pnp and vpp .  Get new reference to
 * result vnode and return through vpp.
 */
STATIC int
rf_lookup_pass(dvp, vpp, pnp, flags, rdirvp, crp, bp, giftsdp, nsubmounts)
	vnode_t			*dvp;
	vnode_t			**vpp;
	pathname_t		*pnp;
	int			flags;
	vnode_t			*rdirvp;
	cred_t			*crp;
	register mblk_t		*bp;
	sndd_t			*giftsdp;
	int			nsubmounts;
{
	register rf_message_t	*msg;
	register rf_response_t	*rp;
	register rf_common_t	*cop;
	sndd_t			*chansdp = VTOSD(dvp);
	gdp_t			*gp = QPTOGP(chansdp->sd_queue);
	int			vcver = gp->version;
	caddr_t			data;
	register int		datalen;
	vnode_t			*vp;
	int			error = 0;
	size_t			hdrsz = RF_MIN_RESP(vcver);

	if (RF_COM(bp)->co_opcode != RFLOOKUP &&
	  (error = RF_PULLUP(bp, hdrsz, (size_t)RF_MSG(bp)->m_size - hdrsz))
	  != 0) {
		return error;
	}

	msg = RF_MSG(bp);
	rp = RF_RESP(bp);
	cop = RF_COM(bp);
	data = rf_msgdata(bp, hdrsz);

	switch ((int)cop->co_opcode) {
	case RFLOOKUP:
		if (!(msg->m_stat & RF_GIFT)) {
			gdp_discon("rf_lookup_pass: no file reference", gp);
			sndd_free(&giftsdp);
			return EPROTO;
		}
		if (!data) {
			gdp_discon("rf_lookup_pass no response data", gp);
			return EPROTO;
		}
		if (!nsubmounts) {
			/*
			 * Pathname, rather than component, lookup.
			 *
		 	 * If LOOKUP_DIR flag is set we were trying to get
			 * a reference to the parent directory of the last
		 	 * componenent.  Leave the last component in the
		 	 * path structure so we can go remote again to get
		 	 * a reference to it.
		 	 * Otherwise, set the path pointer to point to the
		 	 * end of the pathname and set the pathlen to 0,
		 	 * as expected by lookuppn().
		 	 */
			pnp->pn_path += pnp->pn_pathlen;
			pnp->pn_pathlen = 0;
			if (flags & LOOKUP_DIR) {
				pn_setlast(pnp);
			}
		}
		sndd_set(giftsdp, (queue_t *)msg->m_queue, msg->m_giftid);
		if (!(error = rfcl_findsndd(&giftsdp, crp, bp,
		  dvp->v_vfsp))) {
			vp = SDTOV(giftsdp);
			if (!rp->rp_nodata) {
				/*
				 * Set up access and rf_attr cache information.
				 * Don't have to hold vp because we toss the
				 * cache on any RFS request that is not a
				 * VN_RELE request for a vnode other than
				 * this.
				 */
				if (rfcl_lookup_cache.lkc_crp) {
					crfree(rfcl_lookup_cache.lkc_crp);
				}
				crhold(crp);
				rfcl_lookup_cache.lkc_crp =  crp;
				rfcl_lookup_cache.lkc_vp = vp;
				if (gp->hetero != NO_CONV) {
					caddr_t	info;

					info =
					  (caddr_t)&rfcl_lookup_cache.lkc_info;
					if (!rf_fcanon(RFLKC_FMT, data, data +
					  sizeof(rflkc_info_t) + RFLKC_XP,
					  info)) {
						gdp_discon("rf_lookup bad data",
						  gp);
						error = EPROTO;
						break;
					}
				} else {
					rfcl_lookup_cache.lkc_info =
					  *(rflkc_info_t *)data;
				}
				rfcl_lookup_cache.lkc_xuid = cop->co_uid;
				rfcl_lookup_cache.lkc_xgid = cop->co_gid;
#ifdef SHORT_DEVS
				hibyte(
				  loword(rfcl_lookup_cache.lkc_attr.rfa_fsid)
				  ) = ~(gp - gdp);
#else
				hiword(rfcl_lookup_cache.lkc_attr.rfa_fsid) =
				  ~(gp - gdp);
#endif
				ASSERT((long)
				  rfcl_lookup_cache.lkc_attr.rfa_fsid < 0);
			}
#ifndef WRETCHED_HOOK_REMOVED
			if (u.u_syscall == DUMOUNT) {
				if (--vp->v_count == 0) {
					rf_inactive(vp, crp);
				}
				error = EREMOTE;
			}
#endif
		}
		break;
	case RFDOTDOT:
		sndd_free(&giftsdp);
		datalen = rp->rp_v2gift.pathlen;
		if (pnp->pn_pathlen < datalen) {
			gdp_discon("rf_lookup_pass too much response data", gp);
			return EPROTO;
		}
		pnp->pn_path += pnp->pn_pathlen - datalen;
		pnp->pn_pathlen = datalen;
		vp = ((rf_vfs_t *)(dvp->v_vfsp->vfs_data))->rfvfs_rootvp;
		VN_HOLD(vp);
		break;
	case RFPATHREVAL:
		sndd_free(&giftsdp);
		if (!data) {
			gdp_discon("rf_lookup_pass no response data", gp);
			return EPROTO;
		}
		if ((error = pn_set(pnp, data)) != 0) {
			return error;
		}
		if (pn_peekchar(pnp) == '/') {
			pn_skipslash(pnp);
			vp = rdirvp;
		} else {
			vp = ((rf_vfs_t *)
			  (dvp->v_vfsp->vfs_data))->rfvfs_rootvp;
		}
		VN_HOLD(vp);
		break;
	default:
		sndd_free(&giftsdp);
		gdp_discon("rf_lookup_pass bad opcode", gp);
		return EPROTO;
	}
	if (!error) {
		*vpp = vp;
	}
	return error;
}

/*
 * If the error returned is ENOENT, reset the path structure to the
 * path returned from the server.  This is special treatment for
 * multi-component lookup.
 * Propagate any error to the upper level.
 */
STATIC int
rf_lookup_fail(dvp, pnp, crp, bp, giftsdp, error, nsubmounts)
	vnode_t		*dvp;
	pathname_t	*pnp;
	cred_t		*crp;
	mblk_t		*bp;
	sndd_t		*giftsdp;
	int		error;
	int		nsubmounts;
{
	rfcl_giftfree(bp, &giftsdp, crp);
	if (!nsubmounts && error == ENOENT) {
		/*
		 * First condition implies pathname, rather than component,
		 * lookup.
		 */
		register int	datalen = RF_RESP(bp)->rp_v2gift.pathlen;

		if (pnp->pn_pathlen < datalen) {
			gdp_discon("rf_lookup_fail too much response data",
			  QPTOGP(VTOSD(dvp)->sd_queue));
			pn_setlast(pnp);
			error = EPROTO;
		} else {
			pnp->pn_path = pnp->pn_path + pnp->pn_pathlen - datalen;
			pnp->pn_pathlen = datalen;
		}
	} else {
		pn_setlast(pnp);
	}
	return error;
}

STATIC int
rf_mkdir(dvp, nm, vap, vpp, crp)
	vnode_t			*dvp;		/* parent directory */
	char			*nm;		/* name of new dir */
	vattr_t			*vap;		/* attributes of new dir */
	vnode_t			**vpp;		/* out param:  new dir's */
	cred_t			*crp;
{
	register int		error;
	int			nacked;
	int			ntries;
	mblk_t			*bp = NULL;
	rcvd_t			*rdp = NULL;
	sndd_t			*giftsdp = NULL;
	register sndd_t		*chansdp = VTOSD(dvp);
	register gdp_t		*gp = QPTOGP(chansdp->sd_queue);
	register int		vcver = gp->version;
	size_t			datasz;
	size_t			headsz;
	size_t			totalsz;
	int			canon = gp->hetero != NO_CONV;
	struct rqmkdent		rqmkdent;

	*vpp = NULLVP;
	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	rfcl_fsinfo.fsivop_other++;
	if (vcver < RFS2DOT0) {
		return du_mkdir(dvp, nm, vap, vpp, crp);
	}
	/*
	 * Get a send descriptor to hold the remote reference.
	 */
	if ((error = sndd_create(TRUE, &giftsdp)) != 0) {
		return error;
	}

	/* create an rd on which to receive the response */
	if ((error = rcvd_create(TRUE, RDSPECIFIC, &rdp)) != 0) {
		sndd_free(&giftsdp);
		return error;
	}
	rdp->rd_sdp = chansdp;
	/*
	 * Calculate datasz, up datasz to worst case
	 * in heterogeneous environment.
	 */
	headsz = RF_MIN_REQ(vcver);
	datasz = gp->hetero != NO_CONV ? sizeof(struct rqmkdent) + MKDENT_XP :
	  sizeof(struct rqmkdent);
	totalsz = headsz + datasz;
	for (ntries = nacked = 1; ntries < RFCL_MAXTRIES && nacked; ntries++) {
		register struct rqmkdent	*rqdp;
		caddr_t				data;

		if ((error = rf_allocmsg(headsz, datasz, BPRI_LO, TRUE,
		  NULLCADDR, NULLFRP, &bp)) != 0) {
			break;
		}
		rfcl_reqsetup(bp, chansdp, crp, RFMKDIR, ULIMIT);
		data = rf_msgdata(bp, headsz);
		rqdp = canon ? &rqmkdent : (struct rqmkdent *)data;
		(void)strcpy(rqdp->nm, nm);
		vtorf_attr(&rqdp->attr, vap);
		if (canon) {
			datasz = rf_tcanon(MKDENT_FMT,
			  (caddr_t)&rqmkdent, data);
		}
		error = rfcl_xac(&bp, totalsz, rdp, vcver, FALSE, &nacked);
	}
	rcvd_free(&rdp);
	if (!error) {
		if ((error = RF_RESP(bp)->rp_errno) == 0) {
			register rf_message_t *msg = RF_MSG(bp);

			if (!(msg->m_stat & RF_GIFT)) {
			gdp_discon("rf_mkdir_pass file reference",
			  QPTOGP((queue_t *)msg->m_queue));
			error = EPROTO;
		} else {
				sndd_set(giftsdp, (queue_t *)msg->m_queue,
				  msg->m_giftid);
				if ((error = rfcl_findsndd(&giftsdp, crp, bp,
				  dvp->v_vfsp)) == 0) {
					*vpp = SDTOV(giftsdp);
				}
			}
		} else {
			sndd_free(&giftsdp);
		}
		rf_freemsg(bp);
	} else {
		sndd_free(&giftsdp);
	}
	return error;
}

/*
 * For RFS open, forward file mode and vp to server.
 */
STATIC int
rf_open(vpp, filemode, crp)
	vnode_t		**vpp;
	int		filemode;
	cred_t		*crp;
{
	register int	error;
	mblk_t		*bp = NULL;
	union rq_arg	rqarg = init_rq_arg;
	sndd_t		*chansdp = VTOSD(*vpp);
	sndd_t		*giftsdp = NULL;	/* server may switch vnodes */

	rfcl_fsinfo.fsivop_open++;
	if (QPTOGP(chansdp->sd_queue)->version < RFS2DOT0) {
		return du_open(vpp, filemode, crp);
	}
	if (error = sndd_create(TRUE, &giftsdp)) {
		return error;
	}
	rqarg.rqopen.fmode = filemode;
	if ((error = rfcl_op(chansdp, crp, RFOPEN, &rqarg, &bp, TRUE)) == 0) {
		rf_message_t	*msg;

		if (!(error = RF_RESP(bp)->rp_errno) &&
		  (msg = RF_MSG(bp))->m_stat & RF_GIFT) {
			/*
			 * Server switched vnodes on us.
			 * We received a new gift to install; after handling
			 * that, we toss the vnode we started with.
			 */
			vfs_t	*vfsp = SDTOV(chansdp)->v_vfsp;

			sndd_set(giftsdp, (queue_t *)msg->m_queue,
			  msg->m_giftid);
			if (!(error = rfcl_findsndd(&giftsdp, crp, bp, vfsp))) {
				*vpp = SDTOV(giftsdp);
			} else {
				*vpp = NULLVP;
			}
			/* The server held the vnode denoted by chansdp so
			 * we would know that we aren't dealing with a dangling
			 * reference.  We release the reference acquired
			 * before this open.
			 */
			VN_RELE(SDTOV(chansdp));
		} else {
			sndd_free(&giftsdp);
		}
		rf_freemsg(bp);
	} else {
		sndd_free(&giftsdp);
	}
	return error;
}

/* ARGSUSED */
STATIC int
rf_readdir(vp, uiop, crp, eofp)
	vnode_t			*vp;
	uio_t			*uiop;
	cred_t			*crp;
	int			*eofp;
{
	register int		error;
	int			nacked;
	int			ntries;
	mblk_t			*bp = NULL;
	register rf_request_t	*reqp;
	rcvd_t			*rdp = NULL;
	sndd_t			*replysdp = NULL;
	register 		sndd_t *chansdp = VTOSD(vp);
	register int		vcver = QPTOGP(chansdp->sd_queue)->version;
	ulong 			oresid = uiop->uio_resid;

	rfcl_fsinfo.fsivop_readdir++;
	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	/* create an rd on which to receive the response */
	if ((error = rcvd_create(TRUE, RDSPECIFIC, &rdp)) != 0) {
		goto out;
	}
	rdp->rd_sdp = chansdp;
	/* create an sd for copysync responses, if any */
	if ((error = sndd_create(TRUE, &replysdp)) != 0) {
		goto out;
	}

	VOP_RWUNLOCK(vp);
	for (ntries = nacked = 1; ntries < RFCL_MAXTRIES && nacked; ntries++) {
		if ((error = rf_allocmsg(RF_MIN_REQ(vcver), (size_t)0, BPRI_LO,
		  TRUE, NULLCADDR, NULLFRP, &bp)) != 0) {
			break;
		}
		rfcl_reqsetup(bp, chansdp, crp, RFREADDIR,
		  (ulong)uiop->uio_limit);
		reqp = RF_REQ(bp);
		reqp->rq_xfer.offset = uiop->uio_offset;
		reqp->rq_xfer.count = uiop->uio_resid;
		reqp->rq_xfer.base = (long)uiop->uio_iov->iov_base;
		error = rfcl_xac(&bp, RF_MIN_REQ(vcver), rdp, vcver, FALSE,
		  &nacked);
	}
	if (!error) {
		error = rf_readdir_resp(rdp, bp, uiop, replysdp, eofp);
	}
	VOP_RWLOCK(vp);
out:
	rcvd_free(&rdp);
	sndd_free(&replysdp);
	rfcl_fsinfo.fsireadch += oresid - uiop->uio_resid;
	return error;
}

/*
 * Handles any data movement using the rfcl_readmove routine and updates the
 * uio structure.  Always frees bp, sometimes reuses that variable.  Returns
 * any error encountered, including rp_errno from the bracketing response.
 */
STATIC int
rf_readdir_resp(rdp, bp, uiop, replysdp, eofp)
	rcvd_t			*rdp;
	mblk_t			*bp;
	uio_t			*uiop;
	sndd_t			*replysdp;
	int			*eofp;
{
	register int		error;
	int			uio_error = 0;	/* don't overwrite error */
	register rf_response_t	*rp = RF_RESP(bp);
	register off_t		noffset = uiop->uio_offset;
	gdp_t			*gp = QPTOGP(rdp->rd_sdp->sd_queue);
	size_t			hdrsz = RF_MIN_RESP(gp->version);
	register int		rp_op;

	if ((error = rp->rp_errno) != 0) {
		goto out;
	}
	/* break for final response or for EPROTO */
	for (;;) {
		/*
		 * Protocol history - rp_count and rp_offset are meaningful
		 * in the RFREADDIR response iff rp->rp_rval > 0.  In
		 * intermediate RFCOPYOUT responses, they are always defined.
		 */

		if ((rp_op = RF_COM(bp)->co_opcode) != RFREADDIR &&
		  rp_op != RFCOPYOUT) {
			gdp_discon("rf_readdir_resp bad header", gp);
			error = EPROTO;
			goto out;
		}

		if (rp_op == RFCOPYOUT || rp->rp_rval > 0) {

			/* Remember current rp_offset to update uio_offset. */

			noffset = rp->rp_offset;

			if (gp->hetero != NO_CONV &&
			  (error = rf_readdir_fcanon(bp, hdrsz, gp, &uio_error))
			   != 0) {
				goto out;
			}

			/* Possibly updated by rf_readdir_fcanon */

			rp = RF_RESP(bp);

			if (error = rfcl_readmove(&bp, uiop, replysdp,
			  &uio_error)) {
				goto out;
			}
		}
		if (rp_op == RFREADDIR) {
			/*
			 * We are forced to believe that the returned offset is
			 * correct, reset uio_offset accordingly.
			 */
			uiop->uio_offset = noffset;

			/*
			 * signals and rp_errno are meaningful only in the final
			 * response, distinguished here by the RFREADDIR opcode
			 */
			if (!rf_sigisempty(rp, gp->version)) {
				rf_postrpsigs(rp, gp->version, u.u_procp);
			}
			if ((error = rp->rp_errno) == 0 &&
			  gp->version > RFS1DOT0) {
				*eofp = rp->rp_xfer.eof;
			}
			goto out;
		}

		/* receive a new response message and verify it is error free */
		if ((error = rf_rcvmsg(rdp, &bp)) != 0) {
			goto out;
		}
		rp = RF_RESP(bp);
	}
out:
	rf_freemsg(bp);
	return error ? error : uio_error;
}

STATIC int
rf_readlink(vp, uiop, crp)
	vnode_t		*vp;
	uio_t		*uiop;
	cred_t		*crp;
{
	register int	error;
	mblk_t		*bp = NULL;
	register 	sndd_t *chansdp = VTOSD(vp);
	register gdp_t	*gp = QPTOGP(chansdp->sd_queue);
	register int	vcver = gp->version;
	ulong		oresid = uiop->uio_resid;
	union rq_arg	rqarg = init_rq_arg;
	int		uio_error = 0;

	/* If remote is older than RFS2DOT0, symlink is not supported. */
	if (vcver < RFS2DOT0) {
		return ENOSYS;
	}
	rfcl_fsinfo.fsivop_other++;
	/*
	 * Assign request header args.
	 */
	rqarg.rqxfer.offset = uiop->uio_offset;
	rqarg.rqxfer.count = uiop->uio_resid;
	rqarg.rqxfer.base = (long)uiop->uio_iov->iov_base;
	if ((error = rfcl_op(chansdp, crp, RFREADLINK, &rqarg, &bp,
	  TRUE)) == 0 && (error = RF_RESP(bp)->rp_errno) == 0) {
		if (RF_COM(bp)->co_opcode == RFREADLINK) {
			error = rfcl_readmove(&bp, uiop, (sndd_t *)NULL,
			  &uio_error);
		} else {
			gdp_discon("rf_rdlink bad response",
			  QPTOGP(chansdp->sd_queue));
			error = EPROTO;
			rf_freemsg(bp);
		}
	}
	rfcl_fsinfo.fsireadch += oresid - uiop->uio_resid;
	return error ? error : uio_error;
}


/*
 * VOP_READ
 */
/* ARGSUSED */
STATIC int
rf_read(vp, uiop, f, crp)
	vnode_t		*vp;
	uio_t		*uiop;
	int		f;
	cred_t		*crp;
{
	register sndd_t	*sdp = VTOSD(vp);
	rfc_ctl_t	ctl = RFC_INCACHE;
	int		error = 0;
	ulong		oresid = uiop->uio_resid;

	rfcl_fsinfo.fsivop_read++;
	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	VOP_RWUNLOCK(vp);
	for (;;) {
		if (sdp->sd_stat & SDCACHE) {
			if (sdp->sd_crwlock.writer) {
				sdp->sd_crwlock.want = TRUE;
				if (sleep((caddr_t)&sdp->sd_crwlock,
				  PREMOTE | PCATCH)) {
					error = EINTR;
					break;
				}
				continue;
			}
			sdp->sd_crwlock.nreaders++;
			error = rf_read_cache(vp, uiop, crp, &ctl);
			if (--sdp->sd_crwlock.nreaders == 0 &&
			  sdp->sd_crwlock.want) {
			  	sdp->sd_crwlock.want = FALSE;
				wakeup((caddr_t)&sdp->sd_crwlock);
			}
			/*
			 * !error && RFC_INCACHE == done
			 */
			if (error || ctl == RFC_INCACHE) {
				break;
			} else if (ctl == RFC_RETRY) {
				continue;
			}
		}
		error = rf_read_bypass(vp, uiop, crp);
		break;
	}
	VOP_RWLOCK(vp);
	rfcl_fsinfo.fsireadch += oresid - uiop->uio_resid;
	return error;
}

/* ARGSUSED */
STATIC int
rf_write(vp, uiop, f, crp)
	vnode_t		*vp;
	uio_t		*uiop;
	int		f;
	cred_t		*crp;
{
	/*
	 * Cached writes are simpler than reads.  Reads have to worry
	 * about restarting to avoid data inconsistencies.  Writes
	 * only have to track whether they're allowed to cache a
	 * local data copy at any moment, i.e., whether caching is
	 * enabled for the file, because the data is already present
	 * on the client, and the write is defining the file
	 * contents, so no inconsistency between the file contents
	 * on this client and the server can arise.
	 *
	 * We don't worry about updating sd_size until the final
	 * server response is received.  Readers will not proceed
	 * through the cache while caching is enabled, because
	 * we have a write lock on the sndd.  If caching is disabled
	 * before we get to the server, readers will see the size
	 * of the file on the server, because they won't even look
	 * at the cache.
	 */

	register sndd_t	*sdp = VTOSD(vp);
	int		error = 0;
	size_t		iovsize = sizeof(iovec_t);
	caddr_t		iovp;
	rf_rwa_t	rf_rwa;
	unsigned	oresid = uiop->uio_resid;

	rfcl_fsinfo.fsivop_write++;

	if (RF_SERVER()) {
		return EMULTIHOP;
	}

	VOP_RWUNLOCK(vp);

	if (f & IO_APPEND && vp->v_type == VREG) {
		uiop->uio_offset = VTOSD(vp)->sd_size;
	}
	rf_rwa.uiop = uiop;
	rf_rwa.cached = FALSE;
	rf_rwa.wr_ioflag = f;
	rf_rwa.wr_kern = FALSE;

	sdp->sd_nextr = 0;

	while (sdp->sd_stat & SDCACHE && (sdp->sd_crwlock.writer ||
	  sdp->sd_crwlock.nreaders != 0)) {
		sdp->sd_crwlock.want = TRUE;
		if (sleep((caddr_t)&sdp->sd_crwlock, PREMOTE | PCATCH)) {
			error = EINTR;
			goto rwlock;
		}
	}

	if (sdp->sd_stat & SDCACHE) {

		/*
		 * rf_rwa.cached may be reset (and the sndd lock
		 * released) at a lower level if caching is disabled
		 */

		if ((error = rfcl_uioclone(uiop, &rf_rwa, &iovsize)) != 0) {
			goto rwlock;
		}
		iovp = (caddr_t)rf_rwa.cwruio.uio_iov;
		sdp->sd_crwlock.writer = TRUE;
		rf_rwa.cached = TRUE;

		/*
		 * Suppressing this for unmappable files will prevent failing
		 * in fbread -> as_fault -> rf_getpage.  The penalty is that
		 * we will copy data in twice.
		 */

		rf_rwa.wr_kern = !(vp->v_flag & VNOMAP);
	}

	error = rfcl_write_op(VTOSD(vp), crp, RFWRITE, &rf_rwa);

	if (iovsize > sizeof(iovec_t)) {
		kmem_free(iovp, iovsize);
	}
	if (rf_rwa.cached) {
		sdp->sd_crwlock.writer = FALSE;
		if (sdp->sd_crwlock.want) {
		  	sdp->sd_crwlock.want = FALSE;
			wakeup((caddr_t)&sdp->sd_crwlock);
		}
	}

rwlock:
	VOP_RWLOCK(vp);
	rfcl_fsinfo.fsiwritech += oresid - uiop->uio_resid;
	return error;
}

/*
 * Remove any mapping in the specified range of addresses, and
 * replace it with a new mapping to the denoted file.
 * Args
 *	NOTE:  there are currently no alignment restrictions on off, addr, or
 *	len; they are adjusted internally.  It's unclear to what extent this is
 *	a property of the (partly defined) interface, to what extent of the
 *	implementation.
 *
 *	vp
 *		file to be mapped
 *	off
 *		starting offset in the file of the mapping
 *	as
 *		address space in which to establish the mapping
 *	addrp
 *		if MAP_FIXED is set, addrp refers to the virtual address in
 *		as at which to start mapping.  Otherwise, map_addr() is
 *		used to set this address.
 *	len
 *		length of mapping
 *	prot
 *		current PERMISSIONS of the mapping
 *	maxprot
 *		most liberal PERMISSIONS that can be applied to the
 *		mapping.  prot cannot exceed maxprot.
 *	flags
 *		MAP_SHARED | MAP_PRIVATE (required) and possibly
 *		MAP_FIXED | _MAP_NEW | MAP_NORESERVE | MAP_RENAME |
 *		MAP_DUP (others?)
 *	cred
 *		the obvious, probably a don't-care term for all
 *		but stateless file servers.
 * Returns 0 for success, nonzero errno for failure.
 */
STATIC int
rf_map(vp, off, as, addrp, len, prot, maxprot, flags, cred)
	vnode_t			*vp;
	int			off;
	struct as		*as;
	caddr_t			*addrp;
	int			len;
	uint			prot;
	uint			maxprot;
	uint			flags;
	cred_t			*cred;
{
	struct segvn_crargs	crargs;	/* args to segvn_create */

	ASSERT((maxprot & prot) == prot);

	if (RF_SERVER()){
		return EMULTIHOP;
	}
	if (off < 0) {
		return EINVAL;
	}
	if (vp->v_flag & VNOMAP) {
		return ENOSYS;
	}
	if (off + len > ptob(btopr(VTOSD(vp)->sd_size))) {
		/* Don't allow a mapping beyond the last page in the file. */
		return ENXIO;
	}

	crargs.vp = vp;
	crargs.offset = off;
	crargs.type = (u_char)(flags & MAP_TYPE);
	crargs.prot = (u_char)prot;
	crargs.maxprot = (u_char)maxprot;
	crargs.cred = cred;
	crargs.amp = NULL;
	if (!(flags & MAP_FIXED)) {
		(void)map_addr(addrp, len, (off_t)off, 1);
		if (!*addrp) {
			return ENOMEM;
		}
	} else {
		/* User specified address - remove any previous mappings. */
		(void)as_unmap(as, *addrp, len);
	}

	/* as_map takes care of aligning its args. */

	return as_map(as, *addrp, len, segvn_create, (caddr_t)&crargs);
}

/*
 * Add new mapping to mapcnt.
 * Args
 *	NOTE:  there are currently no alignment restrictions on off, addr, or
 *	len; they are adjusted internally.  It's unclear to what extent this is
 *	a property of the (undefined) interface, to what extent of the
 *	implementation.
 *
 *	vp
 *		file to be mapped
 *	off
 *		starting offset in the file of the mapping
 *	as
 *		address space in which to establish the mapping
 *	addrp
 *		if MAP_FIXED is set, addrp refers to the virtual address in
 *		as at which to start mapping.  Otherwise, map_addr() is
 *		used to set this address.
 *	len
 *		length of mapping
 *	prot
 *		current PERMISSIONS of the mapping
 *	maxprot
 *		most liberal PERMISSIONS that can be applied to the
 *		mapping.  prot cannot exceed maxprot.
 *	flags
 *		MAP_SHARED | MAP_PRIVATE (required) and possibly
 *		MAP_FIXED | _MAP_NEW | MAP_NORESERVE | MAP_RENAME |
 *		MAP_DUP (others?)
 *	cred
 *		the obvious, probably a don't-care term for all
 *		but stateless file servers.
 * Returns 0 for success, nonzero errno for failure.
 */
/* ARGSUSED */
STATIC int
rf_addmap(vp, off, as, addrp, len, prot, maxprot, flags, cred)
	vnode_t		*vp;
	int		off;
	struct as	*as;
	caddr_t		*addrp;
	uint		len;
	uint		prot;
	uint		maxprot;
	uint		flags;
	cred_t		*cred;
{
	register sndd_t	*sdp = VTOSD(vp);
	int		error = 0;
	mblk_t		*bp = NULL;

	if (vp->v_flag & VNOMAP) {
		return ENOSYS;
	}
	if (!sdp->sd_mapcnt && (error = rfcl_op(sdp, cred, RFMAP,
	  &init_rq_arg, &bp, TRUE)) == 0) {
		error = RF_RESP(bp)->rp_errno;
		rf_freemsg(bp);
		bp = NULL;
	}
	if (!error) {
		sdp->sd_mapcnt += btopr(len);
	}
	return error;
}

/*
 * Decrement mapcnt to correspond to the removal of any mappings to
 * the denoted file in the specified range.
 * Args
 *	NOTE:  there are currently no alignment restrictions on off, addr, or
 *	len; they are adjusted internally.  It's unclear to what extent this is
 *	a property of the (partly defined) interface, to what extent of the
 *	implementation.
 *
 *	vp
 *		file to be unmapped
 *	off
 *		starting offset in the file of the area to be unmapped
 *	as
 *		address space of the mapping
 *	addrp
 *		virtual address at which the mapping starts.
 *	len
 *		length of mapping
 *	prot
 *		current PERMISSIONS of the mapping
 *	maxprot
 *		most liberal PERMISSIONS that can be applied to the
 *		mapping.  prot cannot exceed maxprot.
 *	flags
 *		MAP_SHARED | MAP_PRIVATE (required) and possibly
 *		MAP_FIXED | _MAP_NEW | MAP_NORESERVE | MAP_RENAME |
 *		MAP_DUP (others?)
 *		ignored here.
 *	cred
 *		the obvious, probably a don't-care term for all
 *		but stateless file servers.
 * Returns 0 for success, nonzero errno for failure.
 */
/* ARGSUSED */
STATIC int
rf_delmap(vp, off, as, addrp, len, prot, maxprot, flags, cred)
	vnode_t		*vp;
	uint		off;
	struct as	*as;
	caddr_t		*addrp;
	uint		len;
	uint		prot;
	uint		maxprot;
	uint		flags;
	cred_t		*cred;
{
	register sndd_t *sdp = VTOSD(vp);

	if (vp->v_flag & VNOMAP) {
		return ENOSYS;
	}
	sdp->sd_mapcnt -= btopr(len);
	ASSERT(sdp->sd_mapcnt >= 0);
	if (!sdp->sd_mapcnt) {
		mblk_t *bp = NULL;

		if (!(rfcl_op(sdp, cred, RFUNMAP, &init_rq_arg, &bp, FALSE))) {
			rf_freemsg(bp);
		}
	}
	return 0;
}

/*
 * If pl is non-NULL, fill *pl with pointers to at least all the pages
 * from [off..off+len) in given file.  If pl is NULL, just bring the pages
 * in.  Assumes it is called with vnode unlocked.
 *
 * Args
 *	vp
 *		vnode backing the faulted pages.  If the faulting
 *		range contains any non-anonymous pages, this vnode
 *		is the one for the original backing file.
 *	off
 *		page-aligned file offset corresponding to the
 *		faulting address.
 *	len
 *		number of bytes we are faulting, equal to an integral
 *		multiple of the number of bytes per page.  Usually 1 page,
 *		except if we're being called explicitly, to lock down a
 *		range of pages, e.g.
 *	protp
 *		denotes out param updated with PERMISSIONS for faulted
 *		pages.  "prot" is really a misnomer, but we use that name
 *		to stay in sync w/ the rest of the system.  The updated
 *		value is really advice as to what the vnode object manager
 *		is willing to support.  For instance, it would be possible
 *		to delay allocation of disk blocks by denying write
 *		permissions on a page corresponding to a hole in the file.
 *	pl
 *		if non-NULL, denotes out param updated with pointers to pages,
 *		terminated by a NULL.  Extra pages may be returned by linking
 *		them to those in pl.  Pages in pl are held.  NULL pl implies
 *		asynchronous fault-ahead (which we do synchronously).
 *		(TO DO:  asynchronous page-ahead.)
 *
 *	plsz
 *		caller guarantees that pl, if non-NULL has room for at least
 *		enough pages for plsz bytes of memory.
 *	seg
 *		segment structure containing the faulting address.
 *	addr
 *		page-aligned fault address
 *	rw
 *		access attempted at fault time
 *	cred
 *		credentials structure for the current process, or
 *		NULL if credentials are unknown.
 *
 * Returns 0 for success, nonzero errno for failure.
 */
STATIC int
rf_getpage(vp, off, len, protp, pl, plsz, seg, addr, rw, cred)
	vnode_t		*vp;
	size_t		off;
	size_t		len;
	uint		*protp;
	page_t		*pl[];
	size_t		plsz;
	struct seg	*seg;
	caddr_t		addr;
	enum seg_rw	rw;
	cred_t		*cred;
{
	int		error;

	ASSERT(len >= PAGESIZE && !(len % PAGESIZE));
	ASSERT(!((ulong)addr % PAGESIZE));
	ASSERT(!(off % PAGESIZE));

	rfcl_fsinfo.fsivop_getpage++;
	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	if (vp->v_flag & VNOMAP) {
		return ENOSYS;
	}
	if (protp) {
		*protp = PROT_ALL;
	}
	if (len == PAGESIZE) {
		error = rf_getapage(vp, off, protp, pl, plsz, seg, addr,
					rw, cred);
	} else {
		error = pvn_getpages(rf_getapage, vp, off, len, protp, pl,
					plsz, seg, addr, rw, cred);
	}
	return error;
}

/*
 * Flush to the backing vnode all dirtypages in the range of file
 * offset [off...off + len), possibly invalidating all pages in the
 * range.  Asumes it is called with vnode unlocked.  If the vnode has
 * no references, returns immediately, else inflates the reference
 * count until the return.
 * Args
 *	NOTE:  there are currently no alignment restrictions on off or len;
 *	they are adjusted internally.  It's unclear to what extent this is
 *	a property of the (undocumented) interface, to what extent of the
 *	implementation.
 *
 *	vp
 *		file backing the pages
 *	off
 *		file offset of start of range to flush
 *	len
 *		length of the range to flush.  len == 0 imples from
 *		off through EOF.
 *	flags
 *		composed of {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED, B_FORCE}
 *
 *	cred
 *		credentials structure for the operation, or NULL if
 *		unknown.
 * FYI:
 * The normal cases should be len == 0 && off == 0 (entire page list),
 * len == MAXBSIZE (from segmap_release actions, because each map is
 * for MAXBSIZE bytes), and len == PAGESIZE (from pageout).
 *
 * Returns 0 for success, nonzero errno for failure.
 */
STATIC int
rf_putpage(vp, off, len, flags, cred)
	register vnode_t	*vp;
	uint			off;
	uint			len;
	int			flags;
	cred_t			*cred;
{
	/*
	 * TO DO:  do we need to lock out the pageout daemon?
	 */
	register sndd_t		*sdp = VTOSD(vp);
						/* remote file reference */
	page_t			*dirty;		/* list of dirty pages */
	uint			bsize;		/* size of chunks to write */
	int			error;

	rfcl_fsinfo.fsivop_putpage++;
	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	if (vp->v_flag & VNOMAP) {
		return ENOSYS;
	}
	if (!vp->v_pages) {
		return 0;
	}
	if (off >= sdp->sd_size) {
		/*
		 * TO DO:  why is this not an error?  Also, ufs and NFS
		 * checked this only after checking len != 0, then set
		 * dirty = NULL, so they would fall through to the end of
		 * the function.  Why?
		 */
		return 0;
	}
	/*
	 * If vp->v_count is 0 here, the vnode is already inactive, return.
	 */
	if (!vp->v_count) {
		return 0;
	}
	VN_HOLD(vp);

	/*
	 * Set minimum IO granularity to convenient integral multiple of
	 * PAGESIZE.
	 */

	bsize = MAX(PAGESIZE, vp->v_vfsp->vfs_bsize);
again:
	sdp->sd_nextr = 0;

	if (!len) {
		/*
		 * Flushing [off...EOF).  Search entire vp page list for pages
		 * >= off.
		 */
		SDLOCK(sdp)
		dirty = pvn_vplist_dirty(vp, off, flags);
		SDUNLOCK(sdp);
	} else {
		/*
		 * Flush [off...off + len).
		 * As an optimization, to avoid searching a potentially long
		 * page list, do a range for [off...off + len) using page_find
		 * (called from pvn_range dirty).  We set end points on
		 * bsize boundaries for klustering.
		 */
		/*
		 * TO DO:  Check for illegal offsets on entry into this
		 * routine, and do reasonable klustering below.
		 */
		uint	fsize;	/* file size rounded up to PAGESIZE boundary */
		uint	eoff;	/* upper bound of range needed */
		uint	offlo;	/* off rounded down to bsize boundary */
		uint	offhi;	/* eoff rounded up to bsize boundary */

		fsize = ptob(btopr(sdp->sd_size));
		eoff = MIN(off + len, fsize);
		offlo = off & ~(bsize - 1);
		offhi = (eoff + bsize) & ~(bsize - 1);

		dirty = pvn_range_dirty(vp, off, eoff, offlo, offhi, flags);
	}
	/*
	 * Now dirty will have a list of kept dirty pages marked for write
	 * back.
	 *
	 * We delay this check until now because it is not an error to try
	 * to flush pages without first checking to see if there are any
	 * dirty pages and the file system is mounted read-only.  We don't
	 * know until now if there are dirty pages.
	 */
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY && dirty) {
		error = EROFS;
	} else {
		error = rf_pushpages(&dirty, vp, flags, bsize, cred);
	}
	/*
	 * The else part below is taken when we're doing synchronous
	 * invalidation of the whole file, and we didn't get all the pages
	 * on our last call to pvn_vplist_dirty because of pvn_getdirty
	 * accounting for some races, or if someone establishes new
	 * mappings to the file.
	 */
	if (error) {
		if (dirty) {
			pvn_fail(dirty, B_WRITE | flags);
		}
	} else if ((flags & (B_INVAL | B_ASYNC)) == B_INVAL &&
	  !len && !off && vp->v_pages) {
		goto again;
	}
out:
	VN_RELE(vp);
	return error;
}

/*
 *  VOP_READ bypassing the page cache
 */
STATIC int
rf_read_bypass(vp, uiop, crp)
	vnode_t		*vp;
	uio_t		*uiop;
	cred_t		*crp;
{
	union rq_arg	rqarg = init_rq_arg;
	rf_rwa_t	rfrwa;

	rqarg.rqxfer.offset = uiop->uio_offset;
	rqarg.rqxfer.count = uiop->uio_resid;
	rfrwa.uiop = uiop;
	rfrwa.cached = FALSE;
	return rfcl_read_op(VTOSD(vp), crp, RFREAD, &rqarg, &rfrwa);
}

/*
 * VOP_READ through the page cache.  *ctlp is set to RFC_INCACHE for a
 * succesful read, to RFC_RETRY to restart the read through the cache.
 * It is undefined in error cases.
 */
STATIC int
rf_read_cache(vp, uiop, crp, ctlp)
	vnode_t		*vp;
	uio_t		*uiop;
	cred_t		*crp;
	rfc_ctl_t	*ctlp;
{
	/* Set offset and length to page granularity and search the cache. */

	sndd_t		*sdp = VTOSD(vp);
	off_t		cache_off;
	off_t		eof;
	size_t		cache_resid;
	size_t		ucache_resid;
	size_t		fresid;
	int		error = 0;
	rf_rwa_t	rf_rwa;
	union rq_arg	rqarg = init_rq_arg;

	if (uiop->uio_offset >= sdp->sd_size) {
		return 0;
	}

	*ctlp = RFC_INCACHE;
	rf_rwa.rd_pfx = NULL;
	rf_rwa.rd_infix = NULL;
	rf_rwa.rd_sfx = NULL;
	cache_off = uiop->uio_offset & PAGEMASK;
	eof = MIN(uiop->uio_offset + uiop->uio_resid, sdp->sd_size);
	ucache_resid = ptob(btopr(eof - cache_off));
	fresid = sdp->sd_size - cache_off;

	if (cache_off == (sdp->sd_nextr & PAGEMASK) && ucache_resid < fresid) {
		size_t	bsize = vp->v_vfsp->vfs_bsize;

		/* sequential read, not to eof */

		cache_resid = ucache_resid + PAGESIZE;
		if (cache_resid < bsize && cache_resid < fresid) {
			cache_resid = MIN(bsize, fresid);
		}
		sdp->sd_nextr = uiop->uio_offset + uiop->uio_resid;
		if (sdp->sd_nextr & PAGEOFFSET == PAGEOFFSET) {
			sdp->sd_nextr++;
		}
	} else {
		cache_resid = ucache_resid;
		sdp->sd_nextr = 0;
	}

	/*
	 * Search the page cache for resident pages.  In non_error case,
	 * rf_rwa.rd_pfx will point to any resident prefix, rf_rwa.rd_sfx
	 * to any resident suffix.
	 *
	 * It's possible that the cache will be invalidated after here, but
	 * before we get to the server to lock the vnode.  We catch that in
	 * rfc_readmove.
	 */

	if (error = rfc_pagelist(vp, cache_off, ucache_resid,
	  &rf_rwa.rd_pfx, &rf_rwa.rd_sfx)) {
		return error;
	}

	if (rf_rwa.rd_pfx &&
	  rf_rwa.rd_pfx->p_prev->p_offset ==
	  cache_off + ucache_resid - PAGESIZE) {

		/*
		 * Last page in resident prefix == last page needed ==
		 * cache hit.
		 *
		 * Move the data, clean up, and go home.  Even if
		 * the cache is disabled while we're moving data,
		 * we have provided the caller with a consistent
		 * snapshot of the cache.
		 */

		error = rfc_plmove(&rf_rwa.rd_pfx, uiop, eof);
	} else {

		/*
		 * Cache miss.
		 * Set up to go remote.  First step is finding the
		 * nonresident interval [firstnonres...pastnonres).
		 */

		if (rf_rwa.rd_pfx) {
			rf_rwa.rd_firstnrb =
			  rf_rwa.rd_pfx->p_prev->p_offset + PAGESIZE;
		} else {
			rf_rwa.rd_firstnrb = cache_off;
		}
		if (rf_rwa.rd_sfx) {
			rf_rwa.rd_endnrb = rf_rwa.rd_sfx->p_offset;
		} else {
			rf_rwa.rd_endnrb = cache_off + cache_resid;
		}
		rf_rwa.uiop = uiop;
		rf_rwa.cached = TRUE;
		rf_rwa.rd_nextnrb = rf_rwa.rd_firstnrb;
		rf_rwa.rd_ctlp = ctlp;
		rf_rwa.rd_sdsize = sdp->sd_size;
		rf_rwa.rd_startoff = cache_off;

		rqarg.rqxfer.offset = rf_rwa.rd_firstnrb;
		rqarg.rqxfer.count = rf_rwa.rd_endnrb - rf_rwa.rd_firstnrb;

		error = rfcl_read_op(sdp, crp, RFREAD, &rqarg, &rf_rwa);
	}

	/*
	 * Give up any pages still held.  We can get into these branches
	 * if we had an error, if a cache disable message beat the
	 * first response to a remote request, or if the read was satisfied
	 * without entirely consuming the last page in the page list.
	 */

	if (rf_rwa.rd_pfx) {
		rfc_plrele(&rf_rwa.rd_pfx);
	}
	if (rf_rwa.rd_infix) {

		/*
		 * Pages are unlocked and released in non-error cases.
		 * In any event, there should be only one page that
		 * we're filling at any one time.
		 */

		ASSERT(error);
		ASSERT(rf_rwa.rd_infix->p_prev == rf_rwa.rd_infix);
		rfc_pageunlock(rf_rwa.rd_infix);
		rfc_plrele(&rf_rwa.rd_sfx);	/* for page_sub */
	}
	if (rf_rwa.rd_sfx) {
		rfc_plrele(&rf_rwa.rd_sfx);
	}
	return error;
}

/*
 * If pl is non-NULL, fill *pl with a pointer to at least the page starting
 * at file offset off in vp.  May also return additional pages in *pl.  If
 * pl is NULL, just bring the page(s) in.  Assumes it is called with vnode
 * locked.
 *
 * Args
 *	vp
 *		vnode backing the fault page.
 *	off
 *		page-aligned file offset corresponding to the
 *		wanted page.
 *	protp
 *		denotes in/out param assumed to be initialized to PROT_ALL,
 *		updated by removing permissions for returned pages.  We
 *		don't use it here.  "prot" is really a misnomer, but we use
 *		it to stay in sync with the rest of the system.
 *	pl
 *		if non-NULL, denotes out param updated with pointers to pages,
 *		terminated by a NULL.  Extra pages may be returned by linking
 *		them to those in pl.  If so, we will not have done a PAGE_HOLD
 *		on those extra pages.  NULL pl implies asynchronous fault-ahead
 *		(which we do synchronously).  (TO DO: asynchronous fault-ahead.)
 *	plsz
 *		caller guarantees that pl, if non-NULL has room for at least
 *		enough pages for plsz bytes of memory.
 *	seg
 *		segment structure containing the faulting address.
 *	addr
 *		aligned address of wanted page
 *	rw
 *		access attempted at fault time
 *	cred
 *		credentials structure for the current process, or
 *		NULL if credentials are unknown.
 *
 * Returns 0 for success, nonzero errno for failure.
 */
/* ARGSUSED */
STATIC int
rf_getapage(vp, off, protp, pl, plsz, seg, addr, rw, cred)
	register vnode_t	*vp;
	register uint		off;
	uint			*protp;
	register page_t		*pl[];
	uint			plsz;
	struct seg		*seg;
	caddr_t			addr;
	enum seg_rw		rw;
	cred_t			*cred;
{
	register sndd_t		*sdp;
	register u_int		bsize;	/* file sys block size */
	struct buf		*bp;
	page_t			*pp;
	page_t			*pp2;
	page_t			**ppp;
	page_t			*pagefound;
	daddr_t			lbn;
	off_t			io_off;
	size_t			io_len;
	size_t			blkoff;
	off_t			blksize;
	int			err;

	sdp = VTOSD(vp);

	bsize = vp->v_vfsp->vfs_bsize;

reread:
	err = 0;
	lbn = off / bsize;
	blkoff = lbn * bsize;

	if ((pagefound = page_find(vp, off)) == NULL) {
		int	resid;		/* detect fault beyond EOF */

		/*
		 * Need to go to server to get a block
		 */

		if (blkoff < sdp->sd_size && blkoff + bsize > sdp->sd_size) {
			/*
			 * If less than a block left in
			 * file read less than a block.
			 */
			if (sdp->sd_size <= off) {
				/*
				 * Trying to access beyond EOF,
				 * set up to get at least one page.
				 */
				blksize = off + PAGESIZE - blkoff;
			} else {
				blksize = sdp->sd_size - blkoff;
			}
		} else {
			blksize = bsize;
		}

		pp = pvn_kluster(vp, off, seg, addr, &io_off, &io_len,
		    blkoff, blksize, 0);

		ASSERT(pp);

		if (pl) {
			register int	sz;

			if (plsz >= io_len) {
				/*
				 * Everything fits, set up to load
				 * up and hold all the pages.
				 */
				pp2 = pp;
				sz = io_len;
			} else {
				/*
				 * Set up to load plsz worth
				 * starting at the needed page.
				 */
				for (pp2 = pp; pp2->p_offset != off;
				    pp2 = pp2->p_next) {
					ASSERT(pp2->p_next->p_offset !=
					    pp->p_offset);
				}
				sz = plsz;
			}

			ppp = pl;
			do {
				PAGE_HOLD(pp2);
				*ppp++ = pp2;
				pp2 = pp2->p_next;
				sz -= PAGESIZE;
			} while (sz > 0);
			*ppp = NULL;		/* terminate list */
		}

		/*
		 * Now round the request size up to page boundaries.
		 * This insures that the entire page will be
		 * initialized to zeroes if EOF is encountered.
		 */

		io_len = ptob(btopr(io_len));

		/* async later */
		bp = pageio_setup(pp, io_len, vp, B_READ);

		bp->b_blkno = RF_OFFTOBLK(io_off);
		bp->b_dev = 0;
		bp->b_edev = 0;
		bp_mapin(bp);

		err = rfcl_strategy(bp, cred, &resid);	/* trashes bp */
		bp = NULL;

		if (!err && resid == io_len && sdp->sd_size <= off) {

			/*
			 * Faulting beyond EOF.  We DO NOT implement
			 * VOP_READ and VOP_WRITE with mapped I/O.
			 * (Otherwise this could be an innocent fault.)
			 */

			ASSERT(seg != segkmap);

			err = EFAULT;
		}

		if (!err) {
			/* interaction with read - async faultahead later */
			sdp->sd_nextr = io_off + io_len;
			vminfo.v_pgin++;
			vminfo.v_pgpgin += btopr(io_len);
		}

	} else {

		int	s;

		/*
		 * We need to be careful here because if the page was
		 * previously on the free list, we might have already
		 * lost it at interrupt level.
		 */
		s = splvm();
		if (pagefound->p_vnode == vp && pagefound->p_offset == off &&
		  (pagefound->p_intrans || pagefound->p_free)) {

			/*
			 * If the page is intransit or if
			 * it is on the free list call page_lookup
			 * to try to wait for / reclaim the page.
			 */

			pagefound = page_lookup(vp, off);
		}

		(void) splx(s);
		if (!pagefound || pagefound->p_offset != off ||
		    pagefound->p_vnode != vp || pagefound->p_gone) {
			goto reread;
		}

		if (pl) {
			PAGE_HOLD(pagefound);
			pl[0] = pagefound;
			pl[1] = NULL;
			sdp->sd_nextr = off + PAGESIZE;
		}

	}

	if (err && pl) {
		for (ppp = pl; *ppp; *ppp++ = NULL) {
			PAGE_RELE(*ppp);
		}
	}

	return err;
}

/*
 * Push pages on *ppp out to file denoted by vp.
 * Args
 *	ppp
 *		points to list of pages to be written out.  Updated to
 *		point to NULL list in normal case, otherwise points to
 *		list with unwritten pages on return.
 *	vp
 *		file backing the pages.
 *	flags
 *		composed of {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED, B_FORCE}.
 *	bsize
 *		logical block size to write in each IO request, assumed to
 *		be some integral multiple of PAGESIZE.
 *	cred
 *		credentials structure for the current process, or
 *		NULL if credentials are unknown.
 *
 * Returns 0 for success, nonzero errno for failure.
 */
STATIC int
rf_pushpages(ppp, vp, flags, bsize, cred)
	register page_t		**ppp;
	register vnode_t	*vp;
	int			flags;
	uint			bsize;
	cred_t			*cred;
{
	register page_t		*pp;		/* page to go on io list */
	register page_t		*nextpp;	/* next page to consider */
	register int		error = 0;
	register uint		io_off;		/* page-aligned io_list offset
						 */
	register uint		io_len;		/* io_list length */
	page_t			*io_list;	/* pages to write together */
	uint			lbn_off;	/* block-aligned io_list offset
						 */
	sndd_t			*sdp = VTOSD(vp);
	off_t			last_off = sdp->sd_size & PAGEMASK;
	struct buf		*bp;

	while (!error && (pp = *ppp) != NULL) {

		/*
		 * Pull off a contiguous chunk of pages that fit into one
		 * logical block.
		 */

		io_off = pp->p_offset;
		lbn_off = (io_off / bsize) * bsize;
		io_list = pp;
		io_len = PAGESIZE;
		page_sub(ppp, pp);
		nextpp = *ppp;
		while (nextpp &&
		  nextpp->p_offset < lbn_off + bsize &&
		  nextpp->p_offset == io_off + io_len) {
			pp = nextpp;
			page_sub(ppp, pp);
			nextpp = *ppp;
			page_sortadd(&io_list, pp);
			io_len += PAGESIZE;
		}
		if (pp->p_prev->p_offset == last_off) {

			/*
			 * Last page - make sure not to flush beyond EOF.
			 */

			io_len -= PAGESIZE - (sdp->sd_size & PAGEOFFSET);
		}

		/*
		 * ufs and NFS have code to handle the case that the asserted
		 * condition here is false.  That may be needed in ufs, but
		 * appears unnecessary for NFS and RFS, because of the way
		 * bsize is specified.
		 */

		ASSERT(io_off + io_len <= lbn_off + bsize);

		/*
		 * We use the block IO interface because other vnode object
		 * managers do, and we get some services from vm by
		 * adhering to the model.
		 */

		bp = pageio_setup(io_list, io_len, vp, flags | B_WRITE);
		if (!bp) {
			pvn_fail(io_list, B_WRITE | flags);
			error = ENOMEM;
		} else {
			int	sink;

			bp->b_blkno = RF_OFFTOBLK(lbn_off);
			bp->b_dev = 0;
			bp->b_edev = 0;
			bp_mapin(bp);

			/*
			 * In addition to doing the IO, rfcl_strategy will
			 * update b_error, b_flags.  The current protocol
			 * requires uid/gid in each message, so we pass the
			 * credentials.
			 */

			error = rfcl_strategy(bp, cred, &sink);

			/* bp is invalid here */
		}
	}
	return error;
}

/*
 * RFS client VOP_REMOVE
 */
STATIC int
rf_remove(dvp, nm, crp)
	vnode_t		*dvp;		/* parent directory */
	char		*nm;		/* name of entry to remove */
	cred_t		*crp;
{
	register int		error;
	int			nacked;
	int			ntries;
	mblk_t			*bp = NULL;
	rcvd_t			*rdp;
	register sndd_t		*chansdp = VTOSD(dvp);
	register int		vcver;
	register size_t		headsz;
	register size_t		datasz;
	register size_t		totalsz;

	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	rfcl_fsinfo.fsivop_other++;
	vcver = QPTOGP(chansdp->sd_queue)->version;
	if (vcver < RFS2DOT0) {
		return du_remove(dvp, nm, crp);
	}

	/* create an rd on which to receive the response */
	if ((error = rcvd_create(TRUE, RDSPECIFIC, &rdp)) != 0) {
		return error;
	}
	rdp->rd_sdp = chansdp;
	headsz = RF_MIN_REQ(vcver);
	datasz = strlen(nm) + 1;
	totalsz = headsz + datasz;
	for (ntries = nacked = 1; ntries < RFCL_MAXTRIES && nacked; ntries++) {
		if ((error = rf_allocmsg(headsz, datasz, BPRI_LO, TRUE,
		  NULLCADDR, NULLFRP, &bp)) != 0) {
			break;
		}
		rfcl_reqsetup(bp, chansdp, crp, RFREMOVE, ULIMIT);
		(void)strcpy(rf_msgdata(bp, headsz), nm);
		error = rfcl_xac(&bp, totalsz, rdp, vcver, FALSE, &nacked);
	}
	if (!error) {
		error = RF_RESP(bp)->rp_errno;
		rf_freemsg(bp);
	}
	rcvd_free(&rdp);
	return error;
}

/* ARGSUSED */
STATIC int
rf_rename(fdvp, fnm, tdvp, tnm, crp)
	vnode_t		*fdvp;
	char		*fnm;
	vnode_t		*tdvp;
	char		*tnm;
	cred_t		*crp;
{
	register int	error;
	int		nacked;
	int		ntries;
	mblk_t		*bp = NULL;
	rcvd_t		*rdp;
	register sndd_t	*chansdp = VTOSD(tdvp);
	register sndd_t	*fdsdp = VTOSD(fdvp);
	register int	vcver;
	size_t		fnmlen = strlen(fnm) + 1;
	size_t		headsz;
	size_t		datasz;
	size_t		totalsz;

	rfcl_fsinfo.fsivop_other++;
	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	vcver = QPTOGP(chansdp->sd_queue)->version;
	if (vcver < RFS2DOT0) {
		return du_rename(fdvp, fnm, tdvp, tnm, crp);
	}
	if (fdsdp->sd_queue != chansdp->sd_queue) {
		return EXDEV;
	}
	/* create an rd on which to receive the response */
	if ((error = rcvd_create(TRUE, RDSPECIFIC, &rdp)) != 0) {
		return error;
	}
	rdp->rd_sdp = chansdp;
	headsz = RF_MIN_REQ(vcver);
	datasz = fnmlen + strlen(tnm) + 1;
	totalsz = headsz + datasz;
	for (ntries = nacked = 1; ntries < RFCL_MAXTRIES && nacked; ntries++) {
		register rf_request_t	*reqp;
		register caddr_t	rqdatap;

		if ((error = rf_allocmsg(headsz, datasz, BPRI_LO, TRUE,
		  NULLCADDR, NULLFRP, &bp)) != 0) {
			break;
		}
		rfcl_reqsetup(bp, chansdp, crp, RFRENAME, ULIMIT);
		reqp = RF_REQ(bp);
		rqdatap = rf_msgdata(bp, headsz);

		reqp->rq_rename.frdid = fdsdp->sd_connid;
		reqp->rq_rename.trdid = chansdp->sd_connid;
		(void)strcpy(rqdatap, fnm);
		(void)strcpy(rqdatap + fnmlen, tnm);
		error = rfcl_xac(&bp, totalsz, rdp, vcver, FALSE, &nacked);
	}
	if (!error) {
		error = RF_RESP(bp)->rp_errno;
		rf_freemsg(bp);
	}
	rcvd_free(&rdp);
	return error;
}

STATIC int
rf_rmdir(dvp, nm, cdvp, crp)
	vnode_t		*dvp;
	char		*nm;
	vnode_t		*cdvp;	/* current directory; must not be removed */
	cred_t		*crp;
{
	register int	error;
	int		nacked;
	int		ntries;
	mblk_t		*bp = NULL;
	rcvd_t		*rdp;
	register sndd_t	*chansdp = VTOSD(dvp);
	register int	vcver;
	size_t		headsz;
	size_t		datasz;
	size_t		totalsz;

	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	rfcl_fsinfo.fsivop_other++;
	vcver = QPTOGP(chansdp->sd_queue)->version;
	if (vcver < RFS2DOT0) {
		return du_rmdir(dvp, nm, cdvp, crp);
	}
	/* create an rd on which to receive the response */
	if ((error = rcvd_create(TRUE, RDSPECIFIC, &rdp)) != 0) {
		return error;
	}
	rdp->rd_sdp = chansdp;
	headsz = RF_MIN_REQ(vcver);
	datasz = strlen(nm) + 1;
	totalsz = headsz + datasz;
	for (ntries = nacked = 1; ntries < RFCL_MAXTRIES && nacked; ntries++) {
		if ((error = rf_allocmsg(headsz, datasz, BPRI_LO, TRUE,
		  NULLCADDR, NULLFRP, &bp)) != 0) {
			break;
		}
		rfcl_reqsetup(bp, chansdp, crp, RFRMDIR, ULIMIT);
		/*
		 * The VOP_RMDIR interface specifies that cdvp (the user's
		 * current * directory) must not be removed.  If it and dvp
		 * (the parent directory of nm) are in the same vfs as seen
		 * by the client then connid is a cookie for cdvp.  Otherwise,
		 * connid is 0.
		 */
		RF_REQ(bp)->rq_rmdir.connid = cdvp->v_vfsp == dvp->v_vfsp ?
		  VTOSD(cdvp)->sd_connid : 0;
		(void)strcpy(rf_msgdata(bp, headsz), nm);
		error = rfcl_xac(&bp, totalsz, rdp, vcver, FALSE, &nacked);
	}
	if (!error) {
		error = RF_RESP(bp)->rp_errno;
		rf_freemsg(bp);
	}
	rcvd_free(&rdp);
	return error;
}

STATIC int
rf_setattr(vp, vap, flags, crp)
	vnode_t		*vp;
	vattr_t		*vap;
	int		flags;
	cred_t		*crp;
{
	register int	error;
	int		nacked;
	int		ntries;
	mblk_t		*bp = NULL;
	rcvd_t		*rdp;
	register sndd_t	*chansdp = VTOSD(vp);
	register int	vcver;
	gdp_t	 	*gp = QPTOGP(chansdp->sd_queue);
	size_t		headsz;
	size_t		datasz;
	size_t		totalsz;

	if (RF_SERVER()) {
		return EMULTIHOP;
	}
	rfcl_fsinfo.fsivop_other++;
	vcver = QPTOGP(chansdp->sd_queue)->version;
	if (vcver < RFS2DOT0) {
		return du_setattr(vp, vap, crp, flags);
	}
	/* create an rd on which to receive the response */
	if ((error = rcvd_create(TRUE, RDSPECIFIC, &rdp)) != 0) {
		return error;
	}
	headsz = RF_MIN_REQ(vcver);
	datasz = sizeof(rf_attr_t);
	if (gp->hetero != NO_CONV) {
		datasz += ATTR_XP;
	}
	totalsz = headsz + datasz;
	rdp->rd_sdp = chansdp;
	for (ntries = nacked = 1; ntries < RFCL_MAXTRIES && nacked; ntries++) {
		register caddr_t	data;

		if ((error = rf_allocmsg(headsz, datasz, BPRI_LO, TRUE,
		  NULLCADDR, NULLFRP, &bp)) != 0) {
			break;
		}
		rfcl_reqsetup(bp, chansdp, crp, RFSETATTR, ULIMIT);
		data = rf_msgdata(bp, headsz);
		RF_REQ(bp)->rq_setattr.flags = flags;
		if (gp->hetero != NO_CONV) {
			rf_attr_t	rf_attr;

			vtorf_attr(&rf_attr, vap);
			(void)rf_tcanon(ATTR_FMT, (caddr_t)&rf_attr, data);
		} else {
			vtorf_attr((rf_attr_t *)data, vap);
		}
		error = rfcl_xac(&bp, totalsz, rdp, vcver, FALSE, &nacked);
	}
	if (!error) {
		error = RF_RESP(bp)->rp_errno;
		rf_freemsg(bp);
	}
	rcvd_free(&rdp);
	return error;
}

STATIC int
rf_symlink(dvp, linkname, vap, target, crp)
	vnode_t		*dvp;
	char		*linkname;
	struct vattr	*vap;
	char		*target;
	cred_t		*crp;
{
	register int	error;
	int		nacked;
	int		ntries;
	mblk_t		*bp = NULL;
	size_t		targetln;
	int		tflag;
	rcvd_t		*rdp = NULL;
	register 	sndd_t *chansdp = VTOSD(dvp);
	sndd_t		*replysdp = NULL;
	register gdp_t	*gp = QPTOGP(chansdp->sd_queue);
	register int	vcver = gp->version;
	size_t		headsz;
	size_t		datasz;
	size_t		totalsz;

	if (RF_SERVER()) {
		return EMULTIHOP;
	}

	/* If remote is older than RFS2DOT0, symlink is not supported. */
	if (vcver < RFS2DOT0) {
		return ENOSYS;
	}

	rfcl_fsinfo.fsivop_other++;

	/* create an rd on which to receive the response */
	if ((error = rcvd_create(TRUE, RDSPECIFIC, &rdp)) != 0) {
		goto out;
	}
	rdp->rd_sdp = chansdp;
	targetln = strlen(target) + 1;
	if (targetln > MAXPATHLEN) {
		error = ENAMETOOLONG;
		goto out;
	}
	/*
	 * For heterogeneous connections, up message size by worst-case
	 * expansion factor.
	 * In all cases, if target is too long to fit in the message data field,
	 * allocate an sndd for later rcopy.
	 */
	headsz = RF_MIN_REQ(vcver);
	datasz = sizeof(struct rqsymlink);
	if (gp->hetero != NO_CONV) {
		datasz += SYMLNK_XP;
	}
	if (datasz + targetln <= gp->datasz) {
		tflag = 1;
		datasz += targetln;
		replysdp = NULL;
	} else {
		tflag = 0;
		if (error = sndd_create(TRUE, &replysdp)) {
			goto out;
		}
	}
	totalsz = headsz + datasz;
	for (ntries = nacked = 1; ntries < RFCL_MAXTRIES && nacked; ntries++) {
		if ((error = rf_allocmsg(headsz, datasz, BPRI_LO, TRUE,
		  NULLCADDR, NULLFRP, &bp)) != 0) {
			break;
		}
		rfcl_reqsetup(bp, chansdp, crp, RFSYMLINK, ULIMIT);
		datasz = rf_symlnk_msg(bp, vap, tflag, targetln,
		  target, linkname, gp, datasz);
		error = rfcl_xac(&bp, totalsz, rdp, vcver, FALSE, &nacked);
	}
	if (!error) {
		error = rf_symlnk_resp(replysdp, bp, gp, targetln, target, rdp);
	}
out:
	rcvd_free(&rdp);
	sndd_free(&replysdp);
	rfcl_fsinfo.fsiwritech += targetln;
	return error;
}

/*
 * Fill in the request fields specific to symlink, canonize if necessary.
 * return the length of the data field.
 */
STATIC int
rf_symlnk_msg(bp, vap, tflag, targetln, targetnm, linknm, gp, datasz)
	mblk_t		*bp;
	vattr_t		*vap;
	int		tflag;
	size_t		targetln;
	char		*targetnm;
	char		*linknm;
	gdp_t		*gp;
	size_t		datasz;
{
	rf_request_t	*reqp = RF_REQ(bp);
	register struct rqsymlink *rqsymp;
	int		canon = gp->hetero != NO_CONV;
	caddr_t		data = rf_msgdata(bp, RF_MIN_REQ(gp->version));
	caddr_t		workspace;
	size_t		worksize;

	if (canon) {
		worksize = datasz;
		workspace = kmem_alloc(worksize, KM_SLEEP);
		rqsymp = (struct rqsymlink *)workspace;
	} else {
		rqsymp = (struct rqsymlink *)data;
	}
	vtorf_attr(&rqsymp->rqmkdent.attr, vap);
	strcpy(rqsymp->rqmkdent.nm, linknm);
	if ((reqp->rq_slink.tflag = tflag) == 1) {
		strcpy(rqsymp->target, targetnm);
	} else {
		reqp->rq_slink.targetln = targetln;
		rqsymp->target[0] = NULL;
	}
	if (canon) {
		datasz = rf_tcanon(SYMLNK_FMT, workspace, data);
		kmem_free(workspace, worksize);
	}
	return datasz;
}

/*
 * Deal with the first non-nack response to our symlink request.  Verify
 * that message's sanity.  Handle an RCOPYIN if necessary.  Always frees bp.
 * Returns any error, even from the bracketing response.
 */
STATIC int
rf_symlnk_resp(replysdp, bp, gp, targetln, targetnm, rdp)
	sndd_t			*replysdp;
	mblk_t			*bp;
	gdp_t			*gp;
	size_t			targetln;
	char			*targetnm;
	rcvd_t			*rdp;
{
	/*
	 * resp is reused for our RFCOPYIN response, if any, and the bracketing
	 * response from the server in that case.
	 */
	register rf_response_t	*resp = RF_RESP(bp);
	register rf_common_t	*cop = RF_COM(bp);
	int			error = 0;
	int			sink;

	/*
	 * We require servers to take in one response any target name that
	 * was not sent with the original request.
	 */
	if (cop->co_opcode == RFCOPYIN && replysdp &&
	  resp->rp_count == targetln) {
		struct rf_message	*mp = RF_MSG(bp);
		register caddr_t	rpdata;
		size_t			hdrsz = RF_MIN_RESP(gp->version);

		sndd_set(replysdp, (queue_t *)mp->m_queue, mp->m_giftid);
		rf_freemsg(bp);
		(void)rf_allocmsg(hdrsz, targetln, BPRI_MED, FALSE, NULLCADDR,
		  NULLFRP, &bp);
		ASSERT(bp);
		resp = RF_RESP(bp);
		cop = RF_COM(bp);
		rpdata = rf_msgdata(bp, hdrsz);
		strcpy(rpdata, targetnm);
		resp->rp_count = targetln;
		cop->co_type = RF_RESP_MSG;
		cop->co_opcode = RFCOPYIN;
		resp->rp_errno = 0;
		rdp->rd_sdp = replysdp;
		if ((error = rfcl_xac(&bp, RF_MIN_REQ(gp->version) + targetln,
		  rdp, gp->version, FALSE, &sink)) == 0) {
			resp = RF_RESP(bp);		/* reuse resp */
			if (!rf_sigisempty(resp, gp->version)) {
				rf_postrpsigs(resp, gp->version, u.u_procp);
			}
			u.u_procp->p_sig |= resp->rp_v1sig;
			error = resp->rp_errno;
		}
	} else if (cop->co_opcode != RFSYMLINK || replysdp) {
		gdp_discon("rf_symlink bad opcode", gp);
		error = EPROTO;
	} else {
		error = resp->rp_errno;
	}
	rf_freemsg(bp);
	return error;
}

/*
 * A hit in the rflkc access cache requires that the cache be valid, that
 * it is for our vp, and either that its lkc_crp is crp or the contained
 * ids match ours.
 */
STATIC int
rflkc_acc_hit(vp, crp)
	vnode_t		*vp;
	register cred_t	*crp;
{
	register cred_t *lkc_crp = rfcl_lookup_cache.lkc_crp;
	register ushort	gn;
	register int	match;

	if (rfcl_lookup_cache.lkc_vp != vp || !lkc_crp || !crp) {
		return 0;
	}
	if (lkc_crp == crp) {
		return 1;
	}
	if (lkc_crp->cr_uid != crp->cr_uid || lkc_crp->cr_gid != crp->cr_gid ||
	  lkc_crp->cr_ngroups != crp->cr_ngroups) {
		return 0;
	}
	match = 1;
	for (gn = 0; gn < crp->cr_ngroups; gn++) {
		if (lkc_crp->cr_groups[gn] != crp->cr_groups[gn]) {
			match = 0;
			break;
		}
	}
	return match;
}

/*
 * Pullup and decanonize directory entries in RFS response message,
 * which must have RFS headers.  Updates *uio_errorp, returns
 * nonzero for fatal errors.  May update mblk denoted by bp.
 */
STATIC int
rf_readdir_fcanon(bp, hdrsz, gp, uio_errorp)
	register mblk_t		*bp;
	register size_t		hdrsz;
	gdp_t			*gp;
	int			*uio_errorp;
{
	register size_t		datasz;
	register caddr_t	msgdata;
	register rf_response_t	*rp;

	datasz = RF_MSG(bp)->m_size - hdrsz;
	if ((*uio_errorp = RF_PULLUP(bp, hdrsz, datasz)) == 0) {
		rp = RF_RESP(bp);
		msgdata = rf_msgdata(bp, hdrsz);
		if ((rp->rp_count = rf_denfcanon(rp->rp_count, msgdata,
		   msgdata + datasz)) == 0) {
			gdp_discon("rf_readdir_fcanon bad data", gp);
			return EPROTO;
		}
	}
	return 0;
}
