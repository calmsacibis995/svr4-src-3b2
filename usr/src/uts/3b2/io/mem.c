/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/mem.c	1.14"
#include "sys/types.h"
#include "sys/sbd.h"
#include "sys/param.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/immu.h"
#include "sys/sysmacros.h"	/* define before ddi.h */
#include "sys/systm.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/firmware.h"
#include "sys/iobd.h"
#include "sys/uio.h"
#include "sys/cred.h"
#include "sys/proc.h"
#include "sys/disp.h"
#include "sys/debug.h"
#include "sys/ddi.h"

#include "sys/mman.h"

#include "sys/vmsystm.h"
#include "vm/hat.h"
#include "vm/as.h"
#include "vm/seg.h"
#include "vm/seg_vn.h"
#include "vm/seg_kmem.h"

#ifdef __STDC__
STATIC int mmrw(dev_t, struct uio *, struct cred *, enum uio_rw);
#else
STATIC int mmrw();
#endif

#define VSIZEOFMEM (*(((struct vectors *)(BASE+0x20000))->p_memsize))

#define	M_MEM		0	/* /dev/mem - physical main memory */
#define	M_KMEM		1	/* /dev/kmem - virtual kernel memory & I/O */
#define	M_NULL		2	/* /dev/null - EOF & Rathole */
#define	M_ZERO		3	/* /dev/zero - source of private memory */

int mmdevflag = 2;

/*
 * Avoid addressing invalid kernel page.  This can happen, for example,
 * if a server process issues a read or write after seeking to a bad address.
 */
extern int memprobe();

/* ARGSUSED */
int
mmopen(devp, flag, type, cr)
        dev_t *devp;
        int flag;
	int type;
        struct cred *cr;
{
        return 0;
}

/* ARGSUSED */
int
mmclose(dev, flag, cr)
        dev_t dev;
        int flag;
        struct cred *cr;
{
        return 0;
}

/* ARGSUSED */
int
mmioctl(vp, cmd, arg, flag, cr, rvalp)
	register struct vnode *vp;
	int cmd;
	int arg;
	int flag;
	struct cred *cr;
	int *rvalp;
{
	return ENODEV;
}

/* ARGSUSED */
int
mmread(dev, uiop, cr)
        dev_t dev;
        struct uio *uiop;
        struct cred *cr;
{
	return (mmrw(dev, uiop, cr, UIO_READ));
}

/* ARGSUSED */
int
mmwrite(dev, uiop, cr)
	dev_t dev;
        struct uio *uiop;
        struct cred *cr;
{
	return (mmrw(dev, uiop, cr, UIO_WRITE));
}

/*
 * When reading the M_ZERO device, we simply copyout the zeroes
 * array in NZEROES sized chunks to the user's address.
 *
 * XXX - this is not very elegant and should be redone.
 */
#define NZEROES		0x100
static char zeroes[NZEROES];

/* ARGSUSED */
STATIC int
mmrw(dev, uiop, cr, rw)
	dev_t dev;
	struct uio *uiop;
        struct cred *cr;
	enum uio_rw rw;
{
	register off_t n;
	int error = 0;

