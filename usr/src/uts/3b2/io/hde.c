/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/hde.c	1.14"
/* This file contains the Hard Disk Error Log Driver
 * that is part of the Bad Block Handling Feature.
 *
 * This driver provides the hdeeqd() and hdelog() subroutines
 * which hard disk drivers call.
 *
 * It also provides open(), close(), and ioctl() system calls.
 * These calls implement the special operations that are required
 * by the utility commands that are part of bad block handling.
 * As such, they are considered an internal interface of the
 * feature and are intended to be used only by those commands.
 */

#include "sys/param.h"
#include "sys/types.h"
#include "sys/debug.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/sysmacros.h"
#include "sys/systm.h"
#include "sys/signal.h"
#include "sys/sbd.h"
#include "sys/immu.h"
#include "sys/user.h"
#include "sys/file.h"
#include "sys/conf.h"
#include "sys/errno.h"
#include "sys/proc.h"
#include "sys/vtoc.h"
#include "sys/hdelog.h"
#include "sys/hdeioctl.h"
#include "sys/cmn_err.h"
#include "sys/open.h"

/* configuration dependent variables defined in master.d/hdelog */
extern int hdeedct;		/* count of slots available in hdeeqdt table */
extern struct hdeedd hdeeqdt[];

int hdeeduc;			/* count of slots used in hdeeqdt table */

/* temporary error report queue */
#define hdeqincr(ndx)	((ndx+1) % HDEQSIZE)
STATIC struct hdedata hdeerq[HDEQSIZE];
STATIC int hdeqondx = 0;	/* index of oldest report */
STATIC int hdeqnndx = 0;	/* index of newest report + 1 */
/* queue empty when hdeqnndx == hdeqondx */
/* queue full when hdeqincr(hdeqnndx) == hdeqondx */
/* this wastes one slot, but so what */

/* hdefix lockout control stuff */
STATIC int hdefixl;		/* hdefix lock */
STATIC int hdefixw;		/* hdefix is waiting */
STATIC int hdefixp;		/* hdefix process ID */

/* Log in progress (LIP) control stuff */
STATIC int hdeerwt;		/* wait for error reports */
STATIC int hdelipid;		/* LIP process ID */
STATIC int hdelipct;		/* count of reports in progress */

/* flags for hdeflgs (in struct hdeedd) */
#define HDEWANTD	0x0001

#define HDEOPRI	(PZERO+1)	/* sleep priority */

STATIC int hdeotyp[OTYPCNT];
STATIC int hdeosum;
STATIC int hdeclinp;
#define MAJORMASK	0x8000

STATIC void	hdeermsg();
STATIC void	hdeunfix();
STATIC int	hdeqcnt();
STATIC int	hdecmp();

STATIC o_dev_t
hdeddev(ddev)
o_dev_t ddev;
{
	/* The purpose of this function is to get rid of the */
	/* extra bit used in the SV file system macros. */
	/* Since the 3B2 assumes a maximum of 128 major */
	/* devices, only the least significant 7 bits */
	/* of the major have to be maintained */
	ddev &= ~MAJORMASK;
	return ddev;
}

int
#ifdef __STDC__
hdeeqd(o_dev_t ddev, daddr_t pdsno, short edtyp)
#else
hdeeqd(ddev, pdsno, edtyp)
o_dev_t ddev;
daddr_t pdsno;
short edtyp;
#endif
{
	if (major(ddev) >= cdevcnt) {
		cmn_err (CE_WARN, "hdeeqd: major(ddev) = %d (>=cdevcnt)\n",
			major(ddev));
		return 0;
	}
	ddev = hdeddev(ddev);
	if (hdeeduc >= hdeedct) {
		cmn_err (CE_WARN, "too few HDE equipped disk slots\n");
		cmn_err (CE_CONT, "Bad block handling skipped for maj/min = %d/%d\n",
			emajor(ddev), eminor(ddev));
		return 0;
	}
	hdeeqdt[hdeeduc].hdepdsno = pdsno;
	hdeeqdt[hdeeduc].hdetype = edtyp;
	hdeeqdt[hdeeduc++].hdeedev = ddev;
	return 0;
}

