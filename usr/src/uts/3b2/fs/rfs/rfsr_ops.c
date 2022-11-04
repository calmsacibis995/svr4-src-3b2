/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fs:fs/rfs/rfsr_ops.c	1.3.1.6"

/*
 * Operations for kernel daemon process that handles requests
 * for file activity from remote unix systems.
 */
#include "sys/list.h"
#include "sys/types.h"
#include "sys/sysinfo.h"
#include "sys/time.h"
#include "sys/fs/rf_acct.h"
#include "sys/vnode.h"
#include "sys/vfs.h"
#include "sys/sysmacros.h"
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/mode.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/immu.h"
#include "sys/stream.h"
#include "vm/seg.h"
#include "rf_admin.h"
#include "sys/rf_comm.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/dirent.h"
#include "sys/nserve.h"
#include "sys/rf_cirmgr.h"
#include "sys/idtab.h"
#include "sys/rf_messg.h"
#include "sys/var.h"
#include "sys/file.h"
#include "sys/fstyp.h"
#include "sys/fcntl.h"
#include "sys/proc.h"
#include "sys/stat.h"
#include "sys/statfs.h"
#include "sys/inline.h"
#include "sys/debug.h"
#include "sys/rf_debug.h"
#include "sys/cmn_err.h"
#include "sys/conf.h"
#include "sys/buf.h"
#include "sys/rf_adv.h"
#include "sys/uio.h"
#include "sys/fs/rf_vfs.h"
#include "sys/pathname.h"
#include "sys/hetero.h"
#include "rf_serve.h"
#include "sys/ustat.h"
#include "sys/statvfs.h"
#include "rfcl_subr.h"
#include "rf_auth.h"
#include "sys/fbuf.h"
#include "vm/seg_map.h"
#include "rf_canon.h"
#include "sys/mman.h"
#include "rf_cache.h"
#include "du.h"
#include "sys/kmem.h"

/*
 * Table indexed by opcode points to these functions, which handle
 * remote file requests.
 */

/*
 * VOP_ACCESS
 */
/* ARGSUSED */
STATIC int
rfsr_access(stp, ctrlp)
	register rfsr_state_t *stp;
	register rfsr_ctrl_t *ctrlp;
{
	register rf_request_t *req = RF_REQ(stp->sr_in_bp);
	vnode_t *vp = stp->sr_rdp->rd_vp;
	register int fmode = req->rq_mode_op.fmode;

	rfsr_fsinfo.fsivop_other++;
	SR_FREEMSG(stp);
	if (fmode & VWRITE && stp->sr_srmp->srm_flags & SRM_RDONLY) {
		return EROFS;
	} else {
		return VOP_ACCESS(vp, fmode, 0, stp->sr_cred);
	}
}

/*
 * VOP_CLOSE
 */
/* ARGSUSED */
STATIC int
rfsr_close(stp, ctrlp)
	register rfsr_state_t 	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register rcvd_t 	*rdp = stp->sr_rdp;
	register vnode_t 	*vp = rdp->rd_vp;
	rd_user_t		*rdup;
	int			error = 0;
	int 			inflated = 0;		/* flag; see below */
	vattr_t			vattr;

	rfsr_fsinfo.fsivop_close++;
	rdup = rdu_find(rdp, u.u_procp->p_sysid, u.u_srchan->sd_mntid,
	  (rd_user_t **)NULL);
	if (!rdup) {
		cmn_err(CE_NOTE, "rfsr_close cannot find rd_user structure");
	}
	if (rdup && rdup->ru_fcount > 1) {
		/*
		 * This may be the last close for some file table entry on
		 * the client, but is not the last file table entry on the
		 * client for this file.  Inflate the reference count on
		 * the vnode to avoid losing it.
		 */
		VN_HOLD(vp);
		inflated = 1;
	}
	/*
	 * Use file table values of client
	 */
	error = VOP_CLOSE(vp, req->rq_close.fmode, req->rq_close.count,
				req->rq_close.foffset, stp->sr_cred);
	if (inflated) {
		VN_RELE(vp);
	}
	if (req->rq_close.count == 1) {
		/*
		 * Last reference from a file table entry on client
		 * corresponds to last reference for some giving of this
		 * gift; give up corresponding reference in rduser structure.
		 */
		rdu_close(rdp, stp->sr_gdpp->sysid, u.u_srchan->sd_mntid,
		  req->rq_close.fmode);
	}
	/*
	 * 3.2 clients need vcode in return value continue cacheing a file upon
	 * each close.
	 */
	if (!error) {
		vattr.va_mask = AT_VCODE;
		if (!(error = VOP_GETATTR(vp, &vattr, 0, stp->sr_cred))) {
			stp->sr_ret_val = vattr.va_vcode;
		}
	}
	if (stp->sr_vcver > RFS1DOT0 && req->rq_close.lastclose) {
		STATIC int rfsr_inactive();

		error = rfsr_inactive(stp, ctrlp);
	} else {
		SR_FREEMSG(stp);
	}
	return error;
}

/*
 * 3.x clients give up their only vnode reference with RFINACTIVE.  4.0 clients
 * give up vcount references at a time.
 */
/* ARGSUSED */
STATIC int
rfsr_inactive(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register vnode_t	*vp = stp->sr_rdp->rd_vp;
	register long		vcount;
	register long		mntid = RF_COM(stp->sr_in_bp)->co_mntid;
	register sysid_t	sysid = stp->sr_gdpp->sysid;

	rfsr_fsinfo.fsivop_other++;
	if (stp->sr_vcver == RFS1DOT0) {
		vcount = 1;
	} else {
                vcount = RF_REQ(stp->sr_in_bp)->rq_rele.vcount;
	}
	SR_FREEMSG(stp);
	if (vcount < 1 || stp->sr_srmp->srm_refcnt < vcount + 1 ||
          !(stp->sr_rdp->rd_stat & RDUSED)) {
		return rfsr_discon("rfsr_inactive redundant VN_RELE", stp);
	}
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
	/*
	 * rf_serve() calls rfsr_cacheck too, but in the case of this op, our
	 * operand vp will be gone.  rfsr_cacheck() can fail, but its
	 * return is discarded so the client will stay happy.
	 */
	if (stp->sr_vcver >= RFS2DOT0) {
		(void)rfsr_cacheck(stp, mntid);
	}
	while (vcount--) {
		--stp->sr_srmp->srm_refcnt;
		rcvd_delete(&stp->sr_rdp, sysid, mntid);
		VN_RELE(vp);
	}
	return 0;
}

/*
 * VOP_CREATE
 */
STATIC int
rfsr_create(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	vnode_t *vp;
	register rf_request_t	*req;
	register int		version = stp->sr_vcver;
	register struct rqmkdent *rqdp;
	register int		error = 0;
	register size_t		hdrsz = RF_MIN_REQ(version);
	register size_t		datasz = RF_MSG(stp->sr_in_bp)->m_size - hdrsz;
	vattr_t			vattr;

	if (version < RFS2DOT0) {
		return dusr_creat(stp, ctrlp);
	}
	rfsr_fsinfo.fsivop_create++;

	if ((error = RF_PULLUP(stp->sr_in_bp, hdrsz, datasz)) != 0) {
		return error;
	}

	req = RF_REQ(stp->sr_in_bp);
	rqdp = (struct rqmkdent *)rf_msgdata(stp->sr_in_bp, hdrsz);
	if (stp->sr_gdpp->hetero != NO_CONV &&
	  !rf_fcanon(MKDENT_FMT, (caddr_t)rqdp, (caddr_t)rqdp + datasz,
	  (caddr_t)rqdp)) {
		return rfsr_discon("rfsr_create bad data", stp);
	}
	rftov_attr(&vattr, &rqdp->attr);
	vattr.va_atime.tv_sec -= stp->sr_gdpp->timeskew_sec;
	vattr.va_ctime.tv_sec -= stp->sr_gdpp->timeskew_sec;
	vattr.va_mtime.tv_sec -= stp->sr_gdpp->timeskew_sec;
	error = VOP_CREATE(stp->sr_rdp->rd_vp, rqdp->nm, &vattr,
	  req->rq_create.ex, req->rq_create.fmode, &vp, stp->sr_cred);
	SR_FREEMSG(stp);
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
	if (!error &&
	  (error = rfsr_gift_setup(stp, vp, u.u_srchan))) {
		VN_RELE(vp);
	}
	return error;
}

