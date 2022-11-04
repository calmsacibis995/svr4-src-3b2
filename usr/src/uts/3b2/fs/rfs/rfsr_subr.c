#ident	"@(#)fs:fs/rfs/rfsr_subr.c	1.10.1.1 UNOFFICIAL"

/*
 * Support operations for the RFS server daemon.
 */
#include "sys/list.h"
#include "sys/types.h"
#include "sys/vnode.h"
#include "sys/vfs.h"
#include "sys/sysmacros.h"
#include "sys/param.h"
#include "sys/systm.h"
#include "sys/mode.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/stream.h"
#include "vm/seg.h"
#include "rf_admin.h"
#include "sys/rf_comm.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/siginfo.h"
#include "sys/nserve.h"
#include "sys/rf_cirmgr.h"
#include "sys/idtab.h"
#include "sys/var.h"
#include "sys/file.h"
#include "sys/pathname.h"
#include "sys/fstyp.h"
#include "sys/fcntl.h"
#include "sys/proc.h"
#include "sys/stat.h"
#include "sys/inline.h"
#include "sys/debug.h"
#include "sys/cmn_err.h"
#include "sys/conf.h"
#include "sys/buf.h"
#include "sys/rf_adv.h"
#include "sys/uio.h"
#include "sys/fs/rf_vfs.h"
#include "sys/rf_messg.h"
#include "sys/dirent.h"
#include "rf_serve.h"
#include "sys/statfs.h"
#include "sys/statvfs.h"
#include "rf_auth.h"
#include "rfcl_subr.h"
#include "rf_cache.h"
#include "sys/hetero.h"
#include "rf_canon.h"
#include "sys/sysinfo.h"
#include "sys/kmem.h"
#include "sys/fs/rf_acct.h"

/* imports */
extern int	strcmp();
extern char	*kseg();
extern void	unkseg();
extern void	ovbcopy();
extern void	exit();

STATIC void	rfsr_rmlist();
STATIC int	getcopyinmsg();
STATIC int	sendcopyinmsg();
STATIC void	rfsr_adj_time();
STATIC mblk_t	*reset_len();
STATIC mblk_t	*reset_path();
STATIC void	rfsr_dev_remove();
STATIC vfs_t	*rfsr_dev_vfs_search();
STATIC int	rfsr_dev_find();
STATIC int	rfsr_dev_insert();
STATIC void	rfsr_dev_clean();
STATIC int	rfsr_writev();

/*
 * Translate pathname to vnode, or path crosses back to client,
 * or error.
 * Handle allocation and freeing of pathname buffer, return error.
 */
int
rfsr_lookupname(followlink, stp, dirvpp, compvpp, sr_ctrlp)
	symfollow_t	followlink;	/* follow sym links */
	rfsr_state_t	*stp;		/* state struct assoc with request */
	vnode_t		**dirvpp;	/* ret for ptr to parent dir vnode */
	vnode_t		**compvpp;	/* ret for ptr to component vnode */
	rfsr_ctrl_t	*sr_ctrlp;
{
	pathname_t	lookpn;
	register int	error;
	size_t		hdrsz = RF_MIN_REQ(stp->sr_vcver);

	if ((error = RF_PULLUP(stp->sr_in_bp, hdrsz,
	  (size_t)RF_MSG(stp->sr_in_bp)->m_size - hdrsz)) != 0) {
		return error;
	}
	if ((error =
	  pn_get(rf_msgdata(stp->sr_in_bp, hdrsz), UIO_SYSSPACE, &lookpn))
	  != 0) {
		return error;
	}
	error = rfsr_lookuppn(&lookpn, followlink, stp, dirvpp,
	  compvpp, sr_ctrlp);
	pn_free(&lookpn);
	return error;
}

/*
 * Starting at current directory, translate pathname pnp to end.
 * Leave pathname of final component in pnp, return the vnode
 * for the final component in *compvpp, and return the vnode
 * for the parent of the final component in dirvpp.
 *
 * This is the central routine in pathname translation and handles
 * multiple components in pathnames, separating them at /'s.  It also
 * implements mounted file systems and processes symbolic links.
 *
 * Allocates response if pathname crosses back to client.
 */
int
rfsr_lookuppn(pnp, followlink, stp, dirvpp, compvpp, sr_ctrlp)
	register pathname_t *pnp;	/* pathname to lookup */
	symfollow_t followlink;		/* (don't) follow sym links */
	rfsr_state_t *stp;		/* state struct assoc with request */
	vnode_t **dirvpp;		/* ptr for parent vnode */
	vnode_t **compvpp;		/* ptr for entry vnode */
	rfsr_ctrl_t *sr_ctrlp;
{
	register vnode_t *vp;		/* current directory vp */
	register vnode_t *cvp;		/* current component vp */
	vnode_t *tvp;			/* addressable temp ptr */
	register vfs_t *vfsp;		/* ptr to vfs for mount indir */
	char component[MAXNAMELEN];	/* buffer for component */
	register int error = 0;
	register int nlink;
	int lookup_flags;
	vnode_t *rsc_rootvp;
	vnode_t *rdir;			/* root directory */

	nlink = 0;
	cvp = NULLVP;
	lookup_flags = dirvpp ? LOOKUP_DIR : 0;
	rsc_rootvp = stp->sr_rsrcp->r_rootvp;

	/*
	 * start at current directory.
	 */
	vp = stp->sr_rdp->rd_vp;
	VN_HOLD(vp);

	/* To avoid special checks
	 */
	rdir = RF_REQ(stp->sr_in_bp)->rq_rrdir ?
		rcvd[RF_REQ(stp->sr_in_bp)->rq_rrdir].rd_vp : rootdir;
begin:
	/*
	 * Each time we begin a new name interpretation (e.g.
	 * when first called and after each symbolic link is
	 * substituted), we allow the search to start at the
	 * root directory if the name starts with a '/', otherwise
	 * continuing from the current directory.
	 */
	component[0] = 0;
	if (pn_peekchar(pnp) == '/') {
		VN_RELE(vp);
		pn_skipslash(pnp);
		vp = rdir;
		VN_HOLD(vp);
	}

next:
	/*
	 * Make sure we have a directory.
	 */
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto bad;
	}
	/*
	 * Process the next component of the pathname.
	 */
	error = pn_stripcomponent(pnp, component);
	if (error)
		goto bad;

	/*
	 * Check for degenerate name (e.g. / or "")
	 * which is a way of talking about a directory,
	 * e.g. "/." or ".".
	 */
	if (!component[0]) {
		/*
		 * If the caller was interested in the parent then
		 * return an error since we don't have the real parent
		 */
		if (dirvpp) {
			VN_RELE(vp);
			return EINVAL;
		}
		(void) pn_set(pnp, ".");
		if (compvpp)
			*compvpp = vp;
		else
			VN_RELE(vp);
		return 0;
	}

	/*
	 * Handle "..": three special cases.
	 * 1. If at root directory (e.g. after chroot)
	 *    then ignore it so can't get out.
	 * 2. If this vnode is root of RFS resource mounted by
	 *    by client, there are two possibilities. If it was
	 *    a result of symbolic link evaluation, then return
	 *    EPATHREVAL to client if client is RFS2DOT0,
	 *    or return EACCES if client is pre-RFS2DOT0.
	 *    Otherwise, set error to EDOTDOT and return so
	 *    client can finish evaluating pathname.
	 * 3. If this vnode is the root of a mounted
	 *    file system, then replace it with the
	 *    vnode which was mounted on so we take the
	 *    .. in the other file system.
	 */
	if (strcmp(component, "..") == 0) {
checkforroot:
		if (rsc_rootvp == vp) {
			pathname_t dotdot;

			if (stp->sr_vcver < RFS2DOT0 && nlink) {
				/*
				 * old clients can't handle symbolic links.
				 */
				error = EACCES;
				goto bad;
			}
			pn_alloc(&dotdot);
			pn_set(&dotdot, "..");
			pn_insert(pnp, &dotdot);
			pn_free(&dotdot);
			if (stp->sr_vcver < RFS2DOT0 || nlink) {
				/*
				 * Allocate a response and copy the remaining
				 * path into the data part.
			 	 */
				ASSERT(!stp->sr_out_bp);
				stp->sr_out_bp = reset_path(stp, pnp,
				  nlink ? RFPATHREVAL : RFDOTDOT);
			} else {
				/*
				 * The client merely needs to adjust its
				 * pn_path pointer, it uses our pathlen
				 * to do so.
				 */
				ASSERT(!stp->sr_out_bp);
				stp->sr_out_bp = reset_len(pnp, RFDOTDOT, stp);
			}
			*sr_ctrlp = SR_PATH_RESP;
			goto bad;
		}
		if (VN_CMP(vp, rdir)) {
			cvp = vp;
			VN_HOLD(cvp);
			goto skip;
		}
		if (vp->v_flag & VROOT) {
			cvp = vp;
			vp = vp->v_vfsp->vfs_vnodecovered;
			VN_HOLD(vp);
			VN_RELE(cvp);
			cvp = NULLVP;
			goto checkforroot;
		}
	}

	/*
	 * Perform a lookup in the current directory.
	 */
	error = VOP_LOOKUP(vp, component, &tvp, pnp, lookup_flags,
			rdir, stp->sr_cred);
	cvp = tvp;
	if (error) {
		cvp = NULLVP;
		/*
		 * On error, if more pathname or if caller was not interested
		 * in the parent directory then hard error.
		 * If the path is unreadable, fail now with the right error.
		 */
		if (pn_pathleft(pnp) || !dirvpp || error == EACCES) {
			if (error == ENOENT) {
				ASSERT(!stp->sr_out_bp);
				stp->sr_out_bp =
					reset_len(pnp, stp->sr_opcode, stp);
			}
			goto bad;
		}
		pn_setlast(pnp);
		*dirvpp = vp;
		if (compvpp)
			*compvpp = NULLVP;
		return 0;
	}
	/*
	 * If we hit a symbolic link and there is more path to be
	 * translated or this operation does not wish to apply
	 * to a link, then place the contents of the link at the
	 * front of the remaining pathname.
	 */
	if (cvp->v_type == VLNK && (followlink == FOLLOW || pn_pathleft(pnp))) {
		pathname_t linkpath;

		nlink++;
		if (nlink > MAXSYMLINKS) {
			error = ELOOP;
			goto bad;
		}
		pn_alloc(&linkpath);
		error = pn_getsymlink(cvp, &linkpath, stp->sr_cred);
		if (error) {
			goto bad;
		}
		if (!pn_pathleft(&linkpath)) {
			(void) pn_set(&linkpath, ".");
		}
		error = pn_insert(pnp, &linkpath);	/* linkpath before pn */
		pn_free(&linkpath);
		if (error) {
			goto bad;
		}
		if (pn_peekchar(pnp) ==  '/') {
			/*
			 * Obtained absolute path. New path has to
			 * be sent back to the client for re-eval.
			 * Allocate the message structure and copy new path
			 * into the data part of the message. If client is
			 * older than RFS2DOT0, set error to EACCES. Otherwise
			 * set it to EPATHREVAL.
			 */
			if(stp->sr_gdpp->version < RFS2DOT0){
				error = EACCES;
			} else {
				ASSERT(!stp->sr_out_bp);
				stp->sr_out_bp = reset_path(stp, pnp,
				  RFPATHREVAL);
				*sr_ctrlp = SR_PATH_RESP;
			}
			goto bad;
		}
		VN_RELE(cvp);
		cvp = NULLVP;
		goto next;
	}

	/*
	 * If this vnode is mounted on, then we
	 * transparently indirect to the vnode that
	 * is the root of the mounted file system.
	 * Before we do this we must check that an unmount is not
	 * in progress on this vnode.
	 * If it is an RFS resource which is mounted, set
	 * error to EMULTIHOP.
	 */
	while ((vfsp = cvp->v_vfsmountedhere) != NULL) {
		if (ISRFSVFSP(vfsp)) {
			error = EMULTIHOP;
			goto bad;
		}
		if (vfsp->vfs_flag & VFS_MLOCK) {
			vfsp->vfs_flag |= VFS_MWAIT;
			(void) sleep((caddr_t)vfsp, PVFS);
			continue;
		}
		if (error = VFS_ROOT(cvp->v_vfsmountedhere, &tvp))
			goto bad;
		VN_RELE(cvp);
		cvp = tvp;
	}