        while (error == 0 && uiop->uio_resid != 0) {
		/* It may take long here, so we put in a preemption point */
		PREEMPT();
		/*
		 * Don't cross page boundaries.  uio_offset could be
		 * negative, so don't just take a simpler MIN.
		 */
                n = MIN(MIN(uiop->uio_resid, ctob(1)),
			ctob(1) - uiop->uio_offset % ctob(1));
		switch (getminor(dev)) {

		case M_MEM:
			/*
			 * Don't read from mainstore we don't have;
			 * fail also for invalid page or uiomove
			 * failure.
			 */
			if (uiop->uio_offset >= VSIZEOFMEM
			  || uiop->uio_offset < 0
			  || memprobe((caddr_t)(uiop->uio_offset + MAINSTORE))
			  || uiomove(uiop->uio_offset + MAINSTORE,
			    n, rw, uiop)) {
	                       	error = ENXIO;
			}
			break;

		case M_KMEM:
			/*
			 * Don't try to read from I/O ports.  This gives
			 * a system bus timeout if there is no board in
			 * the port; fail also for invalid page or
			 * uiomove failure.
			 */
			if ((uiop->uio_offset >= VKIO
			    && uiop->uio_offset < VKIO + 12 * ctob(VKIOSZ))
			    || memprobe((caddr_t)(uiop->uio_offset))
			    || uiomove(uiop->uio_offset, n, rw, uiop)) {
				error = ENXIO;
			}
			break;

		case M_ZERO:
			if (rw == UIO_READ) {
				n = MIN(n, sizeof (zeroes));
				if (uiomove(zeroes, n, rw, uiop))
					error = ENXIO;
				break;
			}
			/* FALLTHROUGH */

		case M_NULL:
			if (rw == UIO_WRITE) {
				uiop->uio_offset += uiop->uio_resid;
				uiop->uio_resid = 0;
			}
			return 0;
		}
	}
	return (error);
}


/*ARGSUSED*/
int
mmmmap(dev, off, prot)
	dev_t dev;
	off_t off;
{
	int pf;

	switch (getminor(dev)) {

	case M_MEM:
		/*
		 * Don't read from mainstore we don't have;
		 * fail also for invalid page.
		 */
		if (off >= VSIZEOFMEM || off < 0
		  || memprobe((caddr_t)(off + MAINSTORE))) {
			break;
		}
		pf = btoc(off + MAINSTORE);
		return pf;

	case M_KMEM:
		/*
		 * Don't try to read from I/O ports.  This gives
		 * a system bus timeout if there is no board in
		 * the port; fail also for invalid page.
		 */
		if ((off >= VKIO && off < VKIO + 12 * ctob(VKIOSZ))
		    || memprobe((caddr_t)(off))
		    || ((pf=(int)vtop((caddr_t)off, u.u_procp)) == 0) )
			break;

		return(btoc(pf));

	case M_ZERO:
		/*
		 * We shouldn't be mmap'ing to /dev/zero here as this
		 * mmsegmap should have already converted the mapping
		 * request for this device to a mapping using seg_vn.
		 */
	default:
		break;
	}
	return (-1);
}

/*
 * This function is called when a memory device is mmap'ed.
 * Set up the mapping to the correct device driver.
 */
int
mmsegmap(dev, off, as, addrp, len, prot, maxprot, flags, cred)
	dev_t dev;
	u_int off;
	struct as *as;
	addr_t *addrp;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct cred *cred;
{
	struct segvn_crargs vn_a;

	/*
	 * If we are not mapping /dev/zero, then use spec_segmap()
	 * to set up the mapping which resolves to using mmmap().
	 */
	if (getminor(dev) != M_ZERO) {
		return (spec_segmap(dev, off, as, addrp, len, prot, maxprot,
		    flags, cred));
	}

	if ((flags & MAP_FIXED) == 0) {
		/*
		 * No need to worry about vac alignment since this
		 * is a "clone" object that doesn't yet exist.
		 */
		map_addr(addrp, len, (off_t)off, 0);
		if (*addrp == NULL)
			return (ENOMEM);
	} else {
		/*
		 * User specified address -
		 * Blow away any previous mappings.
		 */
		(void) as_unmap(as, *addrp, len);
	}

	/*
	 * Use seg_vn segment driver for /dev/zero mapping.
	 * Passing in a NULL amp gives us the "cloning" effect.
	 */
	vn_a.vp = NULL;
	vn_a.offset = 0;
	vn_a.type = (flags & MAP_TYPE);
	vn_a.prot = (u_char)prot;
	vn_a.maxprot = (u_char)maxprot;
	vn_a.cred = cred;
	vn_a.amp = NULL;

	return (as_map(as, *addrp, len, segvn_create, (caddr_t)&vn_a));
}