/*
 * VOP_FRLOCK
 */
/* ARGSUSED */
STATIC int
rfsr_frlock(stp, ctrlp)
	register rfsr_state_t 	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t 	*rvp = stp->sr_rdp->rd_vp;
	register int 		cmd = req->rq_fcntl.cmd;
	register long 		fflag = req->rq_fcntl.fflag;
	off_t 			offset = (off_t)req->rq_fcntl.offset;
	int			error = 0;
	register int		canon = stp->sr_gdpp->hetero != NO_CONV;
	/*
	 * Deadlock avoidance:
	 * Hold incoming data for ops that might sleep, letting us free
	 * incoming streams message.
	 * We assume that a struct flock fits in one request/response.
	 */
	flock_t			flock;

	rfsr_fsinfo.fsivop_other++;
	stp->sr_ret_val = cmd;
	/*
	 * Lock data is prewritten.
	 * Likely to sleep on a lock.
	 * Copy data into stack to free streams message.
	 */
	error = rfsr_copyflock((caddr_t)&flock, stp);
	if (!error) {
		SR_FREEMSG(stp);
		error = VOP_FRLOCK(rvp, cmd, (int)&flock, fflag, offset,
		  stp->sr_cred);
	}
	if (!error) {
		if (cmd == F_GETLK || cmd == F_O_GETLK) {
			caddr_t		rpdata;

			ASSERT(!stp->sr_out_bp);
			stp->sr_out_bp = rfsr_rpalloc(canon ?
			  sizeof(flock_t) + FLOCK_XP : sizeof(flock_t),
			  stp->sr_vcver);
			rpdata =
			  rf_msgdata(stp->sr_out_bp, RF_MIN_RESP(stp->sr_vcver));
			if (canon) {
				RF_RESP(stp->sr_out_bp)->rp_count =
				  rf_tcanon(FLOCK_FMT, (caddr_t)&flock, rpdata);
			} else {
				*(flock_t *)rpdata = flock;
			}
		} else {
			ASSERT(!stp->sr_out_bp);
			stp->sr_out_bp =
				rfsr_rpalloc((size_t)0, stp->sr_vcver);
		}
	}
	return error;

}

/*
 * VOP_SPACE
 */
/* ARGSUSED */
STATIC int
rfsr_space(stp, ctrlp)
	register rfsr_state_t 	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t 	*rvp = stp->sr_rdp->rd_vp;
	register int 		cmd = req->rq_fcntl.cmd;
	register long 		flag = req->rq_fcntl.fflag;
	off_t 			offset = (off_t)req->rq_fcntl.offset;
	int			error = 0;
	/*
	 * Deadlock avoidance:
	 * Hold incoming data for ops that might sleep, letting us free
	 * incoming streams message.
	 * We assume that a flock_t fits in one request/response
	 */
	flock_t flock;

	rfsr_fsinfo.fsivop_other++;
	stp->sr_ret_val = cmd;
	/*
	 * Lock data is prewritten.
	 * Likely to sleep on a lock.
	 * Copy data into stack to free streams message.
	 */
	error = rfsr_copyflock((caddr_t)&flock, stp);
	if (!error) {
		SR_FREEMSG(stp);
		if (rvp->v_type != VREG) {
			error = EINVAL;
		} else {
			error = VOP_SPACE(rvp, cmd, (int)&flock, flag,
					offset, stp->sr_cred);
		}
	}
	if (!error) {
		ASSERT(!stp->sr_out_bp);
		stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
	}
	return error;
}

/*
 * VOP_SETFL
 */
/* ARGSUSED */
STATIC int
rfsr_setfl(stp, ctrlp)
	register rfsr_state_t 	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t 	*vp = stp->sr_rdp->rd_vp;
	register int 		oflags = req->rq_fcntl.fcntl;
	register int 		nflags = req->rq_fcntl.fflag;
	int			error = 0;

	rfsr_fsinfo.fsivop_other++;
	SR_FREEMSG(stp);
	error = VOP_SETFL(vp, oflags, nflags, stp->sr_cred);
	if (!error) {
		ASSERT(!stp->sr_out_bp);
		stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
	}
	return error;
}

/*
 * VOP_GETATTR
 */
/* ARGSUSED */
STATIC int
rfsr_getattr(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t	*rvp = stp->sr_rdp->rd_vp;
	register int		error = 0;
	register rf_response_t	*rp;
	int			canon = stp->sr_gdpp->hetero != NO_CONV;
	vattr_t			vattr;

	rfsr_fsinfo.fsivop_other++;
	vattr.va_mask = req->rq_getattr.mask;
	SR_FREEMSG(stp);
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc(stp->sr_gdpp->hetero != NO_CONV ?
			   sizeof(rf_attr_t) + ATTR_XP : sizeof(rf_attr_t),
			   stp->sr_vcver);
	rp = RF_RESP(stp->sr_out_bp);
	if ((error = VOP_GETATTR(rvp, &vattr, 0, stp->sr_cred)) != 0 ||
	  (error = rfsr_vattr_map(stp, &vattr)) != 0) {
		/* reset these to reflect the failure */
		rp->rp_count = 0;
		rp->rp_nodata = 1;
	} else {
		register rf_attr_t	*rap;
		register caddr_t	data;
		rf_attr_t		rf_attr;

		data = rf_msgdata(stp->sr_out_bp, RF_MIN_RESP(stp->sr_vcver));
		rap = canon ? &rf_attr : (rf_attr_t *)data;
		vtorf_attr(rap, &vattr);
		if (canon) {
			rp->rp_count = rf_tcanon(ATTR_FMT, (caddr_t)rap, data);
		}
	}
	return error;
}

/* ARGSUSED */
STATIC int
rfsr_setattr(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register vnode_t	*vp = stp->sr_rdp->rd_vp;
	register rf_attr_t	*rap;
	register int		error;
	size_t			datasz;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);
	int			hetero = stp->sr_gdpp->hetero;


	rfsr_fsinfo.fsivop_other++;
	datasz = hetero == NO_CONV ? sizeof(rf_attr_t) :
	  sizeof(rf_attr_t) + ATTR_XP - MINXPAND;
	if ((error = RF_PULLUP(stp->sr_in_bp, hdrsz, datasz)) != 0) {
		return error;
	}
	rap = (rf_attr_t *)rf_msgdata(stp->sr_in_bp, hdrsz);
	if (hetero != NO_CONV &&
	  !rf_fcanon(ATTR_FMT, (caddr_t)rap, (caddr_t)rap + datasz,
	   (caddr_t)rap)) {
		return rfsr_discon("rfsr_setattr bad data", stp);
	}
	rap->rfa_uid = gluid(stp->sr_gdpp, rap->rfa_uid);
	rap->rfa_gid = glgid(stp->sr_gdpp, rap->rfa_gid);

	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
	} else {
		vattr_t	vattr;

		rftov_attr(&vattr, rap);
		vattr.va_atime.tv_sec -= stp->sr_gdpp->timeskew_sec;
		vattr.va_ctime.tv_sec -= stp->sr_gdpp->timeskew_sec;
		vattr.va_mtime.tv_sec -= stp->sr_gdpp->timeskew_sec;
		error = VOP_SETATTR(vp, &vattr,
		  RF_REQ(stp->sr_in_bp)->rq_setattr.flags, stp->sr_cred);
		if (!error && vattr.va_mask & AT_MODE &&
		  MANDLOCK(vp, vattr.va_mode)) {
			/*
			 * Other clients will have received disable messages
			 * if this one enabled locks.  Now we send this one
			 * an implicit disable message.
			 */
			rd_user_t *rdup = rdu_find(stp->sr_rdp,
			  u.u_procp->p_sysid, u.u_srchan->sd_mntid,
			  (rd_user_t **)NULL);

			rdup->ru_cflag &= ~RU_CACHE_ON;
			rdup->ru_cflag |= RU_CACHE_DISABLE;
		}
		SR_FREEMSG(stp);
	}
	return error;
}

/*
 * VOP_FSYNC
 */
/* ARGSUSED */
STATIC int
rfsr_fsync(stp, ctrlp)
	register rfsr_state_t *stp;
	register rfsr_ctrl_t *ctrlp;
{
	vnode_t *vp = stp->sr_rdp->rd_vp;

	rfsr_fsinfo.fsivop_other++;
	SR_FREEMSG(stp);
	return VOP_FSYNC(vp, stp->sr_cred);
}

