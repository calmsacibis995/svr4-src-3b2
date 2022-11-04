/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/move.c	1.9.1.2"

#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/param.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/sbd.h"
#include "sys/immu.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/buf.h"
#include "sys/conf.h"
#include "sys/cred.h"
#include "sys/vnode.h"
#include "sys/file.h"
#include "sys/systm.h"
#include "sys/uio.h"

/*
 * Move "n" bytes at byte address "cp"; "rw" indicates the direction
 * of the move, and the I/O parameters are provided in "uio", which is
 * update to reflect the data which was moved.  Returns 0 on success or
 * a non-zero errno on failure.
 */
int
uiomove(cp, n, rw, uio)
	register caddr_t cp;
	register long n;
	enum uio_rw rw;
	register struct uio *uio;
{
	register struct iovec *iov;
	u_int cnt;

	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = MIN(iov->iov_len, n);
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		switch (uio->uio_segflg) {

		case UIO_USERSPACE:
		case UIO_USERISPACE:
			if (rw == UIO_READ) {
				if (copyout(cp, iov->iov_base, cnt))
					return EFAULT;
			} else if (copyin(iov->iov_base, cp, cnt))
				return EFAULT;
			break;

		case UIO_SYSSPACE:
			if (rw == UIO_READ)
				bcopy((caddr_t)cp, iov->iov_base, cnt);
			else
				bcopy(iov->iov_base, (caddr_t)cp, cnt);
			break;
		}
		iov->iov_base += cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp += cnt;
		n -= cnt;
	}
	return 0;
}



/* function: ureadc()
 * purpose:  transfer a character value into the address space
 *           delineated by a uio and update fields within the
 *           uio for next character. Return 0 for success, EFAULT
 *           for error.
 */

int
ureadc(val, uiop)
	int val;
	register struct uio *uiop;
{
	register struct iovec *iovp;
	unsigned char c;

	/*
	 * first determine if uio is valid.  uiop should be 
	 * non-NULL and the resid count > 0.
	 */
	if (!(uiop && uiop->uio_resid > 0)) 
		return EFAULT;

	/*
	 * scan through iovecs until one is found that is non-empty.
	 * Return EFAULT if none found.
	 */
	while (uiop->uio_iovcnt > 0) {
		iovp = uiop->uio_iov;
		if (iovp->iov_len <= 0) {
			uiop->uio_iovcnt--;
			uiop->uio_iov++;
		} else
			break;
	}

	if (uiop->uio_iovcnt <= 0)
		return EFAULT;

	/*
	 * Transfer character to uio space.
	 */

	c = (unsigned char) (val & 0xFF);

	switch (uiop->uio_segflg) {

	case UIO_USERISPACE:
	case UIO_USERSPACE:
		if (copyout((caddr_t)&c, iovp->iov_base, sizeof(unsigned char)))
			return EFAULT;
		break;

	case UIO_SYSSPACE: /* can do direct copy since kernel-kernel */
		*iovp->iov_base = c;
		break;

	default:
		return EFAULT; /* invalid segflg value */
	}

	/*
	 * bump up/down iovec and uio members to reflect transfer.
	 */
	iovp->iov_base++;
	iovp->iov_len--;
	uiop->uio_resid--;
	uiop->uio_offset++;
	return 0; /* success */
}


/* function: uwritec()
 * purpose:  return a character value from the address space
 *           delineated by a uio and update fields within the
 *           uio for next character. Return the character for success,
 *           -1 for error.
 */

int
uwritec(uiop)
	register struct uio *uiop;
{
	register struct iovec *iovp;
	unsigned char c;

	/* verify we were passed a valid uio structure.
	 * (1) non-NULL uiop, (2) positive resid count
	 * (3) there is an iovec with positive length 
	 */

	if (!(uiop && uiop->uio_resid > 0)) 
		return -1;

	while (uiop->uio_iovcnt > 0) {
		iovp = uiop->uio_iov;
		if (iovp->iov_len <= 0) {
			uiop->uio_iovcnt--;
			uiop->uio_iov++;
		} else
			break;
	}

	if (uiop->uio_iovcnt <= 0)
		return -1;

	/*
	 * Get the character from the uio address space.
	 */
	switch (uiop->uio_segflg) {

	case UIO_USERISPACE:
	case UIO_USERSPACE:
		if (copyin(iovp->iov_base, (caddr_t)&c, sizeof(unsigned char))) 
			return -1;
		break;

	case UIO_SYSSPACE:
		c = *iovp->iov_base;
		break;

	default:
		return -1; /* invalid segflg */
	}

	/*
	 * Adjust fields of iovec and uio appropriately.
	 */
	iovp->iov_base++;
	iovp->iov_len--;
	uiop->uio_resid--;
	uiop->uio_offset++;
	return (int)c & 0xFF; /* success */
}