skip:
	/*
	 * Skip to next component of the pathname.
	 * If no more components, return last directory (if wanted) and
	 * last component (if wanted).
	 */
	if (!pn_pathleft(pnp)) {
		pn_setlast(pnp);
		if (dirvpp) {
			/*
			 * check that we have the real parent and not
			 * an alias of the last component
			 */
			if (VN_CMP(vp, cvp)) {
				VN_RELE(vp);
				VN_RELE(cvp);
				return EINVAL;
			}
			*dirvpp = vp;
		} else {
			VN_RELE(vp);
		}
		if (compvpp) {
			*compvpp = cvp;
		} else {
			VN_RELE(cvp);
		}
		return 0;
	}
	/*
	 * skip over slashes from end of last component
	 */
	pn_skipslash(pnp);

	/*
	 * Searched through another level of directory:
	 * release previous directory handle and save new (result
	 * of lookup) as current directory.
	 */
	VN_RELE(vp);
	vp = cvp;
	cvp = NULLVP;
	goto next;

bad:
	/*
	 * Error. Release vnodes and return.
	 */
	if (cvp)
		VN_RELE(cvp);
	VN_RELE(vp);
	return error;
}

/*
 * Allocate a return message structure, copy new path into the data
 * portion of the message and return to client for further resolution.
 */
STATIC mblk_t *
reset_path(stp, pnp, opcode)
	register rfsr_state_t	*stp;
	register pathname_t	*pnp;
	int			opcode;
{
	register mblk_t		*bp;
	register rf_common_t	*cop;

	bp = rfsr_rpalloc(pnp->pn_pathlen + 1, stp->sr_vcver);
	bcopy(pnp->pn_path, rf_msgdata(bp, RF_MIN_RESP(stp->sr_vcver)),
	  pnp->pn_pathlen + 1);
	/*
	 * Set co_mntid to client mount id.
	 */
	cop = RF_COM(bp);
	cop->co_mntid = stp->sr_srmp->srm_mntid;
	cop->co_opcode = opcode;
	return bp;
}

/*
 * Copy length of remaining path into allocated response and return to client
 * for further resolution.
 */
STATIC mblk_t *
reset_len(pnp, opcode, stp)
	pathname_t		*pnp;
	int			opcode;
	register rfsr_state_t	*stp;
{
	register mblk_t		*bp;
	register rf_common_t	*cop;

	bp = rfsr_rpalloc((size_t)0, stp->sr_vcver);
	cop = RF_COM(bp);
	RF_RESP(bp)->rp_v2gift.pathlen = pnp->pn_pathlen;
	cop->co_mntid = stp->sr_srmp->srm_mntid;
	cop->co_opcode = opcode;
	return bp;
}

/*
 * Make a gift to give to a client, updating srmount and other
 * reference counts.
 * Stp is a server state structure, vp the file we're making
 * a gift of, out_port the channel back to the client.
 * stp->sr_gift must be NULL.  Creates a reference to a gift rcvd and
 * assigns stp->sr_gift * to refer to it.
 * Assumes that stp->sr_out_bp refers to an allocated message.
 * Assigns several fields in the response message.
 *
 * Return 0 for success, nonzero errno for failure.
 *
 * NOTE: srmnt() depends on this routine not sleeping until after giftrd
 * has been "held", i.e. its reference count accounts for this new use.
 */
int
rfsr_gift_setup(stp, vp, out_port)
	register rfsr_state_t	*stp;
	register vnode_t	*vp;
	register sndd_t		*out_port;	/* channel back to client */
{
	register sysid_t	sysid = SDTOSYSID(out_port);
	register int		error;
	register rd_user_t	*rdup;
	register rf_response_t	*resp = RF_RESP(stp->sr_out_bp);
	register rf_common_t	*cop = RF_COM(stp->sr_out_bp);
	rcvd_t			*giftrd = vtord(vp);
	vattr_t			vattr;

	/* if sd went bad, forget it */
	if (out_port->sd_stat & SDLINKDOWN) {
		return ENOLINK;
	}
	ASSERT(!stp->sr_gift);
	if (giftrd) {
		/*
		 * Some client has a reference to this file, or it's a
		 * resource root.
		 *
		 * Kick the ref count, which the client will undo later if
		 * the reference is redundant (for that client).
		 */
		giftrd->rd_refcnt++;
	} else {
		/*
		 * First remote reference to this file.
		 */
		if (error = rcvd_create(TRUE, RDGENERAL, &giftrd)) {
			return error;
		}
		giftrd->rd_vp = vp;
	}
	/* keep track of who we're giving it to
	 */
	if ((rdup = rdu_get(giftrd, sysid, out_port->sd_mntid,
	  out_port->sd_queue)) == NULL) {
		rcvd_delete(&giftrd, (sysid_t)0, (long)-1);
		return ENOSPC;
	}
	vattr.va_mask =
		AT_UID | AT_GID | AT_NLINK | AT_SIZE | AT_MODE | AT_VCODE;
	if ((error = VOP_GETATTR(vp, &vattr, 0, stp->sr_cred)) != 0) {
		rdu_del(giftrd, sysid, out_port->sd_mntid);
		rcvd_delete(&giftrd, sysid, out_port->sd_mntid);
		return error;
	}
	stp->sr_srmp->srm_refcnt++;

	cop->co_nlink = vattr.va_nlink;
	cop->co_uid = vattr.va_uid;
	cop->co_gid = vattr.va_gid;
	cop->co_ftype = VTTOIF(vp->v_type);
	cop->co_size = vattr.va_size;
	resp->rp_fhandle = (long)vp;
	if (stp->sr_vcver >= RFS2DOT0) {
		resp->rp_v2gift.flags = 0;
		if (vp->v_flag & VNOMAP) {
			resp->rp_v2gift.flags |= RPG_NOMAP;
		}
	}
	if (stp->sr_srmp->srm_flags & SRM_CACHE && vp->v_type == VREG) {
		if (!MANDLOCK(vp, vattr.va_mode) &&
	  	  (lbolt - giftrd->rd_mtime > rfc_time ||
		   lbolt < giftrd->rd_mtime)) {
			rdup->ru_cflag |= RU_CACHE_ON;
			rdup->ru_cflag &= ~RU_CACHE_DISABLE;
		}
		if (rdup->ru_cflag & RU_CACHE_ON) {
			if (stp->sr_vcver >= RFS2DOT0) {
				resp->rp_v2vcode = vattr.va_vcode;
			} else if (stp->sr_opcode == RFCREATE ||
			  stp->sr_opcode == RFOPEN) {
				/*
				 * 3.2 clients only expect cacheing
				 * info with gifts obtained via open
				 * and create.  rp_rval has
				 * different uses otherwise.
				 */
				resp->rp_cache = DU_CACHE_ENABLE;
				stp->sr_ret_val = vattr.va_vcode;
			}
		}
	}
	stp->sr_gift = giftrd;
	return 0;
}

