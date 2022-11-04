/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/sys3b.c	1.33.1.10"
#include "sys/param.h"
#include "sys/types.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/signal.h"
#include "sys/sbd.h"
#include "sys/iu.h"
#include "sys/immu.h"
#include "sys/proc.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/time.h"
#include "sys/systm.h"
#include "sys/edt.h"
#include "sys/firmware.h"
#include "sys/sys3b.h"
#include "sys/todc.h"
#include "sys/nvram.h"
#include "sys/utsname.h"
#include "sys/sysmacros.h"
#include "sys/boothdr.h"
#include "sys/uadmin.h"
#include "sys/map.h"
#include "sys/vfs.h"
#include "sys/cred.h"
#include "sys/vnode.h"
#include "sys/swap.h"
#include "sys/gate.h"
#include "sys/var.h"
#include "sys/tuneable.h"
#include "sys/debug.h"
#include "sys/cmn_err.h"
#include "sys/inline.h"
#include "sys/buf.h"
#include "sys/conf.h"
#include "sys/fstyp.h"
#include "sys/uio.h"
#include "sys/mman.h"
#include "sys/vmsystm.h"
#include "vm/seg.h"
#include "vm/vm_hat.h"
#include "vm/as.h"

#if defined(__STDC__)
STATIC void greenled(int);
STATIC int chksrl(int);
STATIC int userdma(caddr_t, size_t, int);
STATIC void undma(caddr_t, size_t, int);
STATIC int readsrl(void);
#else
STATIC void greenled();
STATIC int chksrl();
STATIC int userdma();
STATIC void undma();
STATIC int readsrl();
#endif

extern	time_t	time;	/* Defined in os/clock.c */

char u400;

#ifdef KPERF
#include "sys/file.h"
#include "sys/disp.h"

int kpftraceflg;
int kpchildslp;
#endif /* KPERF */

#define	FWREL(ptr)	((_VOID *)((char *) (ptr) + VROM))

#define VMEMINIT	(*(((struct vectors *)VBASE)->p_meminit))
#define VMEMSIZE	(*(((struct vectors *)VBASE)->p_memsize))
#define VSERNO		((struct serno *) \
			FWREL(((struct vectors *)VBASE)->p_serno))

#define	VCHKNVRAM	((char (*)()) \
			FWREL(((struct vectors *)VBASE)->p_chknvram))
#define	VRNVRAM		((char (*)()) \
			FWREL(((struct vectors *) VBASE)->p_rnvram))
#define	VWNVRAM		((char (*)()) \
			FWREL(((struct vectors *) VBASE)->p_wnvram))
#define	VDEMON		((char (*)()) \
			FWREL(((struct vectors *) VBASE)->p_demon))
 
/* pointer to equipped device table */

#define VP_EDT		(((struct vectors *)VBASE)->p_edt)
#define	V_EDTP(x)	(VP_EDT + x)

/* pointer to number of edt entries */

#define VNUM_EDT	(*(((struct vectors *)VBASE)->p_num_edt))

/*
 * The following are the PC and PSW which DEMON set into
 * the second level gate table entry for a bpt interrupt
 * when you set a breakpoint.  These defines are temporary.
 * They will eventually be put into the vector table.
 */

extern struct s3bsym  sys3bsym;		/* generated symbol table	 */
extern struct s3bconf sys3bconfig;	/* generated configuration table */
extern struct s3bboot sys3bboot;	/* generated boot program name	 */
					/* and timestamp		 */

psw_t	gatepsw = GPSW;

/*
 * The following is the control character which will allow
 * us to enter DEMON.  See code in iu.c.
 */

int Demon_cc = 0x10;	/* Cntrl-p is the default.	*/

/* The following is for floating point. */

extern int mau_present;			/* mau equipped? */

/* driver entry points to call when system is shutting down */
extern int (*io_halt[])();

/* The page table for the gate table. */

extern int	*gateptbl0;

/*
 * 3B System Call.
 */

int sys3bautoconfig = 0;	 	/* auto-config flag: 0-no 1-yes */

struct sys3ba {
	int	cmd;
	int	arg1;
	int	arg2;
	int	arg3;
};