/*
 * VOP_READDIR
 */
#define RF_MINDIRENT	12	/* dir entry with one char name */
/* ARGSUSED */
STATIC int
rfsr_readdir(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t	*vp = stp->sr_rdp->rd_vp;
	register sndd_t		*sdp = u.u_srchan;		/* gag */
	size_t			resid = req->rq_xfer.count;
	size_t			oresid = resid;
	register int		error = 0;
	int			dircanon = stp->sr_gdpp->hetero != NO_CONV;
	size_t			datasz = stp->sr_gdpp->datasz;
	size_t			hdrsz = RF_MIN_RESP(stp->sr_vcver);
	int			eof = 0;
	long			base = req->rq_xfer.base;
	struct uio		uio;
	struct iovec		iovec;
	caddr_t			workspace;
	size_t			worksize;

	rfsr_fsinfo.fsivop_readdir++;
	uio.uio_iov = &iovec;
	uio.uio_iovcnt = 1;
	uio.uio_offset = req->rq_xfer.offset;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_fmode = 0;	/* TO DO:  ill-specified use */
	SR_FREEMSG(stp);

	if (dircanon) {
		worksize = MAX(RF_MAXDIRENT, MIN(datasz, oresid));
		workspace = kmem_alloc(worksize, KM_SLEEP);
	}
	ASSERT(!stp->sr_out_bp);
	VOP_RWLOCK(vp);
	while (!error && resid) {
		register size_t		readsize;
		register rf_response_t	*rp;
		caddr_t			data;
		caddr_t			rpdata;
		off_t			ooff = uio.uio_offset;

		/*
		 * If resid is smaller than the next directory entry,
		 * we can get an unwarranted error.
		 */
		uio.uio_resid = iovec.iov_len = readsize
		  = MAX(RF_MAXDIRENT, MIN(datasz, resid));
		stp->sr_out_bp = rfsr_rpalloc(readsize +
		  (dircanon ? readsize / RF_MINDIRENT * DIRENT_XP : 0),
		  stp->sr_vcver);
		rp = RF_RESP(stp->sr_out_bp);
		rpdata = rf_msgdata(stp->sr_out_bp, hdrsz);
		iovec.iov_base = dircanon ? workspace : rpdata;
		data = iovec.iov_base;
		error = VOP_READDIR(vp, &uio, stp->sr_cred, &eof);
		if (!error && resid < RF_MAXDIRENT &&
		  !rfsr_lastdirents(data, &uio, ooff, resid, &eof) &&
		  resid == oresid) {
			/*
			 * Can't fit an entry into response, first time through
			 * loop.
			 */
			error = EINVAL;
		}
		if (!error) {
			/*
			 * The size of the data transfer is the size request
			 * from the readdir less the residual count from the
			 * op.  The total residual count for the IO is reduced
			 * by the amount of data moved.
			 *
			 * The original protocol has the server doing client
			 * pointer arithmetic, not a wonderful idea.  We
			 * continue for compatability, but clients should
			 * ignore it.  (4.0 clients do.)
			 */
			rp->rp_count = readsize - uio.uio_resid;
			rp->rp_nodata = !rp->rp_count;
			rp->rp_offset = uio.uio_offset;
			rp->rp_copyout.buf = base;
			rp->rp_xfer.eof = eof;
			base += rp->rp_count;
			resid -= rp->rp_count;
			if (!rp->rp_nodata && dircanon) {
				rp->rp_count = rf_dentcanon(rp->rp_count,
				  workspace, rpdata);
			}
			if (resid == 0 || uio.uio_resid == readsize || eof) {
				/*
				 * Send last data in bracketing response.
				 * We can't assume an equivalence between
				 * the last two terms because rfsr_lastdirents
				 * may have diddled uio_resid.
				 */
				break;
			}
			/*
			 * Assume there is more data to come and send the
			 * message.
			 */
			RF_COM(stp->sr_out_bp)->co_opcode = RFCOPYOUT;
			rp->rp_copyout.copysync = 0; /* relic of static queue */
			error = rf_sndmsg(sdp, stp->sr_out_bp,
			  hdrsz + (size_t)rp->rp_count,
			  (rcvd_t *)NULL, FALSE);
			stp->sr_out_bp = NULL;
		}
	}
	VOP_RWUNLOCK(vp);
	if (dircanon) {
		kmem_free(workspace, worksize);
	}
	stp->sr_ret_val = oresid - resid;
	return error;
}

/*
 * VOP_IOCTL
 */
/* ARGSUSED */
STATIC int
rfsr_ioctl(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t	*rvp = stp->sr_rdp->rd_vp;
	register int		error;
	file_t			*fp;

	rfsr_fsinfo.fsivop_other++;
	SR_FREEMSG(stp);
	error = VOP_IOCTL(rvp, req->rq_ioctl.cmd, req->rq_ioctl.arg,
	  req->rq_ioctl.fflag, stp->sr_cred, ((int *)&stp->sr_ret_val));
	if (getf(0, &fp) == 0) {

		/*
		 * Ugly hack for ioctls that do opens.  Only known
		 * example is /proc.
		 */

		register vnode_t	*nvp = fp->f_vnode;

		if (!stp->sr_out_bp) {
			stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
		}
		if (error || (error = rfsr_gift_setup(stp, nvp, u.u_srchan))) {
			closef(fp);
		} else {
			unfalloc(fp);
		}
		setf(0, NULLFP);
	}
	return error;
}

/*
 * VOP_LINK
 */
STATIC int
rfsr_link(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register vnode_t	*dvp = stp->sr_rdp->rd_vp;
	register vnode_t	*fvp;
	int			rcvindx;
	int			error = 0;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);

	if (stp->sr_vcver < RFS2DOT0) {
		return dusr_link1(stp, ctrlp);
	}
	rfsr_fsinfo.fsivop_other++;

	/*
	 *All pathnames are fully resolved, and the named target file
	 * is a simple component in the directory represented by the
	 * rd on which the request came.
	 *
	 * The checks are insurance against ill-behaved clients.
	 *
	 * Find source operand vnode and target dir vnode, and make sure
	 * they're in the same writable VFS.
	 */

	if ((rcvindx = RF_REQ(stp->sr_in_bp)->rq_rflink.link) == 0 ||
	  (fvp = rcvd[rcvindx].rd_vp) == NULL) {
		error = ENOENT;
	} else if (fvp->v_vfsp != dvp->v_vfsp) {
		error = EXDEV;
	} else if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error =  EROFS;
	} else if ((error = RF_PULLUP(stp->sr_in_bp, hdrsz,
	  (size_t)RF_MSG(stp->sr_in_bp)->m_size - hdrsz)) == 0) {
		error = VOP_LINK(dvp, fvp, rf_msgdata(stp->sr_in_bp, hdrsz),
		  stp->sr_cred);
	}
	SR_FREEMSG(stp)
	return error;
}


