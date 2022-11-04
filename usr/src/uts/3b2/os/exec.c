/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/exec.c	1.50"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/sysmacros.h"
#include "sys/sbd.h"
#include "sys/systm.h"
#include "sys/map.h"
#include "sys/signal.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/buf.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/file.h"
#include "sys/fstyp.h"
#include "sys/acct.h"
#include "sys/sysinfo.h"
#include "sys/reg.h"
#include "sys/var.h"
#include "sys/immu.h"
#include "sys/proc.h"
#include "sys/prsystm.h"
#include "sys/tuneable.h"
#include "sys/tty.h"
#include "sys/cmn_err.h"
#include "sys/debug.h"
#include "sys/rf_messg.h"
#include "sys/conf.h"
#include "sys/uio.h"
#include "sys/pathname.h"
#include "sys/disp.h"
#include "sys/fbuf.h"
#include "sys/exec.h"
#include "sys/vm.h"
#include "sys/mman.h"
#include "sys/kmem.h"

#include "vm/hat.h"
#include "vm/anon.h"
#include "vm/as.h"
#include "vm/seg.h"
#include "vm/page.h"
#include "vm/seg_vn.h"
#include "vm/seg_kmem.h"
#include "vm/seg_map.h"
#include "sys/inline.h"

extern int exec_ncargs;
int exec_initialstk = (ctob(SSIZE));
/* this reference should be closer to md use */
extern int	mau_present;

/*
 * If the PREREAD(size) macro evaluates true, then we will read in
 * the given text or data a.out segment even though the file can be paged.
 */
#define	PREREAD(size) \
	((int)btopr(size) < (int)(freemem - minfree) && size < pgthresh)
STATIC int pgthresh = 0;

int nullmagic = 0;		/* null magic number */

struct execa {
	char	*fname;
	char	**argp;
	char	**envp;
};

/* ARGSUSED */
int
exece(uap, rvp)
	struct execa *uap;
	rval_t *rvp;
{
	long execsz;		/* temporary count of exec size */
	int error = 0;
	vnode_t *vp;
	char exec_file[PSCOMSIZ];
	struct pathname pn;
	struct uarg args;

	sysinfo.sysexec++;

	/*
	 * Can't do exec if there are any outstanding async I/O operations.
	 */
	if (u.u_procp->p_aiocount)
		return EINVAL;

	execsz = USIZE + SINCR + SSIZE + btoc(exec_ncargs-1);

	/*
	 * Lookup path name and remember last component for later.
	 */
	if (error = pn_get(uap->fname, UIO_USERSPACE, &pn))
		return error;
	if (error = lookuppn(&pn, FOLLOW, NULLVPP, &vp)) {
		pn_free(&pn);
		return error;
	}
	strncpy(exec_file, pn.pn_path, PSCOMSIZ);
	pn_free(&pn);
	struct_zero(&args, sizeof(args));

	if (uap->argp)
	switch (arglistsz(uap->argp, &args.argc, &args.argsize, exec_ncargs)) {
	case -2:
		return E2BIG;
	case -1:
		return EFAULT;
	default:
		args.argp = uap->argp;
		break;
	}

	if (uap->envp)
	switch (arglistsz(uap->envp, &args.envc, &args.envsize,
	    exec_ncargs - args.argsize)) {
	case -2:
		return E2BIG;
	case -1:
		return EFAULT;
	default:
		args.envp = uap->envp;
		break;
	}

	args.fname = uap->fname;


	PREEMPT();
	if (error = gexec(vp, &args, 0, &execsz))
		goto done;
	PREEMPT();

	u.u_execsz = execsz;	/* dependent portion should have checked */

	/*
	 * Remember file name for accounting.
	 */
	u.u_acflag &= ~AFORK;
	bcopy((caddr_t)exec_file, (caddr_t)u.u_comm, PSCOMSIZ);

	if ((error = setregs(&args)) != 0)
		psignal(u.u_procp, SIGKILL);

done:
	PREEMPT();
	VN_RELE(vp);
	return error;
}

/*
 * exec system calls, without and with environments.
 */
int
exec(uap, rvp)
	struct execa *uap;
	rval_t *rvp;
{
	uap->envp = NULL;
	return exece(uap, rvp);
}

exhdmap_t *exhd_freelist;
int exhd_freeincr = 8;