int
sys3b(uap, rvp)
	register struct sys3ba *uap;
	rval_t *rvp;
{
	register int	idx;
	register int	c;
	int		error = 0;

	struct nvparams	nvpx;
	struct todc	clkx;
	char		sysnamex[sizeof(utsname.nodename)];
	extern u_int	timer_resolution;

	switch (uap->cmd) {
	case GRNON:	/* turn green light to a solid on state */
		if (suser(u.u_cred))
			greenled(0);
		else
			error = EPERM;
		break;

	case GRNFLASH:	/* starts green light flashing */
		if (suser(u.u_cred))
			greenled((HZ * 27) / 100);
		else
			error = EPERM;
		break;


	case S3BNVPRT:	/* print an xtra_nvr structure */
		if (!suser(u.u_cred)) {
			error = EPERM;
			break;
		}
		if (userdma((caddr_t)(uap->arg1), 
		  sizeof(struct xtra_nvr), B_WRITE) == 0)
			error = EFAULT;
		else {
			dumpnvram((struct xtra_nvr *)uap->arg1) ;
			undma((caddr_t)(uap->arg1), 
			  sizeof(struct xtra_nvr), B_WRITE);
		}
		break;

	/*
 	 * Copy a string from a given kernel address to a user buffer. Uses
	 * copyout() for each byte to trap bad kernel addresses while watching
	 * for the null terminator.
	 */
	case S3BKSTR:
	{
		register char	*src;
		register char	*dst;
		register char	*dstlim;
		extern int sdata, dataSIZE[];

		if (!suser(u.u_cred)) {
			error = EPERM;
			break;
		}
		src = (char *) uap->arg1;
		if (src < (char *)sdata) {
			error =  EINVAL;
			break;
		}
		dstlim = (dst = (char *) uap->arg2) + (unsigned int) uap->arg3;
		do {
			if (src >= (char *)((unsigned int)sdata + (unsigned int)dataSIZE)
			  || dst == dstlim 
			  || copyout(src, dst++, 1) != 0) {
				error = EINVAL;
				break;
			}
		} while (*src++);

		break;
	}

	/*
	 * Acquire generated system symbol table or config table:
	 *
	 * sys3b(S3BSYM, &buffer, sizeof(buffer))
	 * sys3b(S3BCONF, &buffer, sizeof(buffer))
	 */
	case S3BSYM:
	case S3BCONF:
	{
                register int *buffer = (int*)(uap->arg1);
                register int size = uap->arg2;
		register int *dataddr;
		register int datalen;

		switch (uap->cmd) {
			case S3BSYM:
			    dataddr = (int*)&sys3bsym;
			    datalen = sys3bsym.size-sizeof(sys3bsym.size);
			    break;
			case S3BCONF:
		 	    dataddr = (int*)&sys3bconfig;
			    datalen = sys3bconfig.count*sizeof(struct s3bc);
			    break;
		}

		if (size < sizeof(int)) {
			/* Buffer too small even for size. */
			error = EINVAL;
			break;
		}

		if (suword(buffer,*dataddr) == -1) {
			/* Bad buffer address. */
			error = EFAULT;
			break;
		}

		if ((size -= sizeof(int)) <= 0)
			break;

		if (size > datalen)
			size = datalen;

		if (copyout((caddr_t)dataddr+1,(caddr_t)buffer+1,size) != 0)
			/* Bad buffer address. */
			error = EFAULT;

		break;
	}

	/*
	 * Acquire timestamp and path name of boot program:
	 *
	 * sys3b(S3BBOOT, &buffer)
	 */
	case S3BBOOT:
		if (copyout((caddr_t)&sys3bboot,(caddr_t)uap->arg1,
		  sizeof(struct s3bboot)) != 0)
			/* bad buffer address */
			error = EFAULT;
		break;

	/*
	 * Was an auto-config boot done?  Return 1 if yes, 0 if no.
	 *
	 * sys3b(S3BAUTO)
	 */
	case S3BAUTO:
		if (!suser(u.u_cred)) {
			error = EPERM;
			break;
		}
		rvp->r_val1 = sys3bautoconfig;
		break;

	/*
	 * Undocumented fast illegal op-code handler interface for floating
	 * point simulation.
	 *
	 * sys3b(S3BIOP, handler)
	 */

	case S3BIOP:
	{
		register struct proc *p = u.u_procp;

		if (uap->arg1 == 0)
			u.u_iop = NULL;

		/*
		 * Not supported while tracing to
		 * preserve the sanity of sdb(1).
		 */

		else if (p->p_flag & STRC)
			error = EINVAL;

		/* 
		 * bad handler address 
		 */

		else if (as_checkprot(p->p_as, uap->arg1, 1, PROT_EXEC) != 0)
			error = EFAULT;

		else
			u.u_iop = (int*)uap->arg1;
		break;
	}

	case RTODC:	/* read TODC */
		if (!suser(u.u_cred)) {
			error = EPERM;
			break;
		}
		rtodc(&clkx);
		if (copyout((caddr_t) &clkx, (caddr_t) uap->arg1, sizeof(clkx)))
			error = EFAULT;
		break;

	case STIME:	/* set internal time, not hardware clock */
		if (!suser(u.u_cred)) {
			error = EPERM;
			break;
		}
		time = hrestime.tv_sec = (time_t) uap->arg1;
		hrestime.tv_nsec = 0;
		break;
 
	case SETNAME:	/* rename the system */
		if (!suser(u.u_cred)) {
			error = EPERM;
			break;
		}
		for (idx = 0;
		  (c = fubyte((caddr_t) uap->arg1 + idx)) > 0
		    && idx < sizeof(sysnamex) - 1;
		  ++idx)
			sysnamex[idx] = (char)c;
		if (c) {
			error = c < 0 ? EFAULT : EINVAL;
			break;
		}
		sysnamex[idx] = '\0';
		strncpy(utsname.nodename, sysnamex, SYS_NMLN-1);
		utsname.nodename[SYS_NMLN-1] = '\0';
		break;
 
	case WNVR:	/* write NVRAM */
		if (!suser(u.u_cred)) {
			error = EPERM;
			break;
		}
		if (copyin((caddr_t) uap->arg1, (caddr_t) &nvpx, sizeof(nvpx)))
			error = EFAULT;
		else if (nvpx.addr < (char *)ONVRAM
		    || nvpx.addr > (char *)ONVRAM + NVRSIZ
		    || nvpx.addr + nvpx.cnt < (char *)ONVRAM
		    || nvpx.addr + nvpx.cnt > (char *)ONVRAM + NVRSIZ)
			error = EINVAL;
		else if (userdma(nvpx.data, nvpx.cnt, B_WRITE) == 0)
			error = EFAULT;
		else {
			wnvram(nvpx.data, nvpx.addr, nvpx.cnt);
			undma(nvpx.data, nvpx.cnt, B_WRITE);
		}
		break;


	case RNVR:	/* read NVRAM */
		if (!suser(u.u_cred)) {
			error = EPERM;
			break;
		}
		if (copyin((caddr_t) uap->arg1, (caddr_t) &nvpx, sizeof(nvpx)))
			error = EFAULT;
		else if (nvpx.addr < (char *)ONVRAM
		  || nvpx.addr > (char *)ONVRAM + NVRSIZ
		  || nvpx.addr + nvpx.cnt < (char *)ONVRAM
		  || nvpx.addr + nvpx.cnt > (char *)ONVRAM + NVRSIZ)
			error = EINVAL;
		else if (userdma(nvpx.data, nvpx.cnt, B_READ) == 0)
			error = EFAULT;
		else {
			rnvram(nvpx.addr, nvpx.data, nvpx.cnt);
			undma(nvpx.data, nvpx.cnt, B_READ);
		}
		break;

	/*
	 * Double-map data segment (feature required for basic(1)).
	 *
	 * sys3b(S3BDMM, flag)
	 */
	case S3BDMM:
		/* 
		 * Now that we only support the 32100 MMU this is a nop
		 * Simply return the address of the data segment.
		 */
		if (uap->arg1) {
			/*
			 * Non-zero flag: enable the double-mapping.
			 */

			if (!u.u_dmm)
				u.u_dmm = 1;
			rvp->r_val1 = (int)u.u_procp->p_brkbase & ~((NBPS*4) - 1);
		} else {
			/*
			 * Zero flag: disable the double mapping.
			 */

			if (u.u_dmm) {
				u.u_dmm = 0;
			}
		}
		break;

	case CHKSER:	/* Check soft serial number */
		if (!suser(u.u_cred)) {
			error = EPERM;
			break;
		}
		if (chksrl(uap->arg1) == FAIL)
			error = EINVAL;
		break;


	/*
	 * Read the EDT (Equipped Device Table):
	 *
	 * sys3b(S3BEDT, buffer, size_bytes);
	 */
	case S3BEDT:
	{
		register int i, size, sub_num, edt_size, *buffer;
		struct edt *edtptr;
		int edt_num;

		buffer = (int *)uap->arg1;
		size = uap->arg2;

		sub_num = 0;
		for (i = 0; i < VNUM_EDT; i++) {
			edtptr = V_EDTP(i);
			sub_num = sub_num + edtptr->n_subdev;
		}
		edt_num = (VNUM_EDT << 16 | sub_num);
		if (size < sizeof(int)) {
			error = EFAULT;
			break;
		}
		if (copyout((caddr_t)&edt_num, (caddr_t)buffer, 
		  sizeof(int)) != 0) {
			error = EFAULT;
			break;
		}
		if (size == sizeof(int))
			break;
		else {
			buffer++;
			size -= sizeof(int);
			edt_size = VNUM_EDT * sizeof(struct edt) +
			  sub_num * sizeof(struct subdevice);
			if (size < edt_size) {
				error = EFAULT;
				break;
			}
			if (copyout((caddr_t)VP_EDT, (caddr_t)buffer, 
			  edt_size) != 0) {
				error = EFAULT;
				break;
			}
		}
		break;
	}

	/*
	 * Return the size of memory.
	 */
	case S3BMEM:
	{
		rvp->r_val1 = VMEMSIZE;
		break;
	}

	/*
	 * This function is just here for compatibility.
	 * It is really just a part of S3BSWPI.  This
	 * new function should be used for all new
	 * applications.
	 *
	 * sys3b(S3BSWAP, path, swplo, nblocks);
	 */

	case S3BSWAP:
	{
		swpi_t	swpbuf;

		swpbuf.si_cmd   = SI_ADD;
		swpbuf.si_buf   = (char *)uap->arg1;
		swpbuf.si_swplo = uap->arg2;
		swpbuf.si_nblks = uap->arg3;

		error = swapfunc(&swpbuf);
		break;
	}

	/*
	 * General interface for adding, deleting, or
	 * finding out about swap files.  See swap.h
	 * for a description of the argument.
	 *
	 * sys3b(S3BSWPI, arg_ptr);
	 */

	case S3BSWPI:
	{
		swpi_t	swpbuf;

		if (copyin((caddr_t)uap->arg1, (caddr_t)&swpbuf, 
		  sizeof(swpi_t)) < 0)
			error = EFAULT;
		else
			error = swapfunc(&swpbuf);
		break;
	}

	/*
	 * Transfer to firmware for debugging.  Breakpoints
	 * may be set there and then we can return to run a
	 * test.
	 */

	case S3BTODEMON:
		if (!suser(u.u_cred))
			error = EPERM;
		else if (!call_demon())
			error = EINVAL;
		break;

	/*
	 * Modify the control character which gets
	 * into or out of DEMON.
	 */

	case S3BCCDEMON:
		if (!suser(u.u_cred))
			error = EPERM;
		else if (uap->arg1 == 0)
			rvp->r_val1 = Demon_cc;
		else
			Demon_cc = uap->arg2;
		break;

	/*
	 * Turn the cache on or off on a 32100.
	 */

	case S3BCACHE:
		if (!suser(u.u_cred))
			error = EPERM;
		else if (uap->arg1) {
			if (!is32b()) {
				error = EINVAL;
				break;
			}
			error = cache_on();
		} else {
			if (!is32b()) {
				error = EINVAL;
				break;
			}
			error = cache_off();
		}
		break;

	/*
	 * Delete memory from the available list.  Forces
	 * tight memory situation for load testing.
	 */

	case S3BDELMEM:
	{
		if (!suser(u.u_cred)) {
			error = EPERM;
			break;
		}

		error = page_deladd(0, uap->arg1, rvp);
		break;
	}

	/*
	 * Add back previously deleted memory.
	 */

	case S3BADDMEM:
	{

		if (!suser(u.u_cred)) {
			error = EPERM;
			break;
		}

		error = page_deladd(1, uap->arg1, rvp);
		break;
	}

	/*
	 * Tell the user whether we have a MAU or not.
	 */

	case S3BFPHW:
		/* 
		 * arg1 is an address and must be on a word boundary;
		 * no explicit check here because the hardware enforces
		 * it and suword() will fail on a non-word address.
		 */
		if (suword((int *)uap->arg1, mau_present) == -1)
			error = EFAULT;
		break;

	/*
	 * Read a user process u-block.
	 * XXX this interface should be moved someplace else.
	 */
	case RDUBLK:
	{
		register struct proc *p;
		caddr_t addr;
		int ocount;
		struct uio uio;
		struct iovec iov;

		if (uap->arg3 < 0) {
			error = EINVAL;
			break;
		}		
		if ((p = prfind(uap->arg1)) == NULL) {
			error = ESRCH;
			break;
		}
		if (p->p_stat == 0 || p->p_stat == SIDL
		  || p->p_stat == SZOMB) {
			error = EINVAL;
			break;
		}
		ocount = min(uap->arg3, ctob(USIZE));
		iov.iov_base = (caddr_t) uap->arg2;
		iov.iov_len = uio.uio_resid = ocount;
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;
		uio.uio_offset = 0;
		uio.uio_fmode = 0;
		uio.uio_segflg = UIO_USERSPACE;

		addr = (caddr_t)KUSER(p->p_segu);
		error = uiomove(addr, uio.uio_resid, UIO_READ, &uio);
		rvp->r_val1 = ocount - uio.uio_resid;
		break;
	}

	/*
	 * Turn floating-point overflow catching on/off.
	 */
	case S3BFPOVR:
		switch (uap->arg1) {
		case FP_ENABLE:
		case FP_DISABLE:
			rvp->r_val1 = u.u_fpovr;
			u.u_pcb.psw.OE = u.u_fpovr = uap->arg1;
			break;
		default:
			error = EINVAL;
			break;
		}
		break;

	case S3BGETCLKRT:       /* Get clock rate */
		rvp->r_val1 = timer_resolution;
		break;

	case S3BTRAPLOCORE:	/* Control locore protection */
		rvp->r_val1 = s3btlc_state;
		switch (uap->arg1) {
		case S3BTLC_STATUS:
			break;
		case S3BTLC_DISABLE:
		case S3BTLC_SIGNAL:
		case S3BTLC_PRINT:
			if (!suser(u.u_cred))
				error = EPERM;
			else {
				sde_t	*sdeptr;
				s3btlc_state = uap->arg1;
				sdeptr = (sde_t *)srama[0];
				sdeptr->seg_prot = (sdeptr->seg_prot & KACCESS) |
					((uap->arg1 == S3BTLC_DISABLE) ? URE : UNONE);
				flushmmu(0, 2);
			}
			break;
		default:
			error = EINVAL;
			break;
		}
		break;

#ifdef	KPERF
	/* synchronization between parent and child  
	*/
	case KPFCHILDSLP:
		if (!suser(u.u_cred))
			break;
		if (kpchildslp == 0)
			sleep((caddr_t) &kpchildslp, PPIPE);
		break;

	/*	Turn the kernel performance measurement code on 
	*/

	case KPFTRON:
		if (!suser(u.u_cred))
			break;

		takephase = 0;
		putphase = 0;
		numrccount = 0;
		outbuf = 0;
		pre_trace = 1;
		kpftraceflg = 0;
		kpchildslp = 0;
		/* DEBUG */
		break;

	/*	Wait for a buffer of kernel perf statistics to fill, and return
	**	the data to the user.  First record is number of records
	**	to be returned (maximum is NUMRC).
	**	Usage: sys3b( KPFTRON2, &buffer, sizeof(buffer))
	**	the following logic is used:
	**	1. kpftraceflg on  A. takephase = putphase, sleep waiting for a 
	**			   buffer to fill
	**			B. takephase !=putphase, go copy records
	**			   and takephase = takephase +1 %NUMPHASE
	** 	always check for abnormal conditions
	**      2. kpftraceflg off A. takephase = putphase, copy numrccount
	**			   of records, ( buffer may not be full)
	**			B. takephase != putphase , go copy records
	**			   and takephase = takephase +1 % NUMPHASE
	*/

	case KPFTRON2: 
		{
                register int *buffer = (int*)(uap->arg1);
		int *dataddr;
                register int size = uap->arg2;


		if (!suser(u.u_cred))
			break;


		if ( size != (1+NUMRC)*sizeof(kernperf_t) ) {
			/* buffer too small even for size */
			cmn_err(CE_CONT, "sys3b, kpftron2, Buffer too small\n");
			u.u_error = EINVAL;
			break;
		}

		numrc = NUMRC;
		dataddr = (int*)&kpft[takephase*NUMRC];
		if (kpftraceflg == 1)  
			if (takephase == putphase )  {
				kpchildslp = 1;
				wakeup((caddr_t) &kpchildslp);
				sleep((caddr_t) &kpft[takephase*NUMRC],PPIPE);
			}
		 /* full buffer i.e. abnormal termination */
		if (outbuf == 1) {
			u.u_error = EINVAL;
			break;
		}
	
		/* tracing is off, here under normal conditions */
		if ((kpftraceflg  == 0 ) && (takephase == putphase))
			numrc = numrccount;
		copyrecords(dataddr,buffer,numrc);
		takephase = (takephase + 1) % NUMPHASE;
		break;
		}

	/*	Turn the kernel performance measurement off.
	*/

	case KPFTROFF: 
	{
		if (!suser(u.u_cred))
			break;

		asm(" MOVAW 0(%pc),Kpc ");
		kperf_write(KPT_END,Kpc,curproc);
		pre_trace = 0;
		kpftraceflg = 0;
		wakeup((caddr_t) &kpft[takephase*NUMRC]);
		break;
	}
#endif	/* KPERF */

	default:
		error = EINVAL;
		break;
	}

	return error;
}