STATIC int
rfsr_lookup(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	vnode_t			*vp;
	vnode_t			**cvpp;
	vnode_t			**dvpp;
	int			error = 0;
	register		int hetero = stp->sr_gdpp->hetero != NO_CONV;
	rflkc_info_t		rflkc_info;

	rfsr_fsinfo.fsivop_lookup++;
	if (RF_REQ(stp->sr_in_bp)->rq_lookup.flags & LOOKUP_DIR) {
		dvpp = &vp;
		cvpp = NULLVPP;
	} else {
		dvpp = NULLVPP;
		cvpp = &vp;
	}
	error = rfsr_lookupname(NO_FOLLOW, stp, dvpp, cvpp, ctrlp);
	SR_FREEMSG(stp);
	if (*ctrlp != SR_NORMAL || error) {
		return error;
	}
	ASSERT(!stp->sr_out_bp);
	if (hetero) {
		stp->sr_out_bp = rfsr_rpalloc(sizeof(rflkc_info_t) +
		  RFLKC_XP, stp->sr_vcver);
	} else {
		stp->sr_out_bp = rfsr_rpalloc(sizeof(rflkc_info_t),
		  stp->sr_vcver);
	}
	if (error = rfsr_gift_setup(stp, vp, u.u_srchan)) {
		VN_RELE(vp);
	} else {
		/*
		 * Provide some commonly used information about vp.
		 */
		register rf_response_t *rp = RF_RESP(stp->sr_out_bp);
		vattr_t			vattr;

		vattr.va_mask = AT_ALL;
		if (!VOP_GETATTR(vp, &vattr, 0, stp->sr_cred) &&
		  !rfsr_vattr_map(stp, &vattr)) {
			register caddr_t	rp_data;
			register rflkc_info_t	*rflp;

			rp_data = rf_msgdata(stp->sr_out_bp,
			  RF_MIN_RESP(stp->sr_vcver));
			rflp = hetero ? &rflkc_info : (rflkc_info_t *)rp_data;
			vtorf_attr(&rflp->rflkc_attr, &vattr);
			rflp->rflkc_read_err =
				VOP_ACCESS(vp, VREAD, 0, stp->sr_cred);
			rflp->rflkc_write_err =
				VOP_ACCESS(vp, VWRITE, 0, stp->sr_cred);
			rflp->rflkc_exec_err =
				VOP_ACCESS(vp, VEXEC, 0, stp->sr_cred);
			if (hetero) {
				rp->rp_count = rf_tcanon(RFLKC_FMT,
				  (caddr_t)rflp, rp_data);
			}
			rp->rp_nodata = 0;
		} else {
			rp->rp_count = 0;
			rp->rp_nodata = 1;
		}
	}
	return error;
}

/* ARGSUSED */
STATIC int
rfsr_map(stp, ctrlp)
	register rfsr_state_t *stp;
	register rfsr_ctrl_t *ctrlp;
{
	return VOP_ADDMAP(stp->sr_rdp->rd_vp, 0, (struct as *)NULL,
	  (caddr_t *)NULL, 1, PROT_READ, PROT_READ, 0, stp->sr_cred);
}

/* ARGSUSED */
STATIC int
rfsr_unmap(stp, ctrlp)
	register rfsr_state_t *stp;
{
	return VOP_DELMAP(stp->sr_rdp->rd_vp, 0, (struct as *)NULL,
	  (caddr_t *)NULL, 1, PROT_READ, PROT_READ, 0, stp->sr_cred);
}

/*
 * VOP_MKDIR
 */
STATIC int
rfsr_mkdir(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	vnode_t			*vp;
	register struct rqmkdent *rqdp;
	register int		error = 0;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);
	size_t			datasz = RF_MSG(stp->sr_in_bp)->m_size - hdrsz;
	vattr_t			vattr;

	if (stp->sr_vcver < RFS2DOT0) {
		return dusr_mkdir(stp, ctrlp);
	}

	rfsr_fsinfo.fsivop_other++;

	if ((error = RF_PULLUP(stp->sr_in_bp, hdrsz, datasz)) != 0) {
		SR_FREEMSG(stp);
		return error;
	}

	rqdp = (struct rqmkdent *)rf_msgdata(stp->sr_in_bp, hdrsz);
	if (stp->sr_gdpp->hetero != NO_CONV &&
	  !rf_fcanon(MKDENT_FMT, (caddr_t)rqdp, (caddr_t)rqdp + datasz,
	   (caddr_t)rqdp)) {
		return rfsr_discon("rfsr_mkdir bad data", stp);
	}
	rftov_attr(&vattr, &rqdp->attr);
	error = VOP_MKDIR(stp->sr_rdp->rd_vp, rqdp->nm,
			 &vattr, &vp, stp->sr_cred);
	SR_FREEMSG(stp);
	if (!error) {
		ASSERT(!stp->sr_out_bp);
		stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
		if (error = rfsr_gift_setup(stp, vp, u.u_srchan)) {
			VN_RELE(vp);
		}
	}
	return error;
}

/*
 * VOP_OPEN
 */
STATIC int
rfsr_open(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	int			fmode;
	vnode_t			*vp;
	vnode_t			*ovp;
	int			error = 0;

	if (stp->sr_vcver < RFS2DOT0) {
		return dusr_open(stp, ctrlp);
	}
	rfsr_fsinfo.fsivop_open++;
	fmode = RF_REQ(stp->sr_in_bp)->rq_open.fmode;
	ovp = vp = stp->sr_rdp->rd_vp;
	SR_FREEMSG(stp);
	/*
	 * In case VOP_OPEN swaps vp with another, make sure
	 * it doesn't disappear, because all of a single client's
	 * contribute just 1 to the vnode reference count.
	 */
	VN_HOLD(ovp);
	if ((error = VOP_OPEN(&vp, fmode, stp->sr_cred)) != 0) {
		if (vp != ovp) {
			/*
			 * VOP_OPEN gave us a new vnode.  Client will
			 * release old one in its time.
			 */
			ASSERT(!stp->sr_out_bp);
			stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
			if ((error = rfsr_gift_setup(stp, vp, u.u_srchan))
			  != 0) {
				VN_RELE(vp);
			}
		}
	}
	VN_RELE(ovp);
	return error;
}

/*
 * We do explicit data movement, staying out of RF_SERVER() hooks in
 * copy(in|out) (f|s)u(byte|word), etc.  The intent is that only ioctls
 * still go through those, and that only because drivers know about data
 * direction and format, and we don't.
 *
 * If more than one data movement message is required, we send all but the
 * last as RFCOPYOUT messages.  The last is left in stp->sr_out_bp, for later
 * processing in the server; that message will have a RFREAD opcode.  The
 * number of data bytes in that message is in overloaded rp->rp_count.
 */
STATIC int
rfsr_read(stp, ctrlp)
	register rfsr_state_t *stp;
	register rfsr_ctrl_t *ctrlp;
{
	register rf_request_t *req = RF_REQ(stp->sr_in_bp);
	register vnode_t *vp = stp->sr_rdp->rd_vp;
	/* for compatability, we do ptr update for client
	 */
	caddr_t base = (caddr_t)req->rq_xfer.base;
	register enum vtype vtype = vp->v_type;
	unsigned ioflag = 0;
	struct uio uio;
	struct iovec iovec;
	register int error = 0;

	rfsr_fsinfo.fsivop_read++;
	uio.uio_iov = &iovec;
	if ((error = rfsr_rdwrinit(stp, &uio, vp, &ioflag, ctrlp)) != 0
	  || *ctrlp != SR_NORMAL) {
		SR_FREEMSG(stp);
		return error;
	}
	SR_FREEMSG(stp);
	if (vtype == VCHR && stp->sr_ret_val > stp->sr_gdpp->datasz) {

		/* special code to handle big reads from raw devices */

		error = rfsr_rawread(stp, vp, &uio, ioflag, base);
	} else {
		error = rfsr_cookedread(stp, vp, &uio, ioflag, base);
	}
	if (!stp->sr_out_bp) {
		stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
	}

	if (stp->sr_vcver == RFS1DOT0) {
		if (stp->sr_opcode == RFREAD) {

			/* Only call to rfsr_cacheck for 3.2 clients. */

			error = rfsr_cacheck(stp, u.u_srchan->sd_mntid);
		}
		if (!error) {
			vattr_t vattr;

			vattr.va_mask = AT_SIZE;
			if ((error = VOP_GETATTR(vp, &vattr, 0, stp->sr_cred))
			  == 0) {
				RF_RESP(stp->sr_out_bp)->rp_rdwr.isize =
				  vattr.va_size;
			}
		}
	}
	return error;
}

/*
 * VOP_READLINK
 */
/* ARGSUSED */
STATIC int
rfsr_readlink(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	register vnode_t	*vp = stp->sr_rdp->rd_vp;
	register unsigned	resid = req->rq_xfer.count;
	register rf_response_t	*resp;
	register caddr_t	rpdata;
	struct uio		uio;
	struct iovec		iovec;
	int			error = 0;

	rfsr_fsinfo.fsivop_other++;
	if (resid > MAXPATHLEN) {
		return EINVAL;
	}
	uio.uio_iov = &iovec;
	uio.uio_iovcnt = 1;
	uio.uio_offset = req->rq_xfer.offset;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_fmode = 0;
	SR_FREEMSG(stp);
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc(MAXPATHLEN, stp->sr_vcver);
	resp = RF_RESP(stp->sr_out_bp);
	rpdata = rf_msgdata(stp->sr_out_bp, RF_MIN_RESP(stp->sr_vcver));
	uio.uio_resid = iovec.iov_len = resid;
	iovec.iov_base = rpdata;
	error = VOP_READLINK(vp, &uio, stp->sr_cred);
	stp->sr_ret_val = resid - uio.uio_resid;
	/*
	 * rfsr_rpalloc assigned the following values; correct
	 * them now that the actual
	 * values are known.
	 */
	if ((resp->rp_count = stp->sr_ret_val) == 0) {
		resp->rp_nodata = 1;
	}
	return error;
}