int
hdelog(eptr)
struct hdedata *eptr;
{
	eptr->diskdev = hdeddev(eptr->diskdev);
	if (hdeqincr(hdeqnndx) == hdeqondx) {
		cmn_err (CE_WARN, "HDE queue full, following report not logged\n");
		hdeermsg(eptr);
		return 0;
	}
	hdeermsg(eptr);
	hdeerq[hdeqnndx] = *eptr;
	hdeqnndx = hdeqincr(hdeqnndx);
	if (hdeerwt && !hdefixl) {
		hdeerwt = 0;
		wakeup((caddr_t)&hdeerwt);
	}
	return 0;
}

STATIC void
hdeermsg(esp)
struct hdedata *esp;
{
	char serno[13];
	register int i;

	for (i = 0; i < 12; i++) serno[i] = esp->dskserno[i];
	serno[12] = 0;
	cmn_err (CE_WARN, "%s %s hard disk error: maj/min = %d/%d",
		((esp->severity == HDEMARG) ? "marginal" : "unreadable"),
		((esp->readtype == HDECRC) ? "CRC" : "ECC"),
		emajor(esp->diskdev), eminor(esp->diskdev));
	if (serno[0])
		cmn_err(CE_CONT, ", serial # = %s", serno);
	cmn_err (CE_CONT, "\n\t block # = %d", esp->blkaddr);
	if (esp->severity == HDEMARG) cmn_err (CE_CONT, ", bad try count = %d",
		esp->badrtcnt);
	cmn_err (CE_CONT, "\n");
	if (esp->severity == HDEMARG && esp->readtype == HDEECC)
		cmn_err (CE_CONT, "bit width of corrected error = %d\n",
			esp->bitwidth);
	else esp->bitwidth = 0;
}

/*ARGSUSED*/
void
hdeopen(dev, flag, otyp)
o_dev_t dev;
int flag, otyp;
{
	if ((u.u_error = drv_priv(u.u_cred)) != 0)
		return;
	if (otyp == OTYP_LYR)
		hdeotyp[OTYP_LYR]++;
	else if (otyp >= 0 && otyp < OTYPCNT)
		hdeotyp[otyp] = 1;

	hdeosum = 1;
	while (hdeclinp)
		sleep((caddr_t)&hdeclinp, HDEOPRI);
}

/*ARGSUSED*/
void
hdeclose(dev, flag, otyp)
o_dev_t dev;
int flag, otyp;
{
	register int i;
	register struct hdeedd *edp;
	register int osum;

	if (otyp == OTYP_LYR)
		hdeotyp[OTYP_LYR]--;
	else if (otyp >= 0 && otyp < OTYPCNT)
		hdeotyp[otyp] = 0;

	for (osum = 0, i = 0; i < OTYPCNT; i++)
		osum |= hdeotyp[i];
	if (hdeosum == osum)
		return;
	hdeclinp = u.u_procp->p_pid;
	for (edp = hdeeqdt, i = 0; i < hdeeduc; edp++, i++) {
		if (edp->hdepid) {
			if (*cdevsw[major(edp->hdeedev)].d_flag & D_OLD)
				(*cdevsw[major(edp->hdeedev)].d_close)
				    (edp->hdeedev, FREAD|FWRITE, OTYP_LYR);
			else
				u.u_error = (*cdevsw[major(edp->hdeedev)].d_close)
				    (expdev(edp->hdeedev), FREAD|FWRITE, OTYP_LYR,
				    u.u_cred);
			edp->hdepid = 0;
		}
		edp->hdeflgs &= ~HDEWANTD;
	}
	hdelipid = 0;
	hdelipct = 0;
	hdefixl = hdefixw = hdefixp = 0;
	hdeerwt = 0;
	hdeclinp = 0;
	wakeup((caddr_t)&hdeclinp);
}