/*
 * greenled()
 *
 * Flash the green "power" LED at a given rate. Specify
 * zero to turn the LED steadily on.
 */

STATIC void
greenled(rate)
	register int rate;
{
	register int	s;
	static int	isoff;
	static int	tout;

	s = splhi();
	if (isoff) {
		IDUART->scc_ropbc = PWRLED;
		isoff = 0;
	} else if (rate) {
		IDUART->scc_sopbc = PWRLED;
		isoff = 1;
	}
	if (tout) {
		untimeout(tout);
		tout = 0;
	}
	if (rate)
		tout = timeout(greenled, (caddr_t)rate, rate);
	splx(s);
}


/*
 * chnvram()
 *
 * Check non-volatile RAM using similar checksum technique as
 * with ROM. Calls the appropriate firmware function, optionally
 * saving a corrected checksum.
 */
chnvram(mode)
	register char	mode;
{
	return VCHKNVRAM(mode);
}

/*
 * rnvram()
 *
 * Read non-volatile RAM.  Calls the appropriate firmware routine.
 */
int
rnvram(src, dst, count)
	register caddr_t	src;
	register caddr_t	dst;
	register int count;
{
	return VRNVRAM(src, dst, count);
}

/*
 * wnvram()
 *
 * Write non-volatile RAM.  Calls the appropriate firmware routine.
 */