/*
 * VOP_SYMLINK
 */
/* ARGSUSED */
STATIC int
rfsr_symlink(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register vnode_t	*rvp = stp->sr_rdp->rd_vp;
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	struct rqsymlink	*argp;
	int			error = 0;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);
	size_t			datasz = RF_MSG(stp->sr_in_bp)->m_size - hdrsz;
	char			target[MAXPATHLEN + 1];

	rfsr_fsinfo.fsivop_other++;

	if ((error = RF_PULLUP(stp->sr_in_bp, hdrsz, datasz)) != 0) {
		SR_FREEMSG(stp);
		return error;
	}
	argp = (struct rqsymlink *)rf_msgdata(stp->sr_in_bp, hdrsz);
	if (stp->sr_gdpp->hetero != NO_CONV &&
	  !rf_fcanon(SYMLNK_FMT, (caddr_t)argp, (caddr_t)argp + datasz,
	   (caddr_t)argp)) {
		return rfsr_discon("rfsr_symlink bad data", stp);
	}
	if (req->rq_slink.tflag) {
		strcpy(target, argp->target);
	} else if (req->rq_slink.targetln > MAXPATHLEN ||
	  req->rq_slink.targetln <= 0) {
		/*
		 * We have to move the data in one response.
		 */
		error = EPROTO;
	} else if (rcopyin(NULL, target, (uint)req->rq_slink.targetln, 1)) {
		error =  EFAULT;
	}
	if(!error) {
		vattr_t	vattr;

		rftov_attr(&vattr, &argp->rqmkdent.attr);
		vattr.va_atime.tv_sec -= stp->sr_gdpp->timeskew_sec;
		vattr.va_ctime.tv_sec -= stp->sr_gdpp->timeskew_sec;
		vattr.va_mtime.tv_sec -= stp->sr_gdpp->timeskew_sec;
		error = VOP_SYMLINK(rvp, argp->rqmkdent.nm,
		  &vattr, target, stp->sr_cred);
		if (!error) {
			sysinfo.writech += strlen(target) + 1;
		}
	}
	SR_FREEMSG(stp);
	return error;
}

/*
 * VOP_REMOVE
 */
/* ARGSUSED */
STATIC int
rfsr_remove(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	int			error;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);

	rfsr_fsinfo.fsivop_other++;

	if ((error = RF_PULLUP(stp->sr_in_bp, hdrsz,
	  (size_t)RF_MSG(stp->sr_in_bp)->m_size - hdrsz)) == 0) {
		error = VOP_REMOVE(stp->sr_rdp->rd_vp,
 		  rf_msgdata(stp->sr_in_bp, hdrsz), stp->sr_cred);
	}
	SR_FREEMSG(stp);
	return error;
}

/*
 * VOP_RENAME
 */
/* ARGSUSED */
STATIC int
rfsr_rename(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req;
	vnode_t			*fdvp;
	vnode_t			*tdvp;
	char			*fnm;
	char			*tnm;
	int			frcvid;
	int			trcvid;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);
	int			error = 0;

	rfsr_fsinfo.fsivop_other++;

	if ((error = RF_PULLUP(stp->sr_in_bp, hdrsz,
	  (size_t)RF_MSG(stp->sr_in_bp)->m_size - hdrsz)) == 0) {

		req = RF_REQ(stp->sr_in_bp);
		frcvid = req->rq_rename.frdid;
		trcvid = req->rq_rename.trdid;
		fnm = rf_msgdata(stp->sr_in_bp, hdrsz);
		tnm = fnm + strlen(fnm) + 1;

		if (frcvid <= 0 ||
		  frcvid >= nrcvd ||
		  trcvid <= 0 ||
		  trcvid >= nrcvd ||
		  (fdvp = rcvd[frcvid].rd_vp) == NULL ||
		  (tdvp = rcvd[trcvid].rd_vp) == NULL) {
			error = ENOENT;
		} else if (fdvp->v_vfsp != tdvp->v_vfsp) {
			error = EXDEV;
		} else if (tdvp->v_vfsp->vfs_flag & VFS_RDONLY) {
			error = EROFS;
		} else {
			error = VOP_RENAME(fdvp, fnm, tdvp, tnm, stp->sr_cred);
		}
	}
	SR_FREEMSG(stp);
	/*
	 * The vnodes (fdvp and tdvp) will be released by
	 * the client.
	 */
	return error;
}


STATIC int
rfsr_rsignal(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_common_t	*cop = RF_COM(stp->sr_in_bp);
	register proc_t		*procp;

	*ctrlp = SR_NO_RESP;
	for (procp = rfsr_active_procp; procp; procp = procp->p_rlink) {
		if (procp != u.u_procp &&
		  procp->p_epid == (short)cop->co_pid &&
		  procp->p_sysid == u.u_procp->p_sysid) {
			break;
		}
	}
	if (!procp) {
		/* didn't find surrogate of signalled client */
		register mblk_t *sbp;
		register sndd_t *srchan = u.u_srchan;

		srchan->sd_srvproc = NULL;
		/* look for request message we are supposed to interrupt
		 */
		if ((sbp = rfsr_chkrdq(stp->sr_rdp, cop->co_pid, cop->co_sysid))
		  != NULL) {
			/* Found unserviced predecessor request;
			 * replace the current message with the predecessor,
			 * turn on the signal bit so that the signal will
			 * be processed by the system call code
			 */
			register rf_message_t *msig;

			SR_FREEMSG(stp);
			stp->sr_in_bp = sbp;
			msig = RF_MSG(sbp);
			msig->m_stat |= RF_SIGNAL;
			sndd_set(srchan, (queue_t *)msig->m_queue,
			  msig->m_giftid);
			*ctrlp = SR_OUT_OF_BAND;
		} else {
			/*
			 * The predecessor request message must have
			 * completed already (no server, no pending request)
			 */
			SR_FREEMSG(stp);
		}
	} else {
		/* found surrogate of signalled client */
		psignal(procp, SIGTERM);
		SR_FREEMSG(stp);
	}
	return 0;
}

/*
 * VOP_RMDIR
 */
STATIC int
rfsr_rmdir(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	long			connid = RF_REQ(stp->sr_in_bp)->rq_rmdir.connid;
	int			error = 0;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);

	if (stp->sr_vcver < RFS2DOT0) {
		return dusr_rmdir(stp, ctrlp);
	}

	rfsr_fsinfo.fsivop_other++;

	if ((error = RF_PULLUP(stp->sr_in_bp, hdrsz,
	  (size_t)RF_MSG(stp->sr_in_bp)->m_size - hdrsz)) == 0) {
		error = VOP_RMDIR(stp->sr_rdp->rd_vp,
	  	  rf_msgdata(stp->sr_in_bp, hdrsz),
	  	  connid == 0 ? NULLVP : INXTORD(connid)->rd_vp, stp->sr_cred);
	}
	SR_FREEMSG(stp);
	return error;
}

/*
 * VFS_MOUNT
 *
 * If all is okay, create a mount of a resource for the requesting client.
 *
 * NOTE:  Because concurrency could result in inconsistency, this routine
 * has been ordered so that there is no possibility of sleeping from the
 * return of rfsr_rpalloc to the point in rfsr_gift_setup that the resource's
 * rcvd's reference count has been bumped.
 */