/*
 * Search the denoted receive descriptor for a message block from the
 * client indicated by sysid, the process denoted by pid.
 * If such a message is found, remove it from the receive descriptor
 * queue and return a pointer to it; otherwise, return NULL.
 *
 * Runs at splstr to keep from being trashed by ciruit manager.
 */
mblk_t *
rfsr_chkrdq(rdp, pid, sysid)
	register rcvd_t	*rdp;
	register long	pid;
	register long	sysid;
{
	register int	mi;		/* count messages in rdp queue */
	register int	qcnt;
	register mblk_t	*current;
	int		s;

	if (rdp->rd_stat != RDUSED) {

		/* VOP_INACTIVE could have beat signal */

		return NULL;
	}
	s = splstr();
	qcnt = rdp->rd_qcnt;
	current = (mblk_t *)rdp->rd_rcvdq.ls_next;
	for (mi = 0; mi < qcnt; current = current->b_next, mi++) {
		register rf_common_t	*cop = RF_COM(current);

		if (cop->co_pid == pid && cop->co_sysid == sysid) {
			rdp->rd_qcnt--;
			LS_REMOVE(current);
			if (RCVDEMP(rdp)) {
				rfsr_rmmsg(rdp);
			}
			splx(s);
			return current;
		}
	}
	splx(s);
	return NULL;
}

/*
 * Assumes dying server is on neither the idle or active list, and
 * contributes only to total server count and idle server count.
 * Frees credentials structure, u_srchan (if it's there), and
 * exits.
 *
 * No return.
 */
void
rfsr_exit(stp)
	rfsr_state_t *stp;
{
	register proc_t *procp = u.u_procp;
	register int s = splstr();

	ASSERT(rfsr_nservers > 0);
	procp->p_epid = 0;
	crfree(stp->sr_cred);
	sndd_free(&u.u_srchan);
	--rfsr_nservers;
	if (procp->p_curinfo) {
		kmem_free((caddr_t)procp->p_curinfo, sizeof(*procp->p_curinfo));
		procp->p_curinfo = NULL;
	}
	splx(s);
	exit(CLD_EXITED, 0);
}

/*
 * By convention, the server uses SIGUSR1 to abort operations when resources
 * are getting low and uses SIGTERM to represent any signal transmitted by
 * the client.
 */
/* ARGSUSED */
int
rfsr_sigck(ctrlp, in_error)
	rfsr_ctrl_t		*ctrlp;
	int			in_error;
{
	register int		out_error = 0;
	register struct proc	*p = u.u_procp;
	register k_sigset_t	*kp = &p->p_sig;

	if((sigismember(kp, SIGUSR1) || p->p_cursig == SIGUSR1) &&
	  !sigismember(kp, SIGTERM) && p->p_cursig != SIGTERM &&
	  in_error == EINTR) {
		/*
		 * We infer from SIGUSR1 and EINTR that we refused to sleep
		 * because we were low on resources.  Lack of SIGTERM means
		 * that the requesting client didn't break the operation.
		 * Turn off the SIGUSR1 and NACK the request.  (ENOMEM is
		 * part of the NACK).
		 */
		rf_delsig(p, SIGUSR1);
		out_error = ENOMEM;
		*ctrlp = SR_NACK_RESP;
	}
	rf_delsig(p, SIGTERM);
	if (!out_error) {
		out_error = in_error;
	}
	return out_error;
}

STATIC void
rfsr_adj_time(vap, gp)
	register vattr_t	*vap;
	register gdp_t		*gp;
{
	register long 		tskew_sec = gp->timeskew_sec;

	vap->va_atime.tv_sec += tskew_sec;
	vap->va_ctime.tv_sec += tskew_sec;
	vap->va_mtime.tv_sec += tskew_sec;
}


void
rfsr_adj_timeskew(gp, sec, nsec)
	register gdp_t	*gp;
	register long 	sec;
	register long	nsec;
{
	register long	my_sec = hrestime.tv_sec;
	register long   my_nsec = hrestime.tv_nsec;

	if (gp->version < RFS2DOT0) {
		gp->timeskew_sec = sec - my_sec;
	} else {
                /*
                 * If the nsec values vary by more than a half of a second,
                 * (500 million nanoseconds), then adjust sec.	For now
                 * we only record the timeskew in seconds.  Trying to use
                 * the nsecs too would make the variance in message transit
                 * time visible.
                 */
                if ((nsec - my_nsec) > 500000000) {
                        sec++;
		} else if ((my_nsec - nsec) > 500000000) {
                        sec--;
		}
                gp->timeskew_sec = sec - my_sec;
	}
}

/* Process request for big reads from raw devices.
 * Stp denotes the server state structure with sr_ret_val set to the
 * residual read count, and sr_cred holding the client credentials.
 * Vp denotes the file to be read, uiop a uio_t with associated
 * iovec.  Ioflag and base are also defined by the caller.
 * The last is the start of the io buffer on the client; we update
 * it for compatability, but clients should really ignore it.
 *
 * Return 0 for success, nonzero errno for failure.  In non-error cases,
 * leaves last message in stp->sr_out_bp, else sets it to NULL.
 */
int
rfsr_rawread(stp, vp, uiop, ioflag, base)
	register struct rfsr_state *stp;
	register vnode_t *vp;
	register uio_t *uiop;
	unsigned ioflag;
	register caddr_t base;
{
	/* Big read from a raw device.
	 * Get a buffer plausibly big enough for any
	 * character device record, and break on that amount.
	 *
	 * It may be the case that the medium was written with a
	 * bigger record size than we can accomodate here.  For
	 * disks, that should be no problem, but, for tapes, the
	 * read might fail.
	 *
	 **************************************************************
	 * NOTE TO PORTERS:  We use a segment's worth of contiguous
	 * memory for the 3B2, but that is obviously machine-dependent.
	 **************************************************************
	 */
	register unsigned	readsize = MIN(stp->sr_ret_val, NBPS);
	register caddr_t	lbase;		/* pointer to local buffer */
	int			totread = 0;	/* total bytes read */
	register int		error = 0;

	extern char		*kseg();
	extern void		unkseg();

	if ((lbase = kseg((int)btoc(readsize))) == NULL) {
		return ENOMEM;
	}
	while(stp->sr_ret_val && !error) {
		register size_t	nread;

		uiop->uio_iov->iov_base = lbase;
		uiop->uio_resid = uiop->uio_iov->iov_len = readsize;
		if (error = VOP_READ(vp, uiop, ioflag, stp->sr_cred)) {
			break;
		}
		nread = readsize - uiop->uio_resid;
		totread += nread;
		if (rcopyout(lbase, base, nread, &stp->sr_out_bp)) {
			error = EFAULT;
			break;
		} else {
			base += nread;
			stp->sr_ret_val -= nread;
			if (stp->sr_out_bp && stp->sr_ret_val && nread) {
				/*
				 * rcopyout thought this was the
				 * last message, left it for us, but
				 * we don't know that we're done, so
				 * send it.
				 */
				RF_COM(stp->sr_out_bp)->co_opcode = RFCOPYOUT;
				if (error =
				  rf_sndmsg(u.u_srchan, stp->sr_out_bp,
				   (size_t)(RF_MIN_RESP(stp->sr_vcver) +
				   RF_RESP(stp->sr_out_bp)->rp_count),
				   (rcvd_t *)NULL, FALSE)) {
					break;
				}
				stp->sr_out_bp = NULL;
			}
		}
		if (!nread) {
			break;
		}
	}
	unkseg(lbase);
	if (error) {
		/*
		 * Conserve large message blocks.
		 */
		SR_FREEMSG(stp);
	}
	stp->sr_srmp->srm_kbcnt += totread / 1024;
	return error;
}