int
wnvram(src, dst, count)
	register caddr_t	src;
	register caddr_t	dst;
	register int count;
{
	return VWNVRAM(src, dst, count);
}


/*
 * chksrl()
 *
 * Check the passed suspected system serial number against
 * the actual serial number stored in PROM.  Return PASS or
 * FAIL accordingly.
 */

STATIC int
chksrl(passer)
	int passer;
{
	int promser;

	/* Compute unit's stored serial number. */

	promser = (VSERNO->serial3 +
		  (VSERNO->serial2 << 8) +
		  (VSERNO->serial1 << 16) +
		  (VSERNO->serial0 << 24));
	if (promser != passer)
		return FAIL;
	return PASS;
}

/*
 * Machine dependent code to reboot.
 */

/* ARGSUSED */
void
mdboot(fcn, mdep)
	int fcn;
	int mdep;
{
	void dhalt();

	dhalt();

	switch (fcn) {
	case AD_HALT:
		splhi();
		VRUNFLG = SAFE;
		((struct duart *)CONS)->ip_opcr = 0x00;
		/* Set power-off bit */
		((struct duart *)CONS)->scc_sopbc = KILLPWR;
		for (;;)
			continue;	
		/* NOTREACHED */

	case AD_BOOT:
		splhi();
		VRUNFLG = SAFE;
		rtnfirm();
		/* NOTREACHED */

	case AD_IBOOT:
		splhi();
		VRUNFLG = REENTRY;
		rtnfirm();
		/* NOTREACHED */
	}
}