void
hdeexit()
{
	register int i;
	register struct hdeedd *edp;
	register int mypid;
	register int fixwflg;

	fixwflg = 0;
	mypid = u.u_procp->p_pid;
	for (edp = hdeeqdt, i = 0; i < hdeeduc; edp++, i++) {
		if (edp->hdepid == mypid) {
			if (*cdevsw[major(edp->hdeedev)].d_flag & D_OLD)
				(*cdevsw[major(edp->hdeedev)].d_close)
				    (edp->hdeedev, FREAD|FWRITE, OTYP_LYR);
			else
				u.u_error = (*cdevsw[major(edp->hdeedev)].d_close)
				    (expdev(edp->hdeedev), FREAD|FWRITE, OTYP_LYR,
				    u.u_cred);
			edp->hdepid = 0;
			fixwflg = 1;
			if (edp->hdeflgs & HDEWANTD) {
				edp->hdeflgs &= ~HDEWANTD;
				wakeup((caddr_t)edp);
			}
		}
	}
	if (mypid == hdefixp)
		hdeunfix();
	else if (mypid == hdelipid) {
		hdelipid = 0;
		hdelipct = 0;
		fixwflg = 1;
	}
	if (fixwflg && hdefixw)
		wakeup((caddr_t)&hdefixw);
	if (hdeclinp == mypid) {
		hdeclinp = 0;
		wakeup((caddr_t)&hdeclinp);
	}
}

STATIC void
hdeunfix()
{
	register int i;
	register struct hdeedd *edp;

	hdefixl = hdefixw = hdefixp = 0;
	for (edp = hdeeqdt, i = 0; i < hdeeduc; edp++, i++) {
		if (edp->hdeflgs & HDEWANTD && edp->hdepid == 0) {
			edp->hdeflgs &= ~HDEWANTD;
			wakeup((caddr_t)edp);
		}
	}
	if (hdeerwt) {
		hdeerwt = 0;
		wakeup((caddr_t)&hdeerwt);
	}
}

STATIC int
hdediso()
{
	register int i;
	register struct hdeedd *edp;
	register int my_pid;

	my_pid = u.u_procp->p_pid;
	for (edp = hdeeqdt, i = 0; i < hdeeduc; edp++, i++) {
		if (edp->hdepid && edp->hdepid != my_pid)
			return 1;
	}
	return 0;
}

STATIC int
hdefind(ddev)
register o_dev_t ddev;
{
	register int i;
	register struct hdeedd *edp;
	for (edp = hdeeqdt, i = 0; i < hdeeduc; edp++, i++) {
		if (edp->hdeedev == ddev)
				return i;
	}
	return -1;
}

STATIC int
hdehdio(iocmd, uap, kap)
int iocmd;
struct hdearg *uap, *kap;
{
	register caddr_t uadr;
	o_dev_t ddev;
	register int i;
	struct io_arg khdarg;
	int dummyrval;

	uadr = (caddr_t) &(uap->hdebody.hdargbuf);
	if ((i = hdefind(kap->hdebody.hdedskio.hdeddev))
		< 0) {
		kap->hderval = HDE_NODV;
		u.u_error = EINVAL;
		return -1;
	}
	if (hdeeqdt[i].hdepid != u.u_procp->p_pid) {
		kap->hderval = HDE_NOTO;
		u.u_error = EINVAL;
		return -1;
	}
	khdarg.retval = 0;
	khdarg.sectst = kap->hdebody.hdedskio.hdedaddr;
	khdarg.memaddr = (int) (kap->hdebody.hdedskio.hdeuaddr);
	khdarg.datasz = kap->hdebody.hdedskio.hdebcnt;
	if (u.u_segflg != 1) {
		if (copyout((caddr_t) &khdarg, uadr, sizeof(struct io_arg))) {
			u.u_error = EFAULT;
			return -1;
		}
	} else {
		bcopy((caddr_t) &khdarg, uadr, sizeof(struct io_arg));
	}
	ddev = kap->hdebody.hdedskio.hdeddev;
	if (*cdevsw[major(ddev)].d_flag & D_OLD)
		(*cdevsw[major(ddev)].d_ioctl)
		    (ddev, iocmd, uadr, FREAD|FWRITE);
	else
		u.u_error = (*cdevsw[major(ddev)].d_ioctl)
		    (expdev(ddev), iocmd, uadr, FREAD|FWRITE, u.u_cred, &dummyrval);
	if (u.u_error) {
		kap->hderval = HDE_IOE;
		return -1;
	}
	if (u.u_segflg != 1) {
		if (copyin(uadr, (caddr_t) &khdarg, sizeof(struct io_arg))) {
			u.u_error = EFAULT;
			return -1;
		}
	} else {
		bcopy(uadr, (caddr_t) &khdarg, sizeof(struct io_arg));
	}
	if (khdarg.retval) {
		kap->hderval = HDE_IOE;
		u.u_error = EIO;
		return -1;
	}
	return 0;
}

