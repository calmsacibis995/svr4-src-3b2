/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/core.c	1.15"
#include "sys/param.h"
#include "sys/types.h"
#include "sys/psw.h"
#include "sys/sysmacros.h"
#include "sys/immu.h"
#include "sys/pcb.h"
#include "sys/systm.h"
#include "sys/sbd.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/proc.h"
#include "sys/disp.h"
#include "sys/signal.h"
#include "sys/siginfo.h"
#include "sys/fault.h"
#include "sys/syscall.h"
#include "sys/ucontext.h"
#include "sys/procfs.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/reg.h"
#include "sys/var.h"
#include "sys/debug.h"
#include "sys/inline.h"
#include "sys/uio.h"
#include "sys/mman.h"
#include "sys/rf_messg.h"
#include "sys/exec.h"
#include "sys/file.h"

#include "vm/as.h"

/*
 * Create a core image on the file "core".
 */
int
core(fp, pp, credp, rlimit, sig)
	char *fp;
	proc_t *pp;
	struct cred *credp;
	rlim_t rlimit;
	int sig;
{
	extern int nexectype;
	struct vnode *vp;
	struct vattr vattr;
	register int error, i, closerr;
	register cred_t *crp = pp->p_cred;
	mode_t umask = PTOU(pp)->u_cmask;

	if (crp->cr_uid != crp->cr_ruid || !hasprocperm(crp, credp))
		return EPERM;

	/*
	 * The original intent of the core function interface was to
	 * be able to core dump any process, not just the context we
	 * are currently in. Unfortunately, we never got around to
	 * writing the code to deal with any other context but the current.
	 * This is not so bad as currently there is no user interface to 
	 * get here with a non-current context. We will fix this later when 
	 * an interface is provided to core dump a selected process.
	 */
	if (pp != curproc)  	/* only support current context for now */
		return EINVAL;

	PTOU(pp)->u_syscall = DUCOREDUMP;	/* RFS */
	error = vn_open(fp, UIO_SYSSPACE, FWRITE | FTRUNC | FCREAT,
	  (0666 & ~umask) & PERMMASK, &vp, CRCORE);
	if (error)
		return error;

	if (VOP_ACCESS(vp, VWRITE, 0, credp) || vp->v_type != VREG)
		error = EACCES;
	else {
		vattr.va_size = 0;
		vattr.va_mask = AT_SIZE;
		(void) VOP_SETATTR(vp, &vattr, 0, credp);
		for (i = 0; i < nexectype; i++) {
			if ((short)PTOU(pp)->u_execid == 
					         (*execsw[i].exec_magic)) {
				error = (*execsw[i].exec_core)(vp, pp, 
						credp, rlimit, sig);
				break;
			}
		}
		if (i >= nexectype)
			error = ENOSYS;
	}

	closerr = VOP_CLOSE(vp, FWRITE, 1, 0, credp);
	VN_RELE(vp);

	return error ? error : closerr;
}

/*
 * Common code to core dump process memory.
 */
int
core_seg(pp, vp, offset, addr, size, rlimit, credp)
	proc_t *pp;
	vnode_t *vp;
	off_t offset;
	register caddr_t addr;
	size_t size;
	rlim_t rlimit;
	struct cred *credp;
{
	register addr_t eaddr;
	addr_t base;
	u_int len;
	register int err = 0;

	eaddr = (addr_t)(addr + size);
	for (base = addr; base < eaddr; base += len) {
		len = eaddr - base;
		if (as_memory(pp->p_as, &base, &len) != 0)
			return 0;
		err = vn_rdwr(UIO_WRITE, vp, base, (int)len,
		  offset + (base - (addr_t)addr), UIO_USERSPACE, 0,
		  rlimit, credp, (int *)NULL);
		if (err)
			return err;
	}
	return 0;
}

/* ARGSUSED */
nocore(vp, pp, credp, rlimit, sig)
        vnode_t *vp;
        proc_t *pp;
        struct cred *credp;
        rlim_t rlimit;
        int sig;

{
	return ENOSYS;
}