/*
 * XENIX Compatibility Change:
 *   dhalt - Halt devices.
 *
 * Called late in system shutdown to allow devices to stop cleanly
 * AFTER interrupts are shut off.  Call the device driver entries
 * for halt, if there are any.  This table (io_halt) is created by
 * "cunix" for drivers that contain a "halt" routine.
 *
 * Interrupts may be turned on again so the drivers should make sure
 * no interrupt is pending from their peripheral.
 */
void
dhalt()
{
	register int (**haltptr)();	/* pointer to next halt entry point */

	for (haltptr = &io_halt[0];  *haltptr;  haltptr++)
		(**haltptr)();
}


int in_demon = 0;

int
call_demon()
{
	register sde_t		*sdeptr;
	int			oprot;

	if (VSERNO->serial0 <= 0xB || VSERNO->serial0 >= 0x20)
		return 0;
	if ((int)(VBASE->p_serno) != 0xfff0)
		return 0;
	sdeptr = (sde_t *)srama[0];

	oprot = sdeptr->seg_prot;
	sdeptr->seg_prot = KRWE | (oprot & UACCESS);

	/*
	 * We must do the following for as many pages as are
	 * occupied by the gate tables in ml/gate.c.
	 */

	gateptbl0[0] &= ~PG_W;
	gateptbl0[1] &= ~PG_W;
	flushmmu(0, 2);
	in_demon = 1;
	VDEMON();
	in_demon = 0;
	sdeptr->seg_prot = oprot;
	gateptbl0[0] |= PG_W;
	gateptbl0[1] |= PG_W;
	flushmmu(0, 2);
	return 1;
}