STATIC
int
exhd_getfbuf(ehdap, off, size, keep, mappp)
 exhda_t *ehdap;
 off_t off;
 int size;
 int keep;
 exhdmap_t **mappp;
{
	register exhdmap_t *mapp;
	extern struct as kas;
	off_t boff, eoff, bpoff, epoff;
	size_t len;
	register struct fbuf *fbp;
	faultcode_t err;
	int error;

	boff = off & MAXBMASK;
	bpoff = off & PAGEMASK;
	eoff = off + size;
	epoff = boff + MAXBSIZE;
	if (epoff > eoff)
		epoff = (eoff + PAGESIZE-1) & PAGEMASK;
	for (mapp = ehdap->maplist; mapp != NULL; mapp = mapp->nextmap) {
		fbp = mapp->fbufp;
		if (fbp == NULL)
			continue;
		if (mapp->curbase == boff) {
			if (bpoff < mapp->curoff) {
				len = (size_t)(mapp->curoff - bpoff);
				err = as_fault(&kas,
						fbp->fb_addr - len,
						len,
						F_SOFTLOCK,
						S_READ);
				if (err) {
					if (FC_CODE(err) == FC_OBJERR)
						return(FC_ERRNO(err));
					else
						return EIO;
				}
				fbp->fb_addr -= len;
				fbp->fb_count += len;
				mapp->curoff -= len;
			}
			if (epoff > mapp->cureoff) {
				len = epoff - mapp->cureoff;
				err = as_fault(&kas,
						fbp->fb_addr + fbp->fb_count,
						len,
						F_SOFTLOCK,
						S_READ);
				if (err) {
					if (FC_CODE(err) == FC_OBJERR)
						return(FC_ERRNO(err));
					else
						return EIO;
				}
				fbp->fb_count += len;
				mapp->cureoff = epoff;
			}
			*mappp = mapp;
			return 0;
		}
	}

	/* need a new fbuf */
	mapp = (exhdmap_t *) kmem_fast_alloc(
		(caddr_t *) &exhd_freelist,
		sizeof(*exhd_freelist),
		exhd_freeincr,
		KM_SLEEP);
	struct_zero(mapp, sizeof(*mapp));
	error = fbread(ehdap->vp, bpoff, epoff - bpoff, S_READ, &mapp->fbufp);
	if (error) {
		kmem_fast_free((caddr_t *) &exhd_freelist, (caddr_t)mapp);
		return(error);
	}
	mapp->nextmap = ehdap->maplist;
	ehdap->maplist = mapp;
	mapp->curbase = boff;
	mapp->curoff = bpoff;
	mapp->cureoff = epoff;
	*mappp = mapp;
	return 0;
}