/* Process all read operations but big ones from raw devices.
 * Stp denotes the server state structure with sr_ret_val set to the
 * residual read count, and sr_cred holding the client credentials.
 * Vp denotes the file to be read, uiop a uio_t with associated
 * iovec.  Ioflag, ftype and base are also defined by the caller.
 * The last is the start of the io buffer on the client; we update
 * it for compatability, but clients should really ignore it.
 *
 * Return 0 for success, nonzero errno for failure.  In non-error cases,
 * leaves last message in stp->sr_out_bp, else sets it to NULL.
 */
int
rfsr_cookedread(stp, vp, uiop, ioflag, base)
	register rfsr_state_t	*stp;
	register vnode_t	*vp;
	register uio_t		*uiop;
	unsigned		ioflag;
	caddr_t			base;
{
	unsigned		disabled;	/* flag if cache is disabled */
	int			totread = 0;
	int			error = 0;
	sndd_t			*sdp = u.u_srchan;	/* gag */

	size_t			datasz = stp->sr_gdpp->datasz;
	rd_user_t		*rdup = rdu_find(stp->sr_rdp,
				  u.u_procp->p_sysid, sdp->sd_mntid,
				  (rd_user_t **)NULL);

	ASSERT(rdup);
	VOP_RWLOCK(vp);
	disabled = stp->sr_srmp->srm_flags & SRM_CACHE &&
	  rdup->ru_cflag & RU_CACHE_DISABLE;
	while(stp->sr_ret_val && !error) {
		size_t		readsz;	/* requested from VOP_READ */
		rf_response_t	*rp;

		readsz = uiop->uio_resid = uiop->uio_iov->iov_len =
			MIN(stp->sr_ret_val, datasz);
		stp->sr_out_bp = rfsr_rpalloc(readsz, stp->sr_vcver);
		rp = RF_RESP(stp->sr_out_bp);
		uiop->uio_iov->iov_base =
		  rf_msgdata(stp->sr_out_bp, RF_MIN_RESP(stp->sr_vcver));
		if ((error = VOP_READ(vp, uiop, ioflag, stp->sr_cred)) == 0) {
			if (disabled) {
				rfc_info.rfci_dis_data++;
			}
			/*
			 * The size of the data transfer is the size request
			 * from the read less the residual count from the
			 * op.  The total residual count for the IO is reduced
			 * by the amount of data moved.
			 *
			 * The original protocol has the server doing client
			 * pointer arithmetic, not a wonderful idea.  We
			 * continue for compatability, but clients should
			 * ignore it.  (4.0 clients do.)
			 */
			rp->rp_count = readsz - uiop->uio_resid;
			rp->rp_nodata = !rp->rp_count;
			rp->rp_offset = uiop->uio_offset;
			rp->rp_copyout.buf = (long)base;
			base += rp->rp_count;
			totread += rp->rp_count;
			stp->sr_ret_val -= rp->rp_count;
			if (!stp->sr_ret_val ||
			  uiop->uio_resid &&
			   (vp->v_type == VREG || vp->v_type == VBLK) ||
			  uiop->uio_resid == readsz) {
				break;
			}
			/*
			 * Assume there is more data to come and send the
			 * message.
			 */
			RF_COM(stp->sr_out_bp)->co_opcode = RFCOPYOUT;
			rp->rp_copyout.copysync = 0; /* relic of static queue */
			error = rf_sndmsg(sdp, stp->sr_out_bp,
			  RF_MIN_RESP(stp->sr_vcver) + (size_t)rp->rp_count,
			  (rcvd_t *)NULL, FALSE);
			stp->sr_out_bp = NULL;
		}
	}
	VOP_RWUNLOCK(vp);
	if (error) {
		/*
		 * Conserve large message blocks.
		 */
		SR_FREEMSG(stp);
	}
	stp->sr_srmp->srm_kbcnt += totread / 1024;
	return error;
}

/* Process request for big writes to raw devices.
 * Stp denotes the server state structure with sr_ret_val set to the
 * residual write count, and sr_cred holding the client credentials.
 * Vp denotes the file to be written, uiop a uio_t with associated
 * iovec.  Ioflag and base are also defined by the caller.
 * The last is the start of the io buffer on the client; we update
 * it for compatability, but clients should really ignore it.
 * Req is a pointer to a rf_request_t containing prewritten data,
 * or NULL.
 * Assumes stp->sr_ret_val is set to the residual write count; updates
 * it accordingly.
 *
 * Return 0 for success, nonzero errno for failure.
 */
int
rfsr_rawwrite(stp, vp, uiop, ioflag, base)
	register rfsr_state_t	*stp;
	register vnode_t	*vp;
	register uio_t		*uiop;
	unsigned		ioflag;
	register caddr_t	base;
{
	/*
	 * Big write to a raw device.
	 * Get a buffer plausibly big enough for any
	 * character device record, and break on that amount.
	 *
	 * For really big writes, we can end up writing types that
	 * can't be read on systems where you can't kseg enough
	 * memory.
	 *
	 **************************************************************
	 * NOTE TO PORTERS:  We use a segment's worth of contiguous
	 * memory for the 3B2, but that is obviously machine-dependent.
	 **************************************************************
	 */

	register size_t		bufsize = MIN(stp->sr_ret_val, NBPS);
	register caddr_t	buf;
	register caddr_t	nextcp;		/* in buf */
	register caddr_t	end;		/* beyond end of buf */
	size_t			cpinsize;	/* bytes to rcopyin call */
	size_t			nleft;		/* bytes still on client */
	size_t			writerq = 0;	/* bytes sent to VOP_WRITE */
	size_t			prewrite = 0;
	mblk_t			*iobp;
	int			totwrite = 0;
	int			error = 0;

	if ((buf = kseg((int)btoc(bufsize))) == NULL) {
		return ENOMEM;
	}

	end = buf + bufsize;
	nleft = stp->sr_ret_val;

	if (stp->sr_in_bp) {

		ASSERT(RF_REQ(stp->sr_in_bp)->rq_xfer.prewrite);

		prewrite = RF_REQ(stp->sr_in_bp)->rq_xfer.prewrite;

		/*
		 * Drop the RFS headers, so that all blocks contain only
		 * data, simplifying the processing loop.
		 */

		iobp = stp->sr_in_bp =
		  rf_dropbytes(stp->sr_in_bp, RF_MIN_REQ(stp->sr_vcver));

		if (prewrite > stp->sr_ret_val || !iobp) {
			error = rfsr_discon("rfsr_rawwrite bad request header",
			  stp);
			goto out;
		}
	}

	nleft -= prewrite;
	while (prewrite) {
		size_t	cpsz;

		/* Move data from message into buf. */

		nextcp = buf;
		while (iobp &&
		  (cpsz = MIN(end - nextcp, iobp->b_wptr - iobp->b_rptr))
		   != 0) {
			bcopy((caddr_t)iobp->b_rptr, nextcp, cpsz);
			nextcp += cpsz;
			totwrite += cpsz;
			if ((iobp->b_rptr += cpsz) == iobp->b_wptr) {
				stp->sr_in_bp = stp->sr_in_bp->b_cont;
				iobp->b_cont = NULL;
				rf_freeb(iobp);
				iobp = stp->sr_in_bp;
			}
		}

		writerq = nextcp - buf;
		base += writerq;
		prewrite -= writerq;

		if (!prewrite != !iobp) {
			error = rfsr_discon("rfsr_rawrite bad prewrite", stp);
			goto out;
		}

		if (nextcp == end || !nleft) {

			/*
			 * Write now only if buffer is full or nothing left
			 * on client.  Otherwise delay for rest of data.
			 */

			uiop->uio_iov->iov_base = buf;
			uiop->uio_resid = uiop->uio_iov->iov_len = writerq;

			if ((error =
			   VOP_WRITE(vp, uiop, ioflag, stp->sr_cred)) != 0 ||
			  uiop->uio_resid == writerq) {

				/* Error or can't write any more */

				goto out;
			}

			if (uiop->uio_resid) {

				/*
				 * Now slide unwritten characters down to the
				 * beginning of buf.  This seems easier than
				 * dealing with a circular buffer in rcopyin.
				 */

				ovbcopy(nextcp - uiop->uio_resid, buf,
				  (size_t)uiop->uio_resid);
			}

			stp->sr_ret_val -= writerq - uiop->uio_resid;
		}
#ifdef	DEBUG
		else {
			/* Fall throught to rcopyin and write */
			ASSERT(!prewrite);
		}
#endif
	}

	while(stp->sr_ret_val && !error) {

		ASSERT(nleft);

		cpinsize = MIN(nleft, end - nextcp);
		if (rcopyin(base, nextcp, cpinsize, 1)) {
			error = EFAULT;
			goto out;
		}

		nleft -= cpinsize;
		base += cpinsize;
		totwrite += cpinsize;
		writerq = nextcp + cpinsize - buf;

		uiop->uio_iov->iov_base = nextcp = buf;
		uiop->uio_resid = uiop->uio_iov->iov_len = writerq = cpinsize;

		if ((error =
		   VOP_WRITE(vp, uiop, ioflag, stp->sr_cred)) != 0 ||
		  uiop->uio_resid == writerq) {
			goto out;
		}

		if (uiop->uio_resid) {
			ovbcopy(nextcp - uiop->uio_resid, buf,
			  (size_t)uiop->uio_resid);
		}

		stp->sr_ret_val -= writerq - uiop->uio_resid;
	}
out:
	SR_FREEMSG(stp);
	unkseg(buf);
	stp->sr_srmp->srm_kbcnt += totwrite / 1024;
	return error;
}