/*
 * The terms in the following expression correspond to the sizes
 * of the arrays "gates", "gatex", "gaten", and "gatefiller" in
 * the file ml/gate.c.
 */

#define	GATE_ENTRIES	138+16+1+211	/* # of second-level gate entries */

extern struct gate_l2 gates[];
extern struct kpcb *Xproc, *Ivect[], kpcb_pswtch;
extern struct kpcb kpcb_syscall;

int
cache_off()
{
	register struct kpcb	*ptr;
	register struct kpcb	**ptr2;
	register struct gate_l2	*ptr1;
	register proc_t			**pp;
	register user_t			*up;
	sde_t				*sdeptr;
	int				oprot;
	int				oldpri;

	sdeptr = (sde_t *)srama[0];
	oprot = sdeptr->seg_prot;
	sdeptr->seg_prot = KRWE | (oprot & UACCESS);
	gateptbl0[0] &= ~PG_W;
	gateptbl0[1] &= ~PG_W;
	flushmmu(0, 2);
	oldpri = splhi();

	for (ptr2 = &Xproc; ptr2 < &Ivect[256]; ptr2++) {
		ptr = *ptr2;
		ptr->psw.CSH_D = ptr->psw.CSH_F_D = 1;
		ptr->ipcb.psw.CSH_D = ptr->ipcb.psw.CSH_F_D = 1;
	}

	for (ptr1 = gates; ptr1 < &gates[GATE_ENTRIES]; ptr1++)
		ptr->psw.CSH_D = ptr->psw.CSH_F_D = 1;

	kpcb_pswtch.psw.CSH_D = kpcb_pswtch.psw.CSH_F_D = 1;
	kpcb_pswtch.ipcb.psw.CSH_D = kpcb_pswtch.ipcb.psw.CSH_F_D = 1;
	kpcb_syscall.psw.CSH_D = kpcb_syscall.psw.CSH_F_D = 1;
	kpcb_syscall.ipcb.psw.CSH_D = kpcb_syscall.ipcb.psw.CSH_F_D = 1;
	sendsig_psw.CSH_D = sendsig_psw.CSH_F_D = 1;
	p0init_psw.CSH_D  = p0init_psw.CSH_F_D  = 1;

	for (pp = nproc; pp < v.ve_proc; pp++) {
		if (*pp == NULL || (*pp)->p_stat == 0)
			continue;
		up = (user_t *)KUSER((*pp)->p_segu);
		ptr = (struct kpcb *)&up->u_ipcb;
		ptr->psw.CSH_D = ptr->psw.CSH_F_D = 1;
		ptr->ipcb.psw.CSH_D = ptr->ipcb.psw.CSH_F_D = 1;
		ptr = (struct kpcb *)&up->u_kpcb;
		ptr->psw.CSH_D = ptr->psw.CSH_F_D = 1;
		ptr->ipcb.psw.CSH_D = ptr->ipcb.psw.CSH_F_D = 1;
	}

	u400 = 0;

	asm("  ORW2  &0x02800000,%psw");
	asm("  NOP		     ");
	asm("  NOP		     ");

	splx(oldpri);
	sdeptr->seg_prot = oprot;
	gateptbl0[0] |= PG_W;
	gateptbl0[1] |= PG_W;
	flushmmu(0, 2);
	return 0;
}