STATIC
int
exhd_nomap(ehdap, off, size, flags, cpp)
 exhda_t *ehdap;
 off_t off;
 int size;
 int flags;
 caddr_t cpp;
{
	register exhdmap_t *mapp;
	register exhdmap_t **mpp;
	int error;
	int resid;
	off_t poff, eoff, epoff;
	register long cnt;
	exhdmap_t *copymapp = NULL;
	int icnt = 0;
	caddr_t tcp;
	vnode_t *vp = ehdap->vp;

	eoff = off + size;
	poff = off & PAGEMASK;
	epoff = (eoff + (PAGESIZE-1)) & PAGEMASK;

	/* the code rejects doing the autofree for VNOMAP files
	 * to avoid losing this hidden cache.
	 * So, only non-vnode pages are autofreed.
	 */

	for (mpp = &ehdap->maplist; (mapp = *mpp) != NULL; ) {
		if (mapp->keepcnt
		  || mapp->cureoff) {
			mpp = &mapp->nextmap;
			continue;
		}
		if (mapp->bndrycasep)
			kmem_free(mapp->bndrycasep, mapp->bndrycasesz);
		*mpp = mapp->nextmap;
		kmem_fast_free((caddr_t *) &exhd_freelist, (caddr_t)mapp);
	}
	if (!(flags & EXHD_COPY) && (flags & EXHD_4BALIGN) && (off & 3)) {
		mapp = (exhdmap_t *) kmem_fast_alloc(
			(caddr_t *) &exhd_freelist,
			sizeof(*exhd_freelist),
			exhd_freeincr,
			KM_SLEEP);
		struct_zero(mapp, sizeof(*mapp));
		mapp->bndrycasep = kmem_alloc(size, KM_SLEEP);
		mapp->bndrycasesz = size;
		if (flags & EXHD_KEEPMAP)
			mapp->keepcnt++;
		mapp->nextmap = ehdap->maplist;
		ehdap->maplist = mapp;
		copymapp = mapp;
	}

	/* keep the maplist sorted by offset and allow no overlaps */

	for (mpp = &ehdap->maplist; (mapp = *mpp) != NULL; ) {
		if (mapp->cureoff == 0) {
			mpp = &mapp->nextmap;
			continue;
		}
		if (mapp->curoff >= epoff)
			break;
		if (mapp->cureoff > poff && mapp->curoff < epoff) {
			if (mapp->curoff <= poff && mapp->cureoff >= epoff) {
				/* the simple case: it has it all */
				if (!(flags & EXHD_COPY)) {
					if (copymapp != NULL) {
						*((caddr_t *)cpp) =
							copymapp->bndrycasep;
						bcopy(mapp->bndrycasep + off - mapp->curoff,
							copymapp->bndrycasep, size);
						return 0;
					}
					*((caddr_t *)cpp) = mapp->bndrycasep
						+ (off - mapp->curoff);
					if (flags & EXHD_KEEPMAP)
						mapp->keepcnt++;
					return 0;
				}
				else {
					bcopy(mapp->bndrycasep
						+ (off-mapp->curoff),
						cpp, size);
					return 0;
				}
			}
			icnt++;
			break;
		}
		mpp = &mapp->nextmap;
	}
	if (icnt == 0) {
		mapp = (exhdmap_t *) kmem_fast_alloc(
			(caddr_t *) &exhd_freelist,
			sizeof(*exhd_freelist),
			exhd_freeincr,
			KM_SLEEP);
		struct_zero(mapp, sizeof(*mapp));
		cnt = epoff - poff;
		mapp->bndrycasep = kmem_alloc(cnt, KM_SLEEP);
		mapp->bndrycasesz = cnt;
		mapp->curoff = poff;
		mapp->cureoff = epoff;
		error = vn_rdwr(UIO_READ, vp, mapp->bndrycasep, cnt, poff,
			UIO_SYSSPACE, 0, (long) 0, u.u_cred, &resid);
		if (error || resid) {
			kmem_fast_free((caddr_t *) &exhd_freelist, (caddr_t)mapp);
			ehdap->state = EXHDA_HADERROR;
			if (error)
				return(error);
			return(ENOEXEC);
		}
		mapp->nextmap = *mpp;
		*mpp = mapp;
		if (!(flags & EXHD_COPY)) {
			if (copymapp != NULL) {
				*((caddr_t *)cpp) =
					copymapp->bndrycasep;
				bcopy(mapp->bndrycasep + off - mapp->curoff,
					copymapp->bndrycasep, size);
				return 0;
			}
			*((caddr_t *)cpp) = mapp->bndrycasep
				+ (off - mapp->curoff);
			if (flags & EXHD_KEEPMAP)
				mapp->keepcnt++;
			return 0;
		}
		else {
			bcopy(mapp->bndrycasep + (off-mapp->curoff),
				cpp, size);
			return 0;
		}
	}
	/* a partial overlap:
	 * copy to a separate buffer
	 * rather than fiddling with merging vnode pages.
	 */

	if (!(flags & EXHD_COPY)) {
		if (copymapp)
			tcp = copymapp->bndrycasep;
		else {
			mapp = (exhdmap_t *) kmem_fast_alloc(
				(caddr_t *) &exhd_freelist,
				sizeof(*exhd_freelist),
				exhd_freeincr,
				KM_SLEEP);
			struct_zero(mapp, sizeof(*mapp));
			mapp->bndrycasep = kmem_alloc(size, KM_SLEEP);
			mapp->bndrycasesz = size;
			if (flags & EXHD_KEEPMAP)
				mapp->keepcnt++;
			mapp->nextmap = *mpp;
			*mpp = mapp;
			tcp = mapp->bndrycasep;
			mpp = &mapp->nextmap;
			mapp = mapp->nextmap;
		}
		*((caddr_t *)cpp) = tcp;
	} else
		tcp = cpp;
	ASSERT(mapp != NULL);
	while ((mapp = *mpp) != NULL) {
		if (mapp->cureoff == 0) {
			mpp = &mapp->nextmap;
			continue;
		}
		if (mapp->curoff > off) {
			poff = off & PAGEMASK;
			cnt = mapp->curoff - poff;
			if (cnt > size)
				cnt = size;
			cnt = (cnt + (PAGESIZE-1)) & PAGEMASK;
			mapp = (exhdmap_t *) kmem_fast_alloc(
				(caddr_t *) &exhd_freelist,
				sizeof(*exhd_freelist),
				exhd_freeincr,
				KM_SLEEP);
			struct_zero(mapp, sizeof(*mapp));
			mapp->bndrycasep = kmem_alloc(cnt, KM_SLEEP);
			mapp->bndrycasesz = cnt;
			mapp->nextmap = *mpp;
			*mpp = mapp;
			error = vn_rdwr(UIO_READ, vp, mapp->bndrycasep,
				cnt, poff,
				UIO_SYSSPACE, 0, (long) 0, u.u_cred, &resid);
			if (error || resid) {
				ehdap->state = EXHDA_HADERROR;
				if (error)
					return(error);
				return(ENOEXEC);
			}
			mapp->curoff = poff;
			mapp->cureoff = poff + cnt;
		}
		ASSERT(off >= mapp->curoff && off < mapp->cureoff);
		cnt = mapp->cureoff - off;
		if (cnt > size)
			cnt = size;
		bcopy(mapp->bndrycasep + off - mapp->curoff, tcp, cnt);
		if ((size -= cnt) <= 0)
			return(0);
		off += cnt;
		tcp += cnt;
		mpp = &mapp->nextmap;
	}
	cnt = size;
	cnt = (cnt + (PAGESIZE-1)) & PAGEMASK;
	mapp = (exhdmap_t *) kmem_fast_alloc(
		(caddr_t *) &exhd_freelist,
		sizeof(*exhd_freelist),
		exhd_freeincr,
		KM_SLEEP);
	struct_zero(mapp, sizeof(*mapp));
	mapp->bndrycasep = kmem_alloc(cnt, KM_SLEEP);
	mapp->bndrycasesz = cnt;
	mapp->nextmap = *mpp;
	*mpp = mapp;
	error = vn_rdwr(UIO_READ, vp, mapp->bndrycasep, cnt, off,
		UIO_SYSSPACE, 0, (long) 0, u.u_cred, &resid);
	if (error || resid) {
		ehdap->state = EXHDA_HADERROR;
		if (error)
			return(error);
		return(ENOEXEC);
	}
	mapp->curoff = off;
	mapp->cureoff = off + cnt;
	bcopy(mapp->bndrycasep, tcp, size);
	return(0);
}