/* ARGSUSED */
STATIC int
rfsr_mount(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register struct gdp	*gdpp = stp->sr_gdpp;
	register rf_request_t	*req;
	register caddr_t	mdata;
	int			mflags;
	int			error = 0;
	rf_resource_t		*rp;
	register sndd_t		*srchan = u.u_srchan;
	register sysid_t	my_sysid;
	size_t			hdrsz = RF_MIN_REQ(stp->sr_vcver);
	size_t			datasz = RF_MSG(stp->sr_in_bp)->m_size - hdrsz;
	char			resname[RFS_NMSZ];

	rfsr_fsinfo.fsivop_other++;

	if ((error = RF_PULLUP(stp->sr_in_bp, hdrsz, datasz)) != 0) {
		goto out;
	}
	req = RF_REQ(stp->sr_in_bp);
	if (!gdpp->mntcnt) {
		hibyte(gdpp->sysid) =
		  lobyte(loword(RF_COM(stp->sr_in_bp)->co_sysid));
		u.u_procp->p_sysid = gdpp->sysid;
		rfsr_adj_timeskew(gdpp, req->rq_srmount.synctime, 0);
	}
	my_sysid = u.u_procp->p_sysid;
	stp->sr_srmp = NULL;
	mdata = rf_msgdata(stp->sr_in_bp, hdrsz);
	mflags = (int)req->rq_srmount.mntflag;
	if (!rf_fcanon("c0", mdata, mdata + datasz, resname)) {
		error = rfsr_discon("rfsr_mount bad data", stp);
		goto out;
	}
	if ((error = srm_alloc(&stp->sr_srmp)) != 0) {
		goto out;
	}
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
	if ((rp = name_to_rsc(resname)) == NULL) {
		error =  ENODEV;
		goto out;
	}
 	if (id_to_srm(rp, my_sysid) != NULL) {
		error = EBUSY;
		goto out;
	}
	if ((rp->r_flags & R_RDONLY) && !(mflags & VFS_RDONLY)) {
		error = EROFS;
		goto out;
	}
	/* see if client is authorized  */
	if (rp->r_clistp &&
	  !rf_checkalist(rp->r_clistp, stp->sr_gdpp->token.t_uname)) {
		error = EACCES;
		goto out;
	}
	if (rp->r_flags & R_FUMOUNT) {
		error = ENONET;
		goto out;
	}
#ifdef UNNECESSARY_RESTRICTION_REMOVED
	if (stp->sr_vcver < RFS2DOT0 && rp->r_rootvp->v_type != VDIR) {
		error = ENOTDIR;
		goto out;
	}
#endif
	if (rp->r_flags & R_UNADV) {
		error = ENONET;
		goto out;
	}
	stp->sr_srmp->srm_mntid = srchan->sd_mntid;
	srchan->sd_mntid = rp->r_mntid;
	stp->sr_srmp->srm_sysid = my_sysid;
	stp->sr_srmp->srm_slpcnt = 0;
	if (mflags & VFS_RDONLY) {
		stp->sr_srmp->srm_flags |= SRM_RDONLY;
	}
	if (mflags & MS_CACHE && rfc_time != -1) {
		stp->sr_srmp->srm_flags |= SRM_CACHE;
		stp->sr_ret_val |= MCACHE;
	}
	stp->sr_srmp->srm_nextp = rp->r_mountp;
	stp->sr_srmp->srm_prevp = NULL;
	if (rp->r_mountp) {
		rp->r_mountp->srm_prevp = stp->sr_srmp;
	}
	rp->r_mountp = stp->sr_srmp;
	if ((error = rfsr_gift_setup(stp, rp->r_rootvp, srchan)) != 0) {
		/*
		 * As awkward as the following is, it is necessary because
		 * rfsr_gift_setup may have slept, allowing our rp and/or srmp
		 * to be removed by another process.
		 */
		sr_mount_t *trash_srmp;

		if ((rp = name_to_rsc(resname)) != NULL &&
		  (trash_srmp = id_to_srm(rp, my_sysid)) != NULL) {
			(void)srm_remove(&rp, trash_srmp);
			stp->sr_srmp = NULL;
		}
	}
out:
	if (!error) {
		gdpp->mntcnt++;
		VN_HOLD(stp->sr_gift->rd_vp);
	} else {
		srm_free(&stp->sr_srmp);
		stp->sr_gift = NULL;
	}
	SR_FREEMSG(stp);
	return error;
}

/*
 * VFS_UMOUNT
 *
 * Make sure that the reference count in the srmount table for
 * this system and this inode is zero.  Then free the srmount entry.
 * Vnode is locked by routines that call this routine.  This routine
 * is called by server and by rf_recovery.
 */
/* ARGSUSED */
STATIC int
rfsr_umount(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register long		mntid = u.u_srchan->sd_mntid;
	register vnode_t	*vp = stp->sr_rsrcp->r_rootvp;
	register long		vcount;
	register sysid_t	sysid = stp->sr_gdpp->sysid;
	sr_mount_t		*srp = stp->sr_srmp;
	int			error = 0;

	rfsr_fsinfo.fsivop_other++;
	if (stp->sr_vcver == RFS1DOT0) {
		vcount = 1;
	} else {
                vcount = RF_REQ(stp->sr_in_bp)->rq_rele.vcount;
	}
	SR_FREEMSG(stp);
	DUPRINT3(DB_MNT_ADV,
	  "rfsr_umount: resource %x, vnode %x \n", stp->sr_rsrcp, vp);
        if (srp->srm_refcnt != vcount) {
		/*  still busy for client machine or recovery is going on */
		return EBUSY;
	}
        while (--vcount) {
		--stp->sr_srmp->srm_refcnt;
		rcvd_delete(&stp->sr_rdp, sysid, mntid);
		VN_RELE(vp);
	}
	/*
	 *  Free rfsr_mount structure.  If R_UNADV flag is set
	 * in the resource structure and this was the last
	 * rfsr_mount structure, the resource structure will
	 * also be deallocated.
	 */
	if (srp->srm_flags & SRM_FUMOUNT ||
	  !(error = srm_remove(&stp->sr_rsrcp, srp))) {
		rcvd_delete(&stp->sr_rdp, sysid, mntid);
		stp->sr_gdpp->mntcnt--;
		VN_RELE(vp);
	}
	return error;
}

/*
 * VFS_STATVFS
 */
/* ARGSUSED */
STATIC int
rfsr_statvfs(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	statvfs_t		*svfsb;
	vfs_t			*vfsp = stp->sr_rdp->rd_vp->v_vfsp;
	int			error = 0;
	register rf_response_t	*rp;
	int			canon = stp->sr_gdpp->hetero != NO_CONV;
	caddr_t			rpdata;
	statvfs_t		statvfs;

	rfsr_fsinfo.fsivop_other++;
	SR_FREEMSG(stp);
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc(stp->sr_gdpp->hetero != NO_CONV ?
	  		   sizeof(statvfs_t) + STATVFS_XP : sizeof(statvfs_t),
			   stp->sr_vcver);
	rp = RF_RESP(stp->sr_out_bp);
	rpdata = rf_msgdata(stp->sr_out_bp, RF_MIN_RESP(stp->sr_vcver));
	if (canon) {
		svfsb = &statvfs;
	} else {
		svfsb = (statvfs_t *)rpdata;
	}
	if ((error = VFS_STATVFS(vfsp, svfsb)) != 0) {
		/* reset these to reflect the failure */
		rp->rp_count = 0;
		rp->rp_nodata = 1;
	} else if (canon) {
		rp->rp_count = rf_tcanon(STATVFS_FMT, (caddr_t)svfsb, rpdata);
	}
	return error;
}

/*
 * TO DO:  will we get ed in trouble?  Remove this when possible.
 */
/* ARGSUSED */
STATIC int
rfsr_ustat(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	long			cbuf = req->rq_ustat.buf;
	register struct ustat	*usp;
	vnode_t			*vp;
	register struct vfs	*vfsp;
	register int		error = 0;
	register rf_response_t	*rp;
	caddr_t			rpdata;
	int			canon = stp->sr_gdpp->hetero != NO_CONV;
	struct statvfs		statvfs;
	struct ustat		ustat;

	vfsp = rfsr_dev_dtov((int)req->rq_ustat.dev);
	SR_FREEMSG(stp);
	if (!vfsp) {
		return EINVAL;
	}
	if (error = VFS_ROOT(vfsp, &vp)) {
		return error;
	}
	error = VFS_STATVFS(vfsp, &statvfs);
	VN_RELE(vp);
	if (error) {
		return error;
	}
        if (statvfs.f_ffree > USHRT_MAX) {
		return EOVERFLOW;
	}
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc(stp->sr_gdpp->hetero != NO_CONV ?
	   sizeof(struct ustat) + USTAT_XP : sizeof(struct ustat),
	    stp->sr_vcver);
	rp = RF_RESP(stp->sr_out_bp);
	rpdata = rf_msgdata(stp->sr_out_bp, RF_MIN_RESP(stp->sr_vcver));
	if (canon) {
		usp = &ustat;
	} else {
		usp = (struct ustat *)rpdata;
	}
	usp->f_tfree =
	  (daddr_t)(statvfs.f_bfree * (statvfs.f_frsize/512));
	usp->f_tinode = (o_ino_t)statvfs.f_ffree;
	bcopy(&statvfs.f_fstr[0], usp->f_fpack, sizeof(usp->f_fpack));
	bcopy(&statvfs.f_fstr[sizeof(usp->f_fpack)], usp->f_fname,
	  sizeof(usp->f_fname));
	if (canon) {
		rp->rp_count = rf_tcanon(USTAT_FMT, (caddr_t)usp, rpdata);
	}
	rp->rp_copyout.buf = cbuf;
	return error;
}