cache_on()
{
	register struct kpcb	*ptr;
	register struct kpcb	**ptr2;
	register struct gate_l2	*ptr1;
	register proc_t			**pp;
	register user_t			*up;
	sde_t				*sdeptr;
	int				oprot;
	int				oldpri;

	sdeptr = (sde_t *)srama[0];
	oprot = sdeptr->seg_prot;
	sdeptr->seg_prot = KRWE | (oprot & UACCESS);
	gateptbl0[0] &= ~PG_W;
	gateptbl0[1] &= ~PG_W;
	flushmmu(0, 2);
	oldpri = splhi();

	for (ptr2 = &Xproc; ptr2 < &Ivect[256]; ptr2++) {
		ptr = *ptr2;
		ptr->psw.CSH_D = ptr->psw.CSH_F_D = 0;
		ptr->ipcb.psw.CSH_D = ptr->ipcb.psw.CSH_F_D = 0;
	}

	for (ptr1 = gates; ptr1 < &gates[GATE_ENTRIES]; ptr1++)
		ptr->psw.CSH_D = ptr->psw.CSH_F_D = 0;

	kpcb_pswtch.psw.CSH_D = kpcb_pswtch.psw.CSH_F_D = 0;
	kpcb_pswtch.ipcb.psw.CSH_D = kpcb_pswtch.ipcb.psw.CSH_F_D = 0;
	kpcb_syscall.psw.CSH_D = kpcb_syscall.psw.CSH_F_D = 0;
	kpcb_syscall.ipcb.psw.CSH_D = kpcb_syscall.ipcb.psw.CSH_F_D = 0;
	sendsig_psw.CSH_D = sendsig_psw.CSH_F_D = 0;
	p0init_psw.CSH_D  = p0init_psw.CSH_F_D  = 0;

	for (pp = nproc; pp < v.ve_proc; pp++) {
		if (*pp == NULL || (*pp)->p_stat == 0)
			continue;
		up = (user_t *)KUSER((*pp)->p_segu);
		ptr = (struct kpcb *)&up->u_ipcb;
		ptr->psw.CSH_D = ptr->psw.CSH_F_D = 0;
		ptr->ipcb.psw.CSH_D = ptr->ipcb.psw.CSH_F_D = 0;
		ptr = (struct kpcb *)&up->u_kpcb;
		ptr->psw.CSH_D = ptr->psw.CSH_F_D = 0;
		ptr->ipcb.psw.CSH_D = ptr->ipcb.psw.CSH_F_D = 0;
	}

	u400 = 1;

	asm("  CFLUSH		      ");
	asm("  ANDW2  &0xfd7fffff,%psw");
	asm("  NOP		      ");
	asm("  NOP		      ");

	splx(oldpri);
	sdeptr->seg_prot = oprot;
	gateptbl0[0] |= PG_W;
	gateptbl0[1] |= PG_W;
	flushmmu(0, 2);
	return 0;
}