int
exhd_getmap(ehdap, off, size, flags, cpp)
 exhda_t *ehdap;
 off_t off;
 int size;
 int flags;
 caddr_t cpp;
{
	register exhdmap_t *mapp;
	register exhdmap_t **mpp;
	int error;
	off_t boff, eoff, eboff;
	register long cnt;
	char *fcp, *tcp;
	exhdmap_t *curmapp;

	ASSERT(size > 0);
	if (ehdap->state == EXHDA_HADERROR)
		return(ENOEXEC);	/* we failed previously */
	eoff = off + size;
	if (eoff < off || eoff > ehdap->vnsize) {
		ehdap->state = EXHDA_HADERROR;
		return(ENOEXEC);
	}

	/* assumption: the mappability of a vnode is constant during exec */

/*
	if (ehdap->vp->v_flag & VNOMAP)
*/
	if(ehdap->nomap)
		return(exhd_nomap(ehdap, off, size, flags, cpp));
	boff = off & MAXBMASK;
	eboff = (eoff-1) & MAXBMASK;
	for (mpp = &ehdap->maplist; (mapp = *mpp) != NULL; ) {
		if (mapp->keepcnt
		  || (mapp->cureoff && mapp->curbase >= boff
		      && mapp->curbase <= eboff)) {
			mpp = &mapp->nextmap;
			continue;
		}
		if (mapp->bndrycasep)
			kmem_free(mapp->bndrycasep, mapp->bndrycasesz);
		if (mapp->fbufp)
			fbrelse(mapp->fbufp, S_READ);
		*mpp = mapp->nextmap;
		kmem_fast_free((caddr_t *) &exhd_freelist, (caddr_t)mapp);
	}
	if (!(flags & EXHD_COPY || boff != eboff
	    || !((flags & EXHD_4BALIGN) == 0 || (off & 3) == 0))) {

		/* the simple case of returning a pointer to seg_map space */

		error = exhd_getfbuf(ehdap, off, size, flags & EXHD_KEEPMAP,
			&curmapp);
		if (error) {
			ehdap->state = EXHDA_HADERROR;
			return(error);
		}
		mapp = curmapp;
		*((caddr_t *)cpp) = mapp->fbufp->fb_addr + (off - mapp->curoff);
		return(0);
	}

	if (flags & EXHD_COPY)
		tcp = (caddr_t) cpp;
	else {
		tcp = kmem_alloc(size, KM_SLEEP);
		mapp = (exhdmap_t *) kmem_fast_alloc(
			(caddr_t *) &exhd_freelist,
			sizeof(*exhd_freelist),
			exhd_freeincr,
			KM_SLEEP);
		struct_zero(mapp, sizeof(*mapp));
		mapp->nextmap = ehdap->maplist;
		ehdap->maplist = mapp;
		mapp->bndrycasep = tcp;
		mapp->bndrycasesz = size;
		if (flags & EXHD_KEEPMAP)
			mapp->keepcnt = 1;
		*((caddr_t *) cpp) = tcp;
	}
	error = exhd_getfbuf(ehdap, off, size, flags & EXHD_KEEPMAP,
		&curmapp);
	if (error) {
		ehdap->state = EXHDA_HADERROR;
		return(error);
	}
	mapp = curmapp;
	fcp = mapp->fbufp->fb_addr + (off - mapp->curoff);
	eoff = mapp->cureoff;
	cnt = eoff - off;
	if (cnt > size) cnt = size;
	for (;;) {
		bcopy(fcp, tcp, cnt);
		if ((size -= cnt) <= 0)
			return(0);
		tcp += cnt;
		off += cnt;
		error = exhd_getfbuf(ehdap, off, size, flags & EXHD_KEEPMAP,
			&curmapp);
		if (error) {
			ehdap->state = EXHDA_HADERROR;
			return(error);
		}
		mapp = curmapp;
		fcp = mapp->fbufp->fb_addr + (off - mapp->curoff);
		eoff = mapp->cureoff;
		cnt = eoff - off;
		if (cnt > size) cnt = size;
	}
}