/*
 * VOP_WRITE and VOP_PUTPAGE, as well as system call protocol.
 *
 * TO DO:  remove overloading
 *
 * Client-side write(2) system calls and kernel-generated client writes
 * are handled here.  Different functions were used in SVR3, primarily
 * to keep accounting straight, but we share code here, and just check
 * the opcode for accounting.  By combining the write and writei ops, we
 * also allow client kernels to write to character devices, like network
 * connections, for example.
 *
 * We do explicit data movement, staying out of rf_server() hooks in
 * copy(in|out), (f|s)u(byte|word), etc.  The intent is that only ioctls
 * still go through those, and that only because drivers know about data
 * direction and format, and we don't.
 *
 * If intermediate data movement messages are required, we issue DUCOPYIN
 * messages.
 */
STATIC int
rfsr_write(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	/* base is updated for old clients. */

	rf_request_t		*req = RF_REQ(stp->sr_in_bp);
	register vnode_t	*vp = stp->sr_rdp->rd_vp;
	caddr_t			base = (caddr_t)req->rq_xfer.base;
	register vtype_t	vtype = vp->v_type;
	unsigned		ioflag = 0;
	off_t			apoffset;	/* remember for appends */
	vattr_t			vattr;
	uio_t			uio;
	iovec_t			iovec;
	register int		error = 0;
	long			prewrite = req->rq_xfer.prewrite;

	if (stp->sr_opcode == RFPUTPAGE) {
		rfsr_fsinfo.fsivop_putpage++;
		if (vp->v_flag & VNOMAP) {
			SR_FREEMSG(stp);
			return ENOSYS;
		}
	} else {
		rfsr_fsinfo.fsivop_write++;
	}
	uio.uio_iov = &iovec;
	if ((error = rfsr_rdwrinit(stp, &uio, vp, &ioflag, ctrlp)) != 0 ||
	  *ctrlp != SR_NORMAL) {
		SR_FREEMSG(stp);
		return error;
	}
	/*
	 * If there's no prewritten data free the incoming message
	 * block.
	 */
	if (!prewrite) {
		SR_FREEMSG(stp);	/* NULLs stp->sr_in_bp */
	}
	if (uio.uio_fmode & FAPPEND) {
		vattr.va_mask = AT_SIZE;
		if (error = VOP_GETATTR(vp, &vattr, 0, stp->sr_cred)) {

			/* multihop, net down, e.g. */

			SR_FREEMSG(stp);
			return error;
		}
		apoffset = uio.uio_offset = vattr.va_size;
	} else {
		uio.uio_offset = req->rq_xfer.offset;
	}
	if (vtype == VCHR && stp->sr_ret_val > prewrite) {

		/* special code to handle big writes to raw devices */

		error = rfsr_rawwrite(stp, vp, &uio, ioflag, base);
	} else {
		error = rfsr_cookedwrite(stp, vp, &uio, ioflag, base);
	}
	ASSERT(!stp->sr_out_bp);
	stp->sr_out_bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
	if (stp->sr_vcver < RFS2DOT0 && !error) {
		rf_response_t	*rp;

		/* Only old clients look at rdwr.isize */

		rp = RF_RESP(stp->sr_out_bp);
		if (uio.uio_fmode & FAPPEND) {

			/*
			 * Protocol history:  client adjusts f_offset,
			 * and the upper level of the inode kernel wants
			 * to add the number of characters moved.
			 */

			rp->rp_rdwr.isize = apoffset;
		} else if (!(error = VOP_GETATTR(vp, &vattr, 0, stp->sr_cred))) {
			rp->rp_rdwr.isize = vattr.va_size;
		}
	}
	return error;
}

/*
 * VOP_GETPAGE
 *
 * Client-generated page faults are handled here.  We don't just process
 * them as reads because page faults cannot be allowed to lock vnodes.
 * Otherwise, a deadlock can happen.
 *
 * If more than one data movement message is required, we send all but the
 * last as RFCOPYOUT messages.  The last is left in stp->out_bp, for later
 * processing in the server; that message will have an RFGETPAGE opcode.  The
 * number of data bytes in that message is in overloaded rp->rp_count.
 */