/*
 * Drop the next n chars out of *uiop.
 */
void
uioskip(uiop, n)
	register uio_t	*uiop;
	register size_t	n;
{
	if (n > uiop->uio_resid)
		return;
	while (n != 0) {
		register iovec_t	*iovp = uiop->uio_iov;
		register size_t		niovb = MIN(iovp->iov_len, n);

		if (niovb == 0) {
			uiop->uio_iov++;
			uiop->uio_iovcnt--;
			continue;
		}	
		iovp->iov_base += niovb;
		uiop->uio_offset += niovb;
		iovp->iov_len -= niovb;
		uiop->uio_resid -= niovb;
		n -= niovb;
	}
}

/*
 * Overlapping bcopy (source and target may overlap arbitrarily).
 */
void
ovbcopy(from, to, count)
	register char *from;
	register char *to;
	register u_int count;
{
	register int diff;

	if ((diff = from - to) < 0)
		diff = -diff;
	if (from < to && count > diff) {
		do {
			count--;
			*(to + count) = *(from + count);
		} while (count);
	} else if (from != to) {
		while (count--)
			*to++ = *from++;
	}
}

int
copyinstr(from, to, max, np)
	char *from;	/* Source address (user space) */
	char *to;	/* Destination address */
	size_t max;	/* Maximum number of characters to move */
	size_t *np;	/* Number of characters moved is returned here */
{
	int n;

	if (np)
		*np = 0;
	switch (n = upath(from, to, max)) {
	case -2:
		return ENAMETOOLONG;
	case -1:
		return EFAULT;
	default:
		if (np)
			*np = n + 1;	/* Include null byte */
		return 0;
	}
	/* NOTREACHED */
}

int
copystr(from, to, max, np)
	char *from;	/* Source address (system space) */
	char *to;	/* Destination address */
	size_t max;	/* Maximum number of characters to move */
	size_t *np;	/* Number of characters moved is returned here */
{
	int n;

	*np = 0;
	switch (n = spath(from, to, max)) {
	case -2:
		return ENAMETOOLONG;
	case -1:
		return EFAULT;
	default:
		*np = n + 1;	/* Include null byte */
		return 0;
	}
	/* NOTREACHED */
}

/*
 * Move "n" bytes at byte location "cp" to or from (flag) a user
 * or kernel (u.segflg) area.  The I/O parameters are in u.u_base,
 * u.u_count, and u.u_offset, and all are updated to reflect the
 * number of bytes moved.
 *
 * This routine has been replaced by uiomove() and is retained here
 * only for backward compatibility with old device drivers.
 */
void
iomove(cp, n, flag)
	register caddr_t cp;
	register n;
	int flag;
{
	register t;

	if (n == 0)
		return;
	if (u.u_segflg != 1)  {
		if (flag == B_WRITE)
			t = copyin(u.u_base, (caddr_t)cp, n);
		else
			t = copyout((caddr_t)cp, u.u_base, n);
		if (t) {
			u.u_error = EFAULT;	/* XXX */
			return;
		}
	} else
		if (flag == B_WRITE)
			bcopy(u.u_base, (caddr_t)cp, n);
		else
			bcopy((caddr_t)cp, u.u_base, n);
	u.u_base += n;
	u.u_offset += n;
	u.u_count -= n;
}

/*
 * cpass and passc (below) are also retained only for backward
 * compatibility with old device drivers.
 */

/*
 * Pass back  c  to the user at his location u_base;
 * update u_base, u_count, and u_offset.  Return -1
 * on the last character of the user's read.
 * u_base is in the user data space.
 */
passc(c)
	register c;
{
	if (subyte(u.u_base, c) < 0) {
		u.u_error = EFAULT;	/* XXX */
		return -1;
	}
	u.u_count--;
	u.u_offset++;
	u.u_base++;
	return (u.u_count == 0) ? -1: 0;
}

/*
 * Pick up and return the next character from the user's
 * write call at location u_base;
 * update u_base, u_count, and u_offset.  Return -1
 * when u_count is exhausted.
 * u_base is in the user data space.
 */
cpass()
{
	register c;

	if (u.u_count == 0)
		return -1;
	if ((c = fubyte(u.u_base)) < 0) {
		u.u_error = EFAULT;
		return -1;
	}
	u.u_count--;
	u.u_offset++;
	u.u_base++;
	return c;
}