void
exhd_release(hdp)
 register exhda_t *hdp;
{
	register exhdmap_t *mapp, *nmapp;

	if (hdp == NULL) return;
	for (mapp = hdp->maplist; mapp != NULL; mapp = nmapp) {
		if (mapp->bndrycasep)
			kmem_free(mapp->bndrycasep, mapp->bndrycasesz);
		if (mapp->fbufp)
			fbrelse(mapp->fbufp, S_READ);
		nmapp = mapp->nextmap;
		kmem_fast_free((caddr_t *) &exhd_freelist, (caddr_t)mapp);
	}
}

int
execpermissions(vp, vattrp, ehdp, args)
struct vnode *vp;
struct vattr *vattrp;
exhda_t *ehdp;
struct uarg *args;
{
	int error;

	struct_zero(ehdp, sizeof(*ehdp));
	vattrp->va_mask = AT_MODE|AT_UID|AT_GID|AT_SIZE;
	if (error = VOP_GETATTR(vp, vattrp, ATTR_EXEC, u.u_cred))
		return error;
	/*
	 * Check the access mode.
	 */
	if ((error = VOP_ACCESS(vp, VEXEC, 0, u.u_cred)) != 0
	  || vp->v_type != VREG
	  || (vattrp->va_mode & (VEXEC|(VEXEC>>3)|(VEXEC>>6))) == 0) {
		if (error == 0)
			error = EACCES;
		return error;
	}

       if ((u.u_procp->p_trace || (u.u_procp->p_flag & (STRC|SPROCTR)))
          && (error = VOP_ACCESS(vp, VREAD, 0, u.u_cred))) {
		/*
                 * If process is traced via ptrace(2), fail the exec(2).
                 */
                if (u.u_procp->p_flag & STRC)
                        goto bad;
		/*
                 * Process is traced via /proc.
                 * Arrange to invalidate the /proc vnode.
                 */
                args->traceinval = 1;
	}

	ehdp->vp = vp;
	ehdp->vnsize = vattrp->va_size;
	ehdp->nomap = vp->v_flag & VNOMAP;
	return 0;
bad:
	if (error == 0)
		error = ENOEXEC;
	return error;
}