/*
 *  Return the soft serial number.
 */
STATIC int
readsrl()
{
	register int	promser;

	promser = (VSERNO->serial3 +
		  (VSERNO->serial2 << 8) +
		  (VSERNO->serial1 << 16) +
		  (VSERNO->serial0 << 24));

	return promser;
}

/*
 * Decide whether this is a fast 32B.
 */
int
fast32b()
{
	register int	serial_number;

	/*
	 * Read serial number to determine whether a 400 or a 300.
	 * A 3 or 9 in nibble 5 is a 400.  Everything else is a 300.
	 */

	serial_number = (readsrl() >> 20) & 0xf;
	if (serial_number == 0x9 || serial_number == 0x3)
		return 1;
	return 0;
}

/* XXX - OBSOLETE ROUTINES MAINTAINED FOR COMPATIBILITY */

/*
 * Soft-lock address range; presumably before user physio.
 */
STATIC int
userdma(base, count, rw)
	register caddr_t base;
	register u_int count;
	register int rw;
{
	register res;

	res = (useracc(base, count, rw == B_READ ? S_WRITE:S_READ) == 1);
	if (res != 0) {
		if (as_fault(u.u_procp->p_as, base, count, F_SOFTLOCK,
		                rw == B_READ ? S_WRITE:S_READ))
			res = 1;
	}

	return(res);
}

/*
 * Soft-unlock address range; presumably after user physio.
 */
STATIC void
undma(base, count, rw)
	caddr_t base;
	size_t count;
	int rw;
{
	if (as_fault(u.u_procp->p_as, base, count, F_SOFTUNLOCK,
	             rw == B_READ ? S_WRITE : S_READ) != 0)
		cmn_err(CE_PANIC, "userdma unlock");
}


#ifdef	KPERF
/*	Copy kernel perf statistics from kernel buffer to
**	user buffer.  First record is number of records
**	copied.
*/

copyrecords(dataddr,buffer,numrcx)
int *dataddr, *buffer, numrcx;
{
	register int datalen;
	kernperf_t *bufx;

	datalen = numrcx * sizeof( kernperf_t);
	if (suword(buffer,numrcx) == -1) {
		u.u_error = EFAULT;
		return;
	}

	bufx = ( kernperf_t *) buffer;
	if ((copyout(dataddr,bufx+1,datalen)) == -1)
		u.u_error = EFAULT;
}
#endif	/* KPERF */