/*
 * Process all write operations but big ones to raw devices.
 * Stp denotes the server state structure with sr_ret_val set to the
 * residual write count, and sr_cred holding the client credentials.
 * Vp denotes the file to be write, uiop a uio_t with associated
 * iovec.  Ioflag, ftype and base are also defined by the caller.
 * The last is the start of the io buffer on the client; we send
 * it for compatability, but clients should really ignore it.
 * Req is a pointer to a rf_request_t containing prewritten data,
 * or NULL.
 * Assumes stp->sr_ret_val is set to the residual write count; updates
 * it accordingly.
 *
 * Return 0 for success, nonzero errno for failure.
 */
int
rfsr_cookedwrite(stp, vp, uiop, ioflag, base)
	register rfsr_state_t	*stp;
	register vnode_t	*vp;
	register uio_t		*uiop;
	unsigned		ioflag;
	caddr_t			base;
{
	rcvd_t			*rd = NULL;	/* catch copyin messages */
	register size_t		nleft;		/* bytes on client */
	register size_t		writerq;	/* bytes in message */
	register sndd_t		*srchan;	/* channel to client */
	int			totwrite = 0;
	register int		error = 0;
	int			tmperror = 0;	/* remember errors that don't
						 * immediately break the loop
						 */

	VOP_RWLOCK(vp);

	nleft = stp->sr_ret_val;
	srchan = u.u_srchan;

	if (stp->sr_in_bp) {

		ASSERT(RF_REQ(stp->sr_in_bp)->rq_xfer.prewrite);

		writerq = RF_REQ(stp->sr_in_bp)->rq_xfer.prewrite;

		if (writerq > stp->sr_ret_val) {
			error = rfsr_discon("rfsr_cookedwrite bad prewrite",
			  stp);
			goto out;
		}
	
		if ((error = rfsr_writev(vp, uiop, ioflag, stp,
		  RF_MIN_REQ(stp->sr_vcver))) != 0) {
			goto out;
		}
		stp->sr_ret_val -= writerq - uiop->uio_resid;
	}

	nleft -= writerq;
	base += writerq;
	totwrite += writerq;
	if (!stp->sr_ret_val || uiop->uio_resid || error) {
		goto out;
	}
	if ((error = sendcopyinmsg(&rd, base, srchan, nleft)) != 0) {
		goto out;
	}
	while (nleft) {

		/*
		 * Break for no more data, or error other than write error.
		 * Even if we get an error on write, need to keep spinning
		 * to pick up messages.
		 */

		if ((error = getcopyinmsg(rd, &stp->sr_in_bp)) != 0) {
			goto out;
		}

		writerq = RF_RESP(stp->sr_in_bp)->rp_count;

		if (writerq > stp->sr_ret_val) {
			error = rfsr_discon("rfsr_cookedwrite bad copyin", stp);
			goto out;
		}

		if (!tmperror && !uiop->uio_resid) {
			tmperror = rfsr_writev(vp, uiop, ioflag, stp,
			  RF_MIN_RESP(stp->sr_vcver));
			stp->sr_ret_val -= writerq - uiop->uio_resid;
			totwrite += writerq;
		}
		nleft -= writerq;
		rf_freemsg(stp->sr_in_bp);
		stp->sr_in_bp = NULL;
	}
out:
	rcvd_free(&rd);
	SR_FREEMSG(stp);
	VOP_RWUNLOCK(vp);
	stp->sr_srmp->srm_kbcnt += totwrite / 1024;
	return error ? error : tmperror;
}

/*
 * Code common to initialization sequences of rfsr_read, rfsr_write.
 * Always returns an error status.  If no errors and a change in mandatory
 * lock status has occurred, allocates a response, sets its RP_MNDLCK flag
 * and set *sr_ctrlp to SR_NACK_RESP.
 */
int
rfsr_rdwrinit(stp, uiop, vp, flagp, sr_ctrlp)
	register rfsr_state_t	*stp;
	register uio_t		*uiop;
	register vnode_t	*vp;
	unsigned		*flagp;
	rfsr_ctrl_t		*sr_ctrlp;
{
	register rf_request_t	*req = RF_REQ(stp->sr_in_bp);
	vattr_t			vattr;
	register unsigned	ioflag = 0;
	register int		error = 0;
	int			chklock;

	chklock = RF_MSG(stp->sr_in_bp)->m_stat & RF_VER1 &&
	  !(req->rq_flags & RQ_MNDLCK);
	/*
	 * return value of read, write requests in residual char count
	 */
	stp->sr_ret_val = req->rq_xfer.count;
	stp->sr_oldoffset = req->rq_xfer.offset;
	uiop->uio_iovcnt = 1;
	uiop->uio_offset = req->rq_xfer.offset;
	uiop->uio_segflg = UIO_SYSSPACE;
	uiop->uio_fmode = req->rq_xfer.fmode;
	uiop->uio_limit = req->rq_ulimit;
	if (uiop->uio_fmode & FAPPEND) {
		ioflag |= IO_APPEND;
	}
	if (uiop->uio_fmode & FSYNC) {
		ioflag |= IO_SYNC;
	}
	*flagp = ioflag;

	if (chklock || stp->sr_opcode == RFPUTPAGE) {
		vattr.va_mask = AT_MODE | AT_SIZE;
	  	if ((error = VOP_GETATTR(vp, &vattr, 0, stp->sr_cred)) == 0) {
			if (stp->sr_opcode == RFPUTPAGE &&
			  stp->sr_ret_val + uiop->uio_offset >= vattr.va_size) {
				error = EFAULT;
			} else if (chklock && MANDLOCK(vp, vattr.va_mode)) {
				ASSERT(!stp->sr_out_bp);
				stp->sr_out_bp =
				  rfsr_rpalloc((size_t)0, stp->sr_vcver);
				RF_RESP(stp->sr_out_bp)->rp_cache = RP_MNDLCK;
				*sr_ctrlp = SR_NACK_RESP;
			}
		}
	}
	return error;
}

/*
 * Handle write data that may straddle data buffers.  Destroys
 * stp->sr_in_bp.
 *
 * TO DO:  This thing KNOWS that it can overwrite iovec and iovcnt with
 * impunity.
 */
STATIC int
rfsr_writev(vp, uiop, ioflag, stp, hdrsz)
	vnode_t		*vp;
	uio_t		*uiop;
	unsigned	ioflag;
	rfsr_state_t	*stp;
	size_t		hdrsz;
{
	iovec_t		*iovp;
	int		niov = 0;
	int		error = 0;
	int		resid;

	stp->sr_in_bp = rf_dropbytes(stp->sr_in_bp, hdrsz);

	if (!stp->sr_in_bp) {
		return rfsr_discon("rfsr_writev no client data", stp);
	}

	rf_iov_alloc(uiop, stp->sr_in_bp);
	iovp = uiop->uio_iov;
	niov = uiop->uio_iovcnt;

	do {
		resid = uiop->uio_resid;
		error = VOP_WRITE(vp, uiop, ioflag, stp->sr_cred);
	} while (uiop->uio_resid && uiop->uio_resid != resid && !error);

	SR_FREEMSG(stp);
	RF_IOV_FREE(iovp, niov);
	return error;
}

/*
 * Return as side-effects message block and (possibly) updated
 * sd; sdp must be non-NULL.
 *
 * For supernumerary servers, does not return.
 *
 * We use splstr because rf_deliver manipulates message and server lists
 * at that priority.
 *
 * NOTE:  callers are assumed to be on neither active or idle list.
 */