int
gexec(vp, args, level, execsz)
	struct vnode *vp;
	struct uarg *args;
	int level;
	long *execsz;
{
	register proc_t *pp = u.u_procp;
	register int i;
	int error = 0;
	int resid;
	uid_t uid, gid;
	struct vattr vattr;
	short magic;
	char *mcp;
	exhda_t ehda;

	if ((error = execpermissions(vp, &vattr, &ehda, args)) != 0)
		goto bad;

	if ((error = exhd_getmap(&ehda, 0, 2, EXHD_NOALIGN, (caddr_t)&mcp)) != 0) {
		exhd_release(&ehda);
		goto bad;
	}
	magic = getexmag(mcp);

	for (i = 0; i < nexectype; i++) {
		if (magic == *execsw[i].exec_magic) {
			error = (*execsw[i].exec_func)(vp, args, level, execsz, &ehda);
			break;
		}
	}
	exhd_release(&ehda);

	if (i >= nexectype || error)
		goto bad;

	if (level == 0) {

		/*
		 * Remember credentials.
		 */

		uid = u.u_cred->cr_uid;
		gid = u.u_cred->cr_gid;
	
		if ((vp->v_vfsp->vfs_flag & VFS_NOSUID) == 0) {
			if (vattr.va_mode & VSUID)
				uid = vattr.va_uid;
			if (vattr.va_mode & VSGID)
				gid = vattr.va_gid;
		}
	
		/*
	 	 * Set setuid/setgid protections, if no tracing.  
		 * For the super-user, honor setuid/setgid even in 
		 * the presence of tracing.
	 	 */

		if (((pp->p_flag & STRC) == 0 || u.u_cred->cr_uid == 0)
	  	&& (u.u_cred->cr_uid != uid || u.u_cred->cr_gid != gid)) {

			/*
			 * prevent unprivledged processes from enforcint
			 * resource limitations on setuid/setgid processes
			 */
			for (i = 0; i < RLIM_NLIMITS; i++) {
				u.u_rlimit[i].rlim_cur = rlimits[i].rlim_cur;
				u.u_rlimit[i].rlim_max = rlimits[i].rlim_max;
			}

			u.u_cred = crcopy(u.u_cred);
			u.u_cred->cr_uid = uid;
			u.u_cred->cr_gid = gid;
			u.u_cred->cr_suid = uid;
			u.u_cred->cr_sgid = gid;
			if ( uid < USHRT_MAX)
				u.u_uid = (o_uid_t) uid;
			else
				u.u_uid = (o_uid_t) UID_NOBODY;

			if ( gid < USHRT_MAX)
				u.u_gid = (o_gid_t) gid;
			else
				u.u_gid = (o_gid_t) UID_NOBODY;

			/*
			 * If process is traced via /proc, arrange to
			 * invalidate the associated /proc vnode.
			 */

			if (pp->p_trace || (pp->p_flag & SPROCTR))
				args->traceinval = 1;
		}

		if (pp->p_flag & STRC)
			psignal(pp, SIGTRAP);

		if (args->traceinval)
			prinvalidate(&u);
	}

	return 0;
bad:
	if (error == 0)
		error = ENOEXEC;
	return error;
}