/* ARGSUSED */
STATIC int
rfsr_getpage(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t *ctrlp;
{
	register rf_request_t *req = RF_REQ(stp->sr_in_bp);
	vnode_t		*vp = stp->sr_rdp->rd_vp;
	off_t		resid = req->rq_xfer.count;
	off_t		oresid = resid;
	off_t		offset = req->rq_xfer.offset;
	size_t		datasz = stp->sr_gdpp->datasz;
	register int	error = 0;
	unsigned	disabled;	/* flag cache is disabled */
	rd_user_t	*rdup = rdu_find(stp->sr_rdp,
			  u.u_procp->p_sysid, u.u_srchan->sd_mntid,
			  (rd_user_t **)NULL);

	ASSERT(rdup);
	rfsr_fsinfo.fsivop_getpage++;
	SR_FREEMSG(stp);

	/*
	 *  return value of IO requests is residual char count
	 */

	stp->sr_oldoffset = offset;		/* for caching */
	if (resid <= 0) {
		return EINVAL;
	}
	if (vp->v_flag & VNOMAP) {
		return ENOSYS;
	}
	disabled = stp->sr_srmp->srm_flags & SRM_CACHE &&
		   rdup->ru_cflag & RU_CACHE_DISABLE;

	/*
	 * Iteratively lock down ranges of addresses, esballoc streams
	 * message block to cover the data, and ship it.
	 *
	 * We don't even try to prevent faulting beyond EOF, because the vnode
	 * is not held locked, so the file can change under us.
	 */

	do {
		register off_t	mboff;	/* MAXBOFFSET aligned file offset */
		register off_t	mbon;	/* for first iteration, unaligned
					 * offset in excess of mboff */
		register size_t	nbytes;	/* rfesb_fbread request */
		rf_response_t	*rp;
		rf_common_t	*cop;

		mboff = offset & MAXBMASK;
		mbon = offset & MAXBOFFSET;
		nbytes = MIN(datasz, MIN(MAXBSIZE - mbon, resid));
		offset += nbytes;
		resid -= nbytes;

		/*
		 * Allocate a streams message without data buffer to
		 * avoid a copy, sending the transport provider enough
		 * information to enqueue the cleanup work for the rf_daemon.
		 * We go to this trouble because we don't want the provider
		 * to sleep in fbrelse.
		 */

		ASSERT(!stp->sr_out_bp);
		if ((error = rfesb_fbread(vp, mboff + mbon, nbytes, S_OTHER,
		  RF_MIN_RESP(stp->sr_vcver), BPRI_MED, FALSE,
		  &stp->sr_out_bp)) != 0) {
			break;
		}

		rp = RF_RESP(stp->sr_out_bp);
		rp->rp_count = nbytes;
		rp->rp_nodata = FALSE;

		cop = RF_COM(stp->sr_out_bp);
		cop->co_type = RF_RESP_MSG;
		if (resid) {
			cop->co_opcode = RFCOPYOUT;
			error = rf_sndmsg(u.u_srchan, stp->sr_out_bp,
			  RF_MIN_RESP(stp->sr_vcver) +
			  (size_t)nbytes, (rcvd_t *)NULL, FALSE);
			stp->sr_out_bp = NULL;

		}

		if (disabled) {
			rfc_info.rfci_dis_data++;
		}

	} while (!error && resid);

	stp->sr_ret_val = oresid - resid;
	return error;
}	/* rfsr_getpage */

/*
 * complains about opcode in arg, returns error
 */
/* ARGSUSED */
int
rfsr_undef_op(stp, ctrlp)
	register rfsr_state_t	*stp;
	register rfsr_ctrl_t	*ctrlp;
{
	return rfsr_discon("rfs server undefined op", stp);
}

/* indexed by opcode */
int (*rfsr_ops[])() = {
	rfsr_setfl,	/* 0 RFSETFL */
	rfsr_unmap,	/* 1 RFUNMAP */
	rfsr_map,	/* 2 RFMAP */
	rfsr_read,	/* 3 RFREAD */
	rfsr_write,	/* 4 RFWRITE */
	rfsr_open,	/* 5 DUOPEN */
	rfsr_close,	/* 6 DUCLOSE */
	rfsr_lookup,	/* 7 RFLOOKUP */
	rfsr_create,	/* 8 RFSCREATE */
	dusr_link,	/* 9 DULINK */
	dusr_unlink,	/* 10 DUUNLINK */
	dusr_exec,	/* 11 DUEXEC */
	dusr_chdirec,	/* 12 DUCHDIR */
	rfsr_write,	/* 13 RFPUTPAGE */
	dusr_mknod,	/* 14 DUMKNOD */
	dusr_chmod,	/* 15 DUCHMOD */
	dusr_chown,	/* 16 DUCHOWN */
	rfsr_getpage,	/* 17 RFGETPAGE */
	dusr_stat,	/* 18 DUSTAT */
	dusr_seek,	/* 19 DUSEEK */
	rfsr_getattr,	/* 20 RFGETATTR */
	rfsr_undef_op,	/* 21 UNUSED */
	rfsr_undef_op,	/* 22 UNUSED */
	rfsr_setattr,	/* 23 RFSETATTR */
	rfsr_access,	/* 24 RFACCESS */
	rfsr_undef_op,	/* 25 UNUSED */
	rfsr_undef_op,	/* 26 UNUSED */
	rfsr_undef_op,	/* 27 UNUSED */
	dusr_fstat,	/* 28 DUFSTAT */
	rfsr_undef_op,	/* 29 UNUSED */
	dusr_utime,	/* 30 DUUTIME */
	rfsr_undef_op,	/* 31 UNUSED */
	rfsr_undef_op,	/* 32 UNUSED */
	dusr_saccess,	/* 33 DUSACCESS - access system call */
	rfsr_undef_op,	/* 34 UNUSED */
	dusr_statfs,	/* 35 DUSTATFS */
	rfsr_undef_op,	/* 36 UNUSED */
	rfsr_undef_op,	/* 37 UNUSED */
	dusr_fstatfs,	/* 38 DUFSTATFS */
	rfsr_undef_op,	/* 39 UNUSED */
	rfsr_rename,	/* 40 RFRENAME */
	rfsr_undef_op,	/* 41 UNUSED */
	rfsr_undef_op,	/* 42 UNUSED */
	rfsr_undef_op,	/* 43 UNUSED */
	rfsr_undef_op,	/* 44 UNUSED */
	rfsr_undef_op,	/* 45 UNUSED */
	rfsr_undef_op,	/* 46 UNUSED */
	rfsr_undef_op,	/* 47 UNUSED */
	rfsr_undef_op,	/* 48 UNUSED */
	rfsr_undef_op,	/* 49 UNUSED */
	rfsr_undef_op,	/* 50 UNUSED */
	rfsr_undef_op,	/* 51 UNUSED */
	rfsr_undef_op,	/* 52 UNUSED */
	rfsr_undef_op,	/* 53 UNUSED */
	rfsr_ioctl,	/* 54 DUIOCTL */
	rfsr_undef_op,	/* 55 UNUSED */
	rfsr_undef_op,	/* 56 UNUSED */
	rfsr_ustat,	/* 57 RFUSTAT */
	rfsr_fsync,	/* 58 RFFSYNC */
	dusr_exec,	/* 59 DUEXECE */
	rfsr_undef_op,	/* 60 UNUSED */
	dusr_chdirec,	/* 61 DUCHROOT */
	dusr_fcntl,	/* 62 DUFCNTL */
	rfsr_space,	/* 63 RFSPACE */
	rfsr_frlock,	/* 64 RFFRLOCK */
	rfsr_undef_op,	/* 65 UNUSED */
	rfsr_undef_op,	/* 66 UNUSED */
	rfsr_undef_op,	/* 67 UNUSED */
	rfsr_undef_op,	/* 68 UNUSED */
	rfsr_undef_op,	/* 69 UNUSED */
	rfsr_undef_op,	/* 70 UNUSED */
	rfsr_undef_op,	/* 71 UNUSED */
	dusr_rmount,	/* 72 DURMOUNT */
	rfsr_undef_op,	/* 73 UNUSED */
	rfsr_undef_op,	/* 74 UNUSED */
	rfsr_undef_op,	/* 75 UNUSED */
	rfsr_undef_op,	/* 76 UNUSED */
	rfsr_undef_op,	/* 77 UNUSED */
	rfsr_undef_op,	/* 78 UNUSED */
	rfsr_rmdir,	/* 79 RFRMDIR */
	rfsr_mkdir,	/* 80 RFMKDIR */
	rfsr_readdir,	/* 81 RFREADDIR */
	rfsr_undef_op,	/* 82 UNUSED */
	rfsr_undef_op,	/* 83 UNUSED */
	rfsr_undef_op,	/* 84 UNUSED */
	rfsr_undef_op,	/* 85 UNUSED */
	rfsr_undef_op,	/* 86 UNUSED */
	rfsr_undef_op,	/* 87 UNUSED */
	rfsr_undef_op,	/* 88 UNUSED */
	rfsr_symlink,	/* 89 RFSYMLINK */
	rfsr_readlink,	/* 90 RFREADLINK */
	rfsr_undef_op,	/* 91 UNUSED */
	rfsr_undef_op,	/* 92 UNUSED */
	rfsr_undef_op,	/* 93 UNUSED */
	rfsr_undef_op,	/* 94 UNUSED */
	rfsr_undef_op,	/* 95 UNUSED */
	rfsr_undef_op,	/* 96 UNUSED */
	rfsr_mount,	/* 97 DUSRMOUNT */
	rfsr_umount,	/* 98 DUSRUMOUNT */
	rfsr_undef_op,	/* 99 UNUSED */
	rfsr_undef_op,	/* 100 UNUSED */
	rfsr_undef_op,	/* 101 UNUSED */
	rfsr_undef_op,	/* 102 UNUSED */
	rfsr_statvfs,	/* 103 RFSTATVFS */
	rfsr_undef_op,	/* 104 UNUSED */
	rfsr_undef_op,	/* 105 UNUSED */
	rfsr_undef_op,	/* 106 RFCOPYIN */
	rfsr_undef_op,	/* 107 RFCOPYOUT */
	rfsr_undef_op,	/* 108 UNUSED */
	rfsr_link,	/* 109 RFLINK */
	rfsr_undef_op,	/* 110 UNUSED */
	dusr_coredump,	/* 111 DUCOREDUMP */
	rfsr_write,	/* 112 DUWRITEI */
	rfsr_read,	/* 113 DUREADI */
	rfsr_undef_op,	/* 114 UNUSED */
	rfsr_undef_op,	/* 115 UNUSED */
	rfsr_undef_op,	/* 116 UNUSED */
	rfsr_undef_op,	/* 117 UNUSED */
	rfsr_undef_op,	/* 118 UNUSED */
	rfsr_rsignal,	/* 119 RFRSIGNAL */
	rfsr_undef_op,	/* 120 UNUSED */
	rfsr_undef_op,	/* 121 UNUSED */
	rfsr_undef_op,	/* 122 RFSYNCTIME */
	rfsr_undef_op,	/* 123 UNUSED */
	rfsr_undef_op,	/* 124 RFDOTDOT */
	rfsr_undef_op,	/* 125 UNUSED */
	rfsr_undef_op,	/* 126 RFFUMOUNT */
	rfsr_undef_op,	/* 127 DUSENDUMSG */
	rfsr_undef_op,	/* 128 RFGETUMSG */
	rfsr_remove,	/* 129 RFREMOVE */
	rfsr_undef_op,	/* 130 UNUSED */
	rfsr_inactive,	/* 131 RFINACTIVE */
	dusr_iupdate,	/* 132 DUIUPDATE */
};