void
rfsr_rcvmsg(bufpp, sdp, stp)
	mblk_t		**bufpp;	/* updated w/ received message block */
	sndd_t		*sdp;		/* sd updated if rd in message */
	rfsr_state_t	*stp;		/* state struct for current server */
{
	register rcvd_t		*rdp;
	register int		s;
	register mblk_t		*bufp;
	register rf_message_t	*msg;

	extern rcvd_t *sigrd;

	s = splstr();

	for (;;) {
		if (!sigrd) {
			splx(s);
			rfsr_exit(stp);		/* no return */
			/* NOTREACHED */
		} else if (!RCVDEMP(sigrd)) {
			rdp = sigrd;
			break;
		} else if (rfsr_msgs)  {
			rdp = rfsr_msgs;
			break;
		} else if (rfsr_nidle == minserve) {

			splx(s);
			rfsr_exit(stp);		/* no return */
			/* NOTREACHED */
		} else {
			/* Arrmsg will take the server off the idle list
			 */
			++rfsr_nidle;
			u.u_procp->p_rlink = rfsr_idle_procp;
			rfsr_idle_procp = u.u_procp;
			if (sleep((caddr_t)&u.u_procp->p_srwchan,
			  PREMOTE | PCATCH | PNOSTOP)) {
				rfsr_rmlist(&rfsr_idle_procp, &rfsr_nidle,
				  u.u_procp);
				splx(s);
				rfsr_exit(stp);
				/* NOTREACHED */
			} else {
				continue;	/* to pick up the rd */
			}
		}
	}
	rfc_info.rfci_rcv_msg++;
	bufp = rf_dequeue(rdp);
	ASSERT(bufp);
	*bufpp = bufp;
	msg = RF_MSG(bufp);
	if (msg->m_stat & RF_GIFT) {
		/*
		 * Signal messages don't contain gifts of specific rds.
		 */
		sndd_set(sdp, (queue_t *)msg->m_queue, msg->m_giftid);
	}
	sdp->sd_mntid = RF_COM(bufp)->co_mntid;
	++rfsr_nactive;
	u.u_procp->p_rlink = rfsr_active_procp;
	rfsr_active_procp = u.u_procp;

	splx(s);
}


/*
 * Remove from list the denoted proc, updating list and counter.
 *
 * Servers neither on the active or idle lists are those looking
 * for work, but not sleeping.  We keep such servers off the active
 * list so they won't have signals posted against them erroneously,
 * and take them off the idle list at interrupt level when a message
 * comes in, to avoid overestimating the number of idle servers due
 * to rapidly arriving messages.  If the server were just awakened,
 * and left to take itself off the idle list, several arrivals could
 * see the server as idle.
 */
STATIC void
rfsr_rmlist(listp, countp, procp)
	proc_t		**listp;
	int		*countp;
	register proc_t	*procp;
{
	register proc_t	*cand_procp = *listp;
	register proc_t	*pred_procp = NULL;

	ASSERT(*countp == rfsr_listcount(*listp));
	while (cand_procp && cand_procp != procp) {
		pred_procp = cand_procp;
		cand_procp = cand_procp->p_rlink;
	}
	if (cand_procp == procp) {
		if (procp == *listp) {
			*listp = procp->p_rlink;
		} else {
			pred_procp->p_rlink = procp->p_rlink;
		}
		(*countp)--;
		procp->p_rlink = NULL;
		ASSERT(*countp == rfsr_listcount(*listp));
	}
}

/*
 * Remove denoted process from the active servers list, clearing
 * pending SIGUSR1s (used to abort operations to free server
 * resources).
 */
void
rfsr_rmactive(procp)
	register struct proc *procp;
{
	rf_delsig(procp, SIGUSR1);
	rfsr_rmlist(&rfsr_active_procp, &rfsr_nactive, procp);
}

/* add rcvd to the END of the rfsr_msgs list.
 * duplicates are ignored because interrupts on
 * different receive descriptors can try to put
 * the same receive descriptor on the rfsr_msgs list.
 *
 * We use splstr because rf_deliver manipulates message and server lists
 * at that priority.
 *
 * TO DO: in busy systems, this could be a hog.  Make the list a ring.
 */
void
rfsr_addmsg(rcvdp)
	register rcvd_t *rcvdp;
{
	register rcvd_t *current;
	register int s = splstr();

	if (!rfsr_msgs)  {
		rfsr_msgs = rcvdp;
		rcvdp->rd_next = NULL;
		rfsr_nmsg = 1;
	} else {
		current = rfsr_msgs;
		while (current != rcvdp && current->rd_next) {
			current = current->rd_next;
		}
		if (current != rcvdp) {
			current->rd_next = rcvdp;
			rcvdp->rd_next = NULL;
			rfsr_nmsg++;
		}
	}
	splx(s);
}


/* If the rd is empty, remove it from the rfsr_msgs list.
 * Update rfsr_nmsg, NULL rcvdp->rd_next.
 *
 * We use splstr because rf_deliver manipulates message and server lists
 * at that priority.
 */
void
rfsr_rmmsg(rcvdp)
	register rcvd_t *rcvdp;
{
	register rcvd_t *prev;
	register int s = splstr();

	if (RCVDEMP(rcvdp) && rfsr_msgs) {
		prev = rfsr_msgs;
		if (rfsr_msgs == rcvdp) {
			rfsr_msgs = rfsr_msgs->rd_next;
			--rfsr_nmsg;
		} else {
			register rcvd_t *current;

			while (((current = prev->rd_next) != NULL)
			  && (current != rcvdp)) {
				prev = current;
			}
			if (current) {
				prev->rd_next = current->rd_next;
				current->rd_next = NULL;
				--rfsr_nmsg;
			}
		}
	}
	splx(s);
}

#ifdef DEBUG
int
rfsr_listcount(p)
	register proc_t *p;
{
	register int result = 0;

	while (p) {
		p = p->p_rlink;
		result++;
	}
	return result;
}
#endif

/*
 * Map the va_fsid into an index into the server device table.
 * (TO DO:  send real devs between 4.X machines; this requires
 * network wide hostids.)
 * Assumes times and ids are set; adjusts them for client.
 */
int
rfsr_vattr_map(stp, vap)
	register rfsr_state_t	*stp;
	register vattr_t	*vap;
{
	register dev_t		dev = vap->va_fsid;
	register int		indx;

	if ((indx = rfsr_dev_find(dev)) == -1 &&
	  (indx = rfsr_dev_insert(dev)) == -1) {
		rfsr_dev_clean();
		if ((indx = rfsr_dev_insert(dev)) == -1) {
			return ENOMEM;
		}
	}
	vap->va_fsid = (dev_t)indx;
	rfsr_adj_time(vap, stp->sr_gdpp);
	vattr_rmap(stp->sr_gdpp, vap);
	return 0;
}

/*
 * Table and operations to allow RFS server to fake-up "devs"
 * as it did using static mount table in old system
 */

typedef struct rfsr_dev_entry {
	dev_t	dev;
	int	inuse : 1;
} rfsr_dev_entry_t;

#define RFSRDEV_SIZE	256	/* rfsr_dev_tbl can't be larger than 256
				 * because index fits in a byte
				 */

STATIC int		rfsr_dev_max;	/* max index currently in use */
STATIC rfsr_dev_entry_t	rfsr_dev_tbl[RFSRDEV_SIZE];

/*
 * rfsr_dev_init() initializes the rfsr_dev_tbl such that all devices are
 * 0 and not in use.  rfsr_dev_max is initialized to -1, indicating that
 * there are no entries currently in use in the rfsr_dev_tbl.
 */
void
rfsr_dev_init()
{
	int i;

	for (i = 0; i < RFSRDEV_SIZE; i++) {
		rfsr_dev_tbl[i].dev = 0;
		rfsr_dev_tbl[i].inuse = 0;
	}
	rfsr_dev_max = -1;
}

/*
 * rfsr_dev_getvfs() returns the vfs corresponding to the pseudo-dev
 * index if index is in use and the file system referred to by the
 * dev is currently mounted.  Else returns NULL.
 */
vfs_t *
rfsr_dev_dtov(indx)
	register int indx;
{
	struct vfs *vfsp;

	if (indx < 0 || indx >= RFSRDEV_SIZE) {
		return NULL;
	}
	if (!rfsr_dev_tbl[indx].inuse) {
		return NULL;
	}
	if (!(vfsp = rfsr_dev_vfs_search(rfsr_dev_tbl[indx].dev))) {
		rfsr_dev_remove(indx);
	}
	return vfsp;
}

/*
 * rfsr_dev_find() returns the index corresponding to the given dev, or
 * -1 if the dev is not entered in the table.
 */
STATIC int
rfsr_dev_find(dev)
	dev_t dev;
{
	int i;

	if (rfsr_dev_max == -1) {
		return -1;
	}
	for (i = 0; i <= rfsr_dev_max; i++) {
		if (rfsr_dev_tbl[i].inuse && rfsr_dev_tbl[i].dev == dev) {
			return i;
		}
	}
	return -1;
}

/*
 * rfsr_dev_vfs_search() scans the list of mounted vfs's to see if the file
 * system corresponding to the specified dev is still mounted.
 * It returns a pointer to the vfs for success (mounted), NULL for
 * failure (not mounted).
 */
STATIC struct vfs *
rfsr_dev_vfs_search(dev)
	dev_t dev;
{
	struct vfs *tmpvfs = rootvfs;

	while (tmpvfs) {
		if (tmpvfs->vfs_dev == dev) {
			return tmpvfs;
		}
		tmpvfs= tmpvfs->vfs_next;
	}
	return NULL;
}

/*
 * rfsr_dev_clean() checks the entire rfsr_dev_tbl for invalid (unmounted)
 * entries, removing any it finds.
 */