int
execmap(vp, addr, len, zfodlen, offset, prot)
	struct vnode *vp;
	caddr_t addr; 
	size_t len, zfodlen;
	off_t  offset;
	int prot;
{
	int error = 0;
	int page = 0;
	caddr_t zfodbase, oldaddr;
	size_t zfoddiff, end, oldlen;
	proc_t *p = u.u_procp;
	off_t oldoffset;


	if (((long)offset & PAGEOFFSET) == ((long)addr & PAGEOFFSET)
		&& (!(vp->v_flag & VNOMAP))) {
			page = 1;
	}
		
	oldaddr = addr;
	addr = (caddr_t)((long)addr & PAGEMASK);
	if (len) {
		oldlen = len;
		len += ((size_t)oldaddr - (size_t)addr);
		oldoffset = offset;
		offset = (off_t)((long)offset & PAGEMASK);
		if (page) {
			if (error = VOP_MAP(vp, offset, p->p_as, &addr,
				len, prot, PROT_ALL,
				 MAP_PRIVATE | MAP_FIXED, u.u_cred))
					goto bad;
			/*
			 * If the segment can fit, then we prefault
			 * the entire segment in.  This is based on the
			 * model that says the best working set of a
			 * small program is all of its pages.
			 */
			if (PREREAD(len)) {
				(void) as_fault(p->p_as, (caddr_t)addr,
				  	   	 len, F_INVAL, S_READ);
			}

		} else {	
			if (error = as_map(p->p_as, addr, len,
			  segvn_create, zfod_argsp))
				goto bad;
			/*
			 * Read in the segment in one big chunk.
			 */
			if (error = vn_rdwr(UIO_READ, vp, (caddr_t)oldaddr,
			  oldlen, oldoffset, UIO_USERSPACE, 0,
			  (u_long) 0, u.u_cred, (int *)0))
				goto bad;
			/*
			 * Now set protections.
			 */
			(void)as_setprot(p->p_as, (caddr_t)addr, len, prot);
		}
	}

	if (zfodlen){
		end = (size_t)addr + len;
		zfodbase = (caddr_t)roundup(end, PAGESIZE);
		zfoddiff = (size_t)zfodbase - end;
		if (zfoddiff != 0)
			bzeroba((caddr_t)end, zfoddiff);
		if ( zfodlen > zfoddiff){
			zfodlen -= zfoddiff;
			if (error = as_map(p->p_as, (caddr_t)zfodbase, zfodlen,
				segvn_create, zfod_argsp))
				goto bad;

			(void)as_setprot(p->p_as, (caddr_t)zfodbase, 
					  zfodlen, prot);
		}
	}

		
	return 0;
bad:
	return error;
}

/*
 * machine independent final setup code goes in setexecenv().
 */

void
setexecenv(ep)
struct execenv *ep;
{
	register int	i;
	register struct proc *p = u.u_procp;
	file_t *fp;

	u.u_execid = (int)ep->ex_magic;
	p->p_brkbase = ep->ex_brkbase;
	p->p_brksize = 0;
	p->p_exec = ep->ex_vp;

	u.u_oldcontext = 0;

	u.u_sigresethand = 0;
	u.u_signodefer = 0;
	u.u_sigonstack = 0;

	u.u_altsp = 0;
	u.u_altsize = 0;
	u.u_altflags = SS_DISABLE;

	/*
	 * Any pending signal remain held, so don't clear p_hold and
	 * p_sig.
	 */	

	/*
	 * If the action was to catch the signal, then the action
	 * must be reset to SIG_DFL.
	 */
	for (i = 1; i < NSIG; i++) {
		if (u.u_signal[i-1] != SIG_DFL && u.u_signal[i-1] != SIG_IGN) {
			ev_signal(p, i);
			u.u_signal[i - 1] = SIG_DFL;
			sigemptyset(&u.u_sigmask[i - 1]);
			if (sigismember(&ignoredefault, i))
				sigdelq(p, i);
		}
	}

	sigorset(&p->p_ignore, &ignoredefault);
	sigdiffset(&p->p_siginfo, &ignoredefault);
	sigdiffset(&p->p_sig, &ignoredefault);

	p->p_flag &= ~(SNOWAIT|SJCTL);
	p->p_flag |= SEXECED;

	detachcld(p);


	/* Clear illegal opcode handler. */
	u.u_iop = NULL;

	for (i = 0; i < u.u_nofiles; i++) {
		if (getf(i, &fp) == 0 && (getpof(i) & FCLOSEXEC)) {
			closef(fp);
			setf(i, NULLFP);
		}
	}
}

int
remove_proc(args)
	struct uarg	*args;
{
	extern void shmexec();
	extern void punlock();
	register struct proc *p;	/* process exiting or exec'ing */
	struct as *nas;
	int error;

	p = u.u_procp;
	punlock();
	
	u.u_prof.pr_scale = 0;

	if (error = extractarg(args)) {
		return(error);
	}
	ev_exec(p);

	if (u.u_nshmseg)
		shmexec(p);
/* #ifdef MERGE */
	if (p->p_sdp)
		xsdexit();
/* #endif MERGE */

	u.u_nshmseg = u.u_dmm = 0;

	nas = as_alloc();
	as_exec(p->p_as, args->estkstart, args->estksize,
		nas, args->stacklow, args->estkhflag);
	relvm(p);

	p->p_as = nas;
	hat_asload();
	return 0;
}

/* ARGSUSED */
noexec(vp, args, level, ehdp)
	struct vnode *vp;
	struct uarg *args;
	int level;
	exhda_t *ehdp;
{
	cmn_err(CE_WARN, "missing exec capability for %s\n", args->fname);
	return ENOEXEC;
}