/*ARGSUSED*/
int
hdeioctl(dev, command, uarg, flag)
o_dev_t dev;
int command, flag;
struct hdearg *uarg;
{
	struct hdearg karg;
	struct hdedata kers;
	register int uadr, i;
	register int my_pid;
	register int j, k;

	if (u.u_segflg != 1) {
		if (copyin((caddr_t)uarg, (caddr_t)&karg, sizeof(struct hdearg))) {
			u.u_error = EFAULT;
			return 0;
		}
	} else {
		bcopy((caddr_t)uarg, (caddr_t)&karg, sizeof(struct hdearg));
	}
	karg.hderval = 0;
	karg.hdersize = 0;
	my_pid = u.u_procp->p_pid;
	switch (command)
	{
		case HDEGEDCT:
			karg.hdersize = hdeeduc;
			break;
		case HDEGEQDT:
			karg.hdersize = hdeeduc;
			if (karg.hdebody.hdegeqdt.hdeeqdct < hdeeduc) {
				karg.hderval = HDE_SZER;
				u.u_error = EINVAL;
				break;
			}
			uadr = (int) (karg.hdebody.hdegeqdt.hdeudadr);
			for (i = 0; i < hdeeduc; i++) {
				if (u.u_segflg != 1) {
					if (copyout((caddr_t)&hdeeqdt[i],
					(caddr_t)uadr, sizeof(struct hdeedd))) {
						u.u_error = EFAULT;
					}
				} else {
					bcopy((caddr_t)&hdeeqdt[i],
					(caddr_t)uadr, sizeof(struct hdeedd));
				}
				uadr += sizeof(struct hdeedd);
			}
			break;
		case HDEOPEN:
		    {
			major_t maj;

			if ((i = hdefind(karg.hdebody.hdedskio.hdeddev))
				< 0) {
				karg.hderval = HDE_NODV;
				u.u_error = EINVAL;
				break;
			}
			if (hdeeqdt[i].hdepid == my_pid)
				break;
			while (hdeeqdt[i].hdepid ||
			    (hdefixl && hdefixp != my_pid)) {
				hdeeqdt[i].hdeflgs |= HDEWANTD;
				sleep((caddr_t)&hdeeqdt[i], HDEOPRI);
			}
			/* LINTED implicit narrowing */
			hdeeqdt[i].hdepid = my_pid;
			maj = major(karg.hdebody.hdedskio.hdeddev);
			if (*cdevsw[maj].d_flag & D_OLD)
				(*cdevsw[maj].d_open)
				    (karg.hdebody.hdedskio.hdeddev,
				    FREAD|FWRITE, OTYP_LYR);
			else {
				dev_t tmpdev;
				/* new-style drive so expand dev */
				tmpdev = expdev(karg.hdebody.hdedskio.hdeddev);
				u.u_error = (*cdevsw[maj].d_open)
				    (&tmpdev,
				    FREAD|FWRITE, OTYP_LYR, u.u_cred);
				karg.hdebody.hdedskio.hdeddev = cmpdev(tmpdev);
			}
			if (u.u_error) {
				hdeeqdt[i].hdepid = 0;
				karg.hderval = HDE_BADO;
				break;
			}
			break;
		    }
		case HDEGETSS:
			hdehdio(V_GETSSZ, uarg, &karg);
			break;
		case HDERDPD:
			hdehdio(V_PDREAD, uarg, &karg);
			break;
		case HDEWRTPD:
			hdehdio(V_PDWRITE, uarg, &karg);
			break;
		case HDERDISK:
			hdehdio(V_PREAD, uarg, &karg);
			break;
		case HDEWDISK:
			hdehdio(V_PWRITE, uarg, &karg);
			break;
		case HDECLOSE:
		    {
			major_t maj;

			if ((i = hdefind(karg.hdebody.hdedskio.hdeddev))
				< 0) {
				karg.hderval = HDE_NODV;
				u.u_error = EINVAL;
				break;
			}
			if (hdeeqdt[i].hdepid != my_pid) {
				karg.hderval = HDE_NOTO;
				u.u_error = EINVAL;
				break;
			}
			maj = major(karg.hdebody.hdedskio.hdeddev);
			if (*cdevsw[maj].d_flag & D_OLD)
				(*cdevsw[maj].d_close)
				    (karg.hdebody.hdedskio.hdeddev,
				    FREAD|FWRITE, OTYP_LYR);
			else
				u.u_error = (*cdevsw[maj].d_close)
				    (expdev(karg.hdebody.hdedskio.hdeddev),
				    FREAD|FWRITE, OTYP_LYR, u.u_cred);
			hdeeqdt[i].hdepid = 0;
			if (hdefixw)
				wakeup((caddr_t)&hdefixw);
			if (hdeeqdt[i].hdeflgs & HDEWANTD) {
				hdeeqdt[i].hdeflgs &= ~HDEWANTD;
				wakeup((caddr_t)&hdeeqdt[i]);
			}
			break;
		    }
		case HDEMLOGR:
			uadr = (int)karg.hdebody.hdeelogr.hdeuladr;
			for (i = karg.hdebody.hdeelogr.hdelrcnt; i > 0; --i) {
				if (u.u_segflg != 1) {
					if (copyin((caddr_t)uadr,
					  (caddr_t)&kers,
					  sizeof(struct hdedata))) {
						u.u_error = EFAULT;
					}
				} else {
					bcopy((caddr_t)uadr, (caddr_t)&kers,
					  sizeof(struct hdedata));
				}
				if (u.u_error) {
					karg.hdersize =
					  karg.hdebody.hdeelogr.hdelrcnt - i;
					goto endofsw;
				}
				hdelog(&kers);
			}
			karg.hdersize = karg.hdebody.hdeelogr.hdelrcnt;
			break;
		case HDEGERCT:
			while (hdefixl && my_pid != hdefixp) {
				hdeerwt = 1;
				sleep ((caddr_t)&hdeerwt, HDEOPRI);
			}
			karg.hdersize = hdeqcnt();
			break;
		case HDEERSLP:
			while ((hdefixl && my_pid != hdefixp)
				|| hdeqcnt() == 0) {
				hdeerwt = 1;
				sleep ((caddr_t)&hdeerwt, HDEOPRI);
			}
			karg.hdersize = hdeqcnt();
			break;
		case HDEGEREP:
			while (hdefixl && my_pid != hdefixp) {
				hdeerwt = 1;
				sleep ((caddr_t)&hdeerwt, HDEOPRI);
			}
			if (hdeqcnt() == 0) break;
			if (my_pid != hdefixp) {
				if (u.u_procp->p_ppid != 1) {
					/* only demon or hdefix can do it */
					karg.hderval = HDE_NOTD;
					u.u_error = EINVAL;
					break;
				}
				if (hdelipct && my_pid != hdelipid) {
					karg.hderval = HDE_TWOD;
					u.u_error = EINVAL;
					break;
				}
			}
			i = min(hdeqcnt(), karg.hdebody.hdeelogr.hdelrcnt);
			karg.hdersize = i;
			uadr = (int)karg.hdebody.hdeelogr.hdeuladr;
			for (j = hdeqondx, k = 0; k < i; j = hdeqincr(j), k++) {
				if (u.u_segflg != 1) {
					if (copyout((caddr_t)&hdeerq[j],
					  (caddr_t)uadr,
					  sizeof(struct hdedata))) {
						u.u_error = EFAULT;
					}
				} else {
					bcopy((caddr_t)&hdeerq[j],
					  (caddr_t)uadr,
					  sizeof(struct hdedata));
				}
				if (u.u_error) {
					karg.hdersize = 0;
					goto endofsw;
				}
				uadr += sizeof(struct hdedata);
			}
			if (my_pid != hdefixp) {
				hdelipid = u.u_procp->p_pid;
				if (i > hdelipct) hdelipct = i;
			}
			break;
		case HDECEREP:
			if (my_pid != hdefixp && my_pid != hdelipid) {
				karg.hderval = HDE_CANT;
				u.u_error = EINVAL;
				break;
			}
			i = karg.hdebody.hdeelogr.hdelrcnt;
			uadr = (int) (karg.hdebody.hdeelogr.hdeuladr);
			while (i-- > 0) {
				if (u.u_segflg != 1) {
					if (copyin((caddr_t) uadr,
					  (caddr_t) &kers,
					  sizeof(struct hdedata))) {
						u.u_error = EFAULT;
					}
				} else {
					bcopy((caddr_t) uadr,
					  (caddr_t) &kers,
					  sizeof(struct hdedata));
				}
				if (u.u_error) {
					goto endofsw;
				}
				for (j = hdeqondx; j != hdeqnndx; j = hdeqincr(j)) {
					if (hdecmp((caddr_t) &kers,
						(caddr_t) &hdeerq[j],
						sizeof(struct hdedata)))
							continue;
					karg.hdersize++;
					for (k = j; k != hdeqondx;) {
						if (--k < 0) k = HDEQSIZE-1;
						hdeerq[j] = hdeerq[k];
						j = k;
					}
					hdeqondx = hdeqincr(hdeqondx);
					if (my_pid == hdelipid &&
						--hdelipct <= 0) {
						hdelipct = 0;
						hdelipid = 0;
						if (hdeerwt) {
							hdeerwt = 0;
							wakeup((caddr_t)&hdeerwt);
						}
						if (hdefixw)
							wakeup((caddr_t)&hdefixw);
					}
					goto gotit;
				}
				karg.hderval = HDE_BADR;
				u.u_error = EINVAL;
				goto endofsw;
gotit:
				uadr += sizeof(struct hdedata);
			}
			break;
		case HDEFIXLK:
			if (hdefixl || hdefixw) {
				karg.hderval = HDE_TWOF;
				u.u_error = EINVAL;
				break;
			}
			hdefixw = 1;
			while (hdelipct || hdediso()) {
				sleep ((caddr_t)&hdefixw, HDEOPRI);
			}
			hdefixl = 1;
			hdefixp = my_pid;
			hdefixw = 0;
			break;
		case HDEFIXUL:
			if (my_pid != hdefixp) {
				karg.hderval = HDE_NOTF;
				u.u_error = EINVAL;
				break;
			}
			hdeunfix();
			break;
		default:
			karg.hderval = HDE_UNKN;
			u.u_error = EINVAL;
			break;
	}
endofsw:
	if (!u.u_error) {
		if (u.u_segflg != 1) {
			if (copyout((caddr_t)&karg,
			  (caddr_t)uarg, sizeof(struct hdearg))) {
				u.u_error = EFAULT;
				return 0;
			}
		} else {
			bcopy((caddr_t)&karg,
			  (caddr_t)uarg, sizeof (struct hdearg));
		}
		return 1;
	} else {
		return 0;
	}
}

STATIC int
hdeqcnt()
{
	register int i;

	i = hdeqnndx - hdeqondx;
	if (i < 0) i += HDEQSIZE;
	return i;
}

STATIC int
hdecmp(cp1,cp2,sz)
register char *cp1, *cp2;
register int sz;
{
	while (sz--)
		if (*cp1++ != *cp2++)
			return 1;
	return 0;
}