STATIC void
rfsr_dev_clean()
{
	int i;

	for (i = 0; i < RFSRDEV_SIZE; i++) {
		if (!rfsr_dev_vfs_search(rfsr_dev_tbl[i].dev)) {
			rfsr_dev_remove(i);
		}
	}
}

/*
 * rfsr_dev_insert() inserts the specified dev in the rfsr_dev_tbl, returning
 * the corresponding index into the table on success or -1 if the table
 * is full.
 */
STATIC int
rfsr_dev_insert(dev)
	dev_t dev;
{
	int i;

	for (i = 0; i < RFSRDEV_SIZE; i++) {
		if (!rfsr_dev_tbl[i].inuse) {
			rfsr_dev_tbl[i].dev = dev;
			rfsr_dev_tbl[i].inuse = 1;
			if (i > rfsr_dev_max) {
				rfsr_dev_max = i;
			}
			return i;
		}
	}
	return -1;
}

/*
 * rfsr_dev_remove() invalidates the entry corresponding to the specifiec
 * index by nulling the entry's dev and setting inuse.
 * If the index of the entry being removed was the largest index currently
 * in use, reset rfsr_dev_max to be the new largest index.
 */
STATIC void
rfsr_dev_remove(indx)
	int indx;
{
	int i;

	rfsr_dev_tbl[indx].dev = 0;
	rfsr_dev_tbl[indx].inuse = 1;
	if (indx == rfsr_dev_max) {
		rfsr_dev_max = -1;
		for (i = RFSRDEV_SIZE - 1; i >= 0; i--) {
			if (rfsr_dev_tbl[i].inuse) {
				rfsr_dev_max = i;
				break;
			}
		}
	}
}

/*
 * Routines to move data between client and server
 */

/*
 * Move nbytes bytes of data starting at "from" on the client to a location
 * starting at "to" on the server.  Explicit == 0 iff call is via "if
 * RF_SERVER() hook," allowing us to catch drivers that ignore segflg.
 *
 * Returns 0 for succcess, -1 for failure.
 */
int
rcopyin(from, to, nbytes, explicit)
	register caddr_t	from;
	register caddr_t	to;
	register size_t		nbytes;
	int			explicit;
{
	/*
	 * The u_gift stuff in here is gross, but we have no other
	 * way to get the send descriptor on implicit calls from drivers.
	 */

	mblk_t			*bp;
	rcvd_t			*rd;
	gdp_t			*gp = QPTOGP(u.u_srchan->sd_queue);
	int			vcver = gp->version;
	int			result = 0;
	uio_t			uio;

	if (!explicit && u.u_syscall != RFIOCTL) {
		/*
		 * Only drivers that ignore segflg will trip
		 * over the RF_SERVER() hook in copyin on ops
		 * other than ioctl.
		 */
		cmn_err(CE_WARN,
		  "rcopyin: driver ignores seg flag, no RFS support\n");
		return -1;
	}

	/* Send copyin response on u.u_srchan and wait for data */

	if (sendcopyinmsg(&rd, from, u.u_srchan, nbytes)) {
		return -1;
	}

	/*
	 * Some meaningless fields here, offset, fmode, limit, e.g.
	 */

	uio.uio_iov = NULL;
	uio.uio_iovcnt = 0;
	uio.uio_offset = 0;
	uio.uio_segflg = UIO_SYSSPACE;
	uio.uio_fmode = 0;
	uio.uio_limit = 0;

	for (;;) {
		iovec_t			*iovp;
		int			niov;
		int			count;


		if (getcopyinmsg(rd, &bp)) {
			result = -1;
			break;
		}
		count = RF_RESP(bp)->rp_count;

		if (count > nbytes ||
		  (bp = rf_dropbytes(bp, RF_MIN_RESP(vcver))) == NULL) {
			cmn_err(CE_WARN,"rcopyin bad response");
			gdp_discon("rcopyin", gp);
			rf_freemsg(bp);
			bp = NULL;
			result = -1;
			break;
		}

		rf_iov_alloc(&uio, bp);
		iovp = uio.uio_iov;
		niov = uio.uio_iovcnt;

		if (count != uio.uio_resid) {
			cmn_err(CE_WARN,"rcopyin count != uio.uio_resid");
			gdp_discon("rcopyin", gp);
			rf_freemsg(bp);
			bp = NULL;
			result = -1;
			break;
		}

		(void)uiomove(to, count, UIO_WRITE, &uio);	/* bcopy */
		ASSERT(!uio.uio_resid);
		nbytes -= count;
		to += count;

		RF_IOV_FREE(iovp, niov);
		rf_freemsg(bp);
		bp = NULL;
		if (!nbytes) {
			break;
		}
	}
	rcvd_free(&rd);
	return result;
}

/*
 * Move nbytes bytes of data starting at "from" on the server to a location
 * starting at "to" on the client.
 *
 * Rpp is non-NULL iff calling context expects response containing
 * last fragment of data to be returned.  Otherwise, last message is
 * shipped from here.  In the case that rpp is non-NULL, *rpp
 * MUST be NULL, or we will create garbage.  The intent is that
 * only "implicit calls" from if server hooks will pass in a NULL
 * rpp.
 *
 * Returns 0 for succcess, -1 for failure.
 */
int
rcopyout(from, to, nbytes, bpp)
	register caddr_t	from;
	register caddr_t	to;
	register size_t		nbytes;
	register mblk_t		**bpp;
{
	/*
	 * The u_srchan stuff in here is gross, but we have no other
	 * way to get the send descriptor on implicit calls.
	 */
	int			result = 0;
	sndd_t			*sdp = u.u_srchan;	/* gag */
	gdp_t			*gp = QPTOGP(sdp->sd_queue);
	int			vcver = gp->version;
	size_t			datasz = gp->datasz;
	size_t			hdrsz = RF_MIN_RESP(vcver);
	mblk_t			*bp;

	if (!bpp && u.u_syscall != RFIOCTL) {

		/*
		 * Only drivers that ignore segflg will trip
		 * over the RF_SERVER() hook in copyout on ops
		 * other than ioctl.  If the driver is well-behaved,
		 * it will be following the server, doing kernel-kernel
		 * data movement.
		 */

		cmn_err(CE_WARN,
		  "rcopyout: driver ignores seg flag, no RFS support\n");
		return -1;
	}
	for (;;) {
		register rf_response_t *rp;
		register size_t copycount = MIN(nbytes, datasz);

		bp = rfsr_rpalloc(copycount, vcver);
		rp = RF_RESP(bp);
		bcopy(from, rf_msgdata(bp, hdrsz), copycount);
		rp->rp_copyout.buf = (long)to;
		rp->rp_errno = 0;
		rp->rp_copyout.copysync = 0;
		if (nbytes > datasz || !bpp) {
			/*
			 * Not last copyout message or caller wants
			 * us to send last.
			 */
			RF_COM(bp)->co_opcode = RFCOPYOUT;
			if (rf_sndmsg(sdp, bp, hdrsz + (size_t)copycount,
			  (rcvd_t *)NULL, FALSE)) {
				result = -1;
				bp = NULL;
				break;
			}
			bp = NULL;
		}
		nbytes -= copycount;
		from += copycount;
		to += copycount;
		if (!nbytes) {
			break;
		}
	}
	if (bpp) {
		*bpp = bp;
	}
	return result;
}

/*
 * rfubyte(): This routine is called by fubyte to copy one byte from the
 * remote client machine.
 */
int
rfubyte(from)
	caddr_t from;
{
	char to[sizeof(char)+1];

	if (rcopyin(from, (caddr_t)to, sizeof(char), 0)) {
		return -1;
	}
	return *to;
}

/*
 * rfuword(): This routine is called to copy a word from the remote
 * client machine.
 */
int
rfuword(from)
	int *from;
{
	int to;

	if (rcopyin((caddr_t)from, (caddr_t)&to, sizeof(int), 0)) {
		return -1;
	}
	return to;
}

/*
 * rsubyte(): This routine passes c back to the remote user.
 */
int
rsubyte(to, c)
	unsigned char *to;
	unsigned char	c;
{
	return rcopyout((caddr_t)&c, (caddr_t)to,
	  sizeof(char), (mblk_t **)NULL);
}

/*
 * rsuword(): This routine passes back w to the remote user's space.
 */
int
rsuword(to, w)
	size_t *to;
	size_t w;
{
	return rcopyout((caddr_t)&w, (caddr_t)to, sizeof(int), (mblk_t **)NULL);
}

/* Sends a copyin message to client, requesting data to complete
 * current io.
 * For success, updates *rdpp with a receive descriptor ready to
 * receive copyin messages from the client.  Otherwise, *rdpp
 * is undefined.  Base is denotes the client io buffer, and is
 * used only for protocol consistency; clients have this information
 * and should ignore server copy.  Gift is assumed to point to the
 * channel to the client; nbytes is the number of bytes of data to request.
 *
 * Returns 0 for success, nonzero errno for failure.  NULLs *rdpp in
 * failure cases.
 */
STATIC int
sendcopyinmsg(rdpp, base, gift, nbytes)
	rcvd_t			**rdpp;
	register caddr_t	base;
	register sndd_t		*gift;
	size_t			nbytes;
{
	register rcvd_t		*rdp;
	register int		error;
	register mblk_t		*outbp;
	register rf_response_t	*resp;
	register int		vcver = QPTOGP(gift->sd_queue)->version;

	/* NOTE: because of protocol history, there is NO SUCH
	 * THING AS A RFCOPYIN REQUEST.  Messages in both directions
	 * are RESPONSES.
	 */
	if ((error = rcvd_create(TRUE, RDSPECIFIC, rdpp)) != 0) {
		return error;
	}
	rdp = *rdpp;
	rdp->rd_sdp = gift;
	outbp = rfsr_rpalloc((size_t)0, vcver);
	resp = RF_RESP(outbp);
	RF_COM(outbp)->co_opcode = RFCOPYIN;
	resp->rp_count = nbytes;
	resp->rp_xfer.buf = (long)base;
	if ((error = rf_sndmsg(gift, outbp, RF_MIN_RESP(vcver), rdp, FALSE))
	  != 0) {
		rcvd_free(rdpp);
	}
	return error;
}

/* Assumes rdp is set up to receive copying messages from client.
 * Grabs message, updates *bpp with its address for success; *bpp
 * is undefined otherwise.  Frees incoming mblk_t in error cases.
 * Returns 0 for success, nonzero errno for failure.
 */
STATIC int
getcopyinmsg(rdp, bpp)
	register rcvd_t *rdp;
	register mblk_t **bpp;
{
	register int error;

	/* NOTE: because of protocol history, there is NO SUCH
	 * THING AS A RFCOPYIN REQUEST.  Messages in both directions
	 * are RESPONSES.
	 */
	if ((error = rf_rcvmsg(rdp, bpp)) != 0) {
		/* link down */
		return error;
	}
	if ((error = RF_RESP(*bpp)->rp_errno) != 0) {
		/* error on client side; assume no more messages coming */
		rf_freemsg(*bpp);
		*bpp = NULL;
	}
	return error;
}

/*
 * Update *flp with flock embedded in stp->sr_in_bp, de-canonizing if
 * necessary.  In error cases, frees  stp->sr_in_bp, updates *ctrlp.
 */
int
rfsr_copyflock(flp, stp)
	register caddr_t	flp;
	register rfsr_state_t	*stp;
{
	int			error;
	size_t			datasz;
	size_t			hdrsz;
	register caddr_t	rqdata;

	datasz =
	  stp->sr_vcver >= RFS2DOT0 ? sizeof(flock_t) : sizeof(o_flock_t);
	hdrsz = RF_MIN_REQ(stp->sr_vcver);
	if (stp->sr_gdpp->hetero != NO_CONV) {
		datasz += (stp->sr_vcver >= RFS2DOT0 ? FLOCK_XP :
		  OFLOCK_XP) - MINXPAND;
	}
	if ((error = RF_PULLUP(stp->sr_in_bp, hdrsz, datasz)) != 0) {
		SR_FREEMSG(stp);
		return error;
	}
	rqdata = rf_msgdata(stp->sr_in_bp, hdrsz);
	if (stp->sr_gdpp->hetero != NO_CONV &&
	  !rf_fcanon((stp->sr_vcver >= RFS2DOT0 ? FLOCK_FMT : O_FLOCK_FMT),
	   rqdata, rqdata + datasz, rqdata)) {
		return rfsr_discon("rfsr_copyflock bad data", stp);
	}
	if (stp->sr_vcver >= RFS2DOT0) {
		((struct flock *)flp)[0] = ((struct flock *)rqdata)[0];
	} else {
		((struct o_flock *)flp)[0] = ((struct o_flock *)rqdata)[0];
	}
	return 0;
}

/*
 * Return a message block large enough to handle the requested data size.
 * Assign its co_type and rp_bp and assign rp_nodata and rp_count
 * based on the size given.
 */
mblk_t *
rfsr_rpalloc(size, vcver)
	register size_t		size;
	int			vcver;
{
	mblk_t			*nbp;
	register rf_response_t	*rsp;

	(void)rf_allocmsg(RF_MIN_RESP(vcver), size, BPRI_MED, FALSE,
	  NULLCADDR, NULLFRP, &nbp);
	ASSERT(nbp);
	rsp = RF_RESP(nbp);
	RF_COM(nbp)->co_type = RF_RESP_MSG;
	rsp->rp_count = size;
	rsp->rp_nodata = !size;
	return nbp;
}

/*
 * Called after a succesful VOP_READDIR when read was rounded up to
 * RF_MAXDIRENT to make sure to get last directory entry or entries.
 *
 * Updates *uiop offset and resid to reflect the number of entries and
 * number of bytes to be sent and returns the number of bytes.
 * Also updates *eofp.
 */
int
rfsr_lastdirents(dp, uiop, ooff, resid, eofp)
	register caddr_t	dp;	/* start of dirents */
	register off_t		ooff;	/* uio_offset before VOP_READDIR */
	register uio_t		*uiop;
	register size_t		resid;	/* residual count for client request */
	int			*eofp;	/* in/out, from VOP_READDIR */
{
	register caddr_t	end = dp + RF_MAXDIRENT - uiop->uio_resid;
	register size_t		reclen;
	register size_t		nbytes;

	if (dp == end) {
		return 0;
	}
	reclen = ((dirent_t *)dp)->d_reclen;
	nbytes = 0;
	while (nbytes + reclen <= resid && dp + reclen < end) {
		ooff = ((dirent_t *)dp)->d_off;
		nbytes += reclen;
		dp += reclen;
		reclen = ((dirent_t *)dp)->d_reclen;
	}
	if (nbytes + reclen <= resid) {
		ASSERT(dp + reclen == end);
		nbytes += reclen;
		uiop->uio_offset = ((dirent_t *)dp)->d_off;
	} else {
		/*
		 * We can't fit into message everything we read.
		 */
		uiop->uio_offset = ooff;
		*eofp = 0;
	}
	uiop->uio_resid = RF_MAXDIRENT - nbytes;
	return nbytes;
}

/*
 * This routine is called by ops dealing with vnodes when they are to
 * this point error-free, and after they have allocated a response message,
 * referred to by stp->sr_out_bp.
 *
 * This routine should be used only for connections of version 2 or greater,
 * where the response does not contain a gift.
 *
 * When the response contains a gift, rp_v2vcode refers to
 * the newly referenced file.  Code in rfsr_gift_setup(), rather than this
 * routine, handles that case.
 *
 * rfsr_cacheck() sets rp_v2vcode in the response to indicate whether
 * cacheing may be enabled(reenabled) for the file referred to by stp->sr_vp.
 * Whenever it sets this to a non-zero value, it ensures that the
 * rduptr->ru_cflag is set for this file/client pair.
 */
int
rfsr_cacheck(stp, mntid)
	register rfsr_state_t	*stp;
	register long		mntid;
{
	register rd_user_t	*rdup;
	register rcvd_t		*rcvdp = stp->sr_rdp;
	register vnode_t	*vp = RDTOV(rcvdp);
	register int		error;

	/*
	 * vp or rdup can be NULL if we've just given up our last
	 * reference to a vnode.  stp->sr_gift != NULL implies
	 * we've already done checking equivalent to this.
	 */

	ASSERT(!stp->sr_gift);

	if (vp && (rdup = rdu_find(rcvdp, u.u_procp->p_sysid,
	   mntid, (rd_user_t **)NULL)) != NULL) {
		register rf_response_t	*rp = RF_RESP(stp->sr_out_bp);
		vattr_t			vattr;

		vattr.va_mask = AT_MODE | AT_SIZE | AT_VCODE;
		if ((error = VOP_GETATTR(vp, &vattr, 0, stp->sr_cred)) != 0) {
			return error;
		}
		RF_COM(stp->sr_out_bp)->co_size = vattr.va_size;
		rp->rp_fhandle = (long)vp;
		if (!MANDLOCK(vp, vattr.va_mode) && vp->v_type == VREG &&
                  stp->sr_srmp->srm_flags & SRM_CACHE &&
		  (lbolt - rcvdp->rd_mtime > rfc_time ||
		   lbolt < rcvdp->rd_mtime)) {
			rdup->ru_cflag |= RU_CACHE_ON;
			if (rdup->ru_cflag & RU_CACHE_DISABLE) {
				rdup->ru_cflag &= ~RU_CACHE_DISABLE;
				if (stp->sr_vcver == RFS1DOT0) {
					rp->rp_cache = DU_CACHE_ENABLE;
				}
			}
		}
		if (rdup->ru_cflag & RU_CACHE_ON && stp->sr_vcver >= RFS2DOT0) {
			rp->rp_v2vcode = vattr.va_vcode;
		}
	}
	return 0;
}

int
rfsr_discon(msg, stp)
	char		*msg;
	rfsr_state_t	*stp;
{
	gdp_discon(msg, stp->sr_gdpp);
	SR_FREEMSG(stp);
	return EPROTO;
}
