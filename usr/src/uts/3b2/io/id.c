/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/id.c	1.17"
/*
 * 3B2 UNIX Integral Winchester Disk Driver
 */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sbd.h"
#include "sys/buf.h"
#include "sys/dma.h"
#include "sys/immu.h"
#include "sys/signal.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/buf.h"
#include "sys/elog.h"
#include "sys/iobuf.h"
#include "sys/systm.h"
#include "sys/firmware.h"
#include "sys/cmn_err.h"
#include "sys/vtoc.h"
#include "sys/hdelog.h"
#include "sys/open.h"
#include "sys/inline.h"
#include "sys/cred.h"
#include "sys/uio.h"
#ifdef KPERF
#include "sys/proc.h"
#include "sys/disp.h"
#endif /* KPERF */
#include "sys/ddi.h"
#include "vm/vm_hat.h"
#include "sys/id.h"
#include "sys/if.h"
#include "sys/debug.h"

#if defined(__STDC__)
STATIC void idflush(unsigned);
STATIC void idbreakup(struct buf *);
STATIC int idldcmd(u_char, u_char *, u_char, int);
STATIC void idrecal(int);
STATIC void idscan(void);
STATIC void idseek(int);
STATIC void idxfer(int);
STATIC void idtimeout(int);
STATIC void idsetblk(struct buf *, u_char, daddr_t, dev_t);
STATIC void idsetup(u_int);
extern void idstrategy(struct buf *);
#else
STATIC void idflush();
STATIC void idbreakup();
STATIC int idldcmd();
STATIC void idrecal();
STATIC void idscan();
STATIC void idseek();
STATIC void idxfer();
STATIC void idtimeout();
STATIC void idsetblk();
STATIC void idsetup();
extern void idstrategy();
#endif

int iddevflag = 0;

/* pointer to disk controller */
extern int idisk;
#define	ID ((struct iddev *) &idisk)

extern struct dma	dmac;
#define	DMAC (&dmac)

#define DIS_DMAC	0x04	/*  disables dmac chan for requested chan */

STATIC struct vtoc idvtoc[IDNDRV];

STATIC struct idsave idsvaddr[IDNDRV];

/* first block for current job converted to phyical blkno */
STATIC daddr_t blk1[IDNDRV];

/* defect maps */
extern struct defstruct iddefect[];

/* bad block stuff */
STATIC struct hdedata idelog[IDNDRV];
extern hdelog ();

extern int idiskmaj;	/* defined in master.d file for idisk */
/* physical description table */
STATIC struct pdsector	idsect0[IDNDRV];

/* seek parameter structure */
STATIC struct idseekstruct idseekparam[IDNDRV];

/* controller initialization */
STATIC struct idspecparam idspec_s;
STATIC struct idspecparam idspec_f;

/* transfer parameter sturcture */
STATIC struct idxferstruct idxferparam[IDNDRV];

/* drive status byte */
STATIC struct idstatstruct idstatus[IDNDRV];

/* temporary buffer for jobs which cross 64-Kbyte boundaries */
STATIC struct {
	unsigned int buf[128];
} idcache[IDNDRV+1];
STATIC unsigned int idpcacheaddr[IDNDRV];

struct	iobuf	idtab[IDNDRV];		/* drive information */
STATIC struct	iotime	idtime[IDNDRV];		/* drive status information */

STATIC int	idspurintr[10];		/* spurious interrupt counter */
extern	paddr_t id_addr[];	/* local bus base address of disk controller */
extern unsigned ifstate;	/* floppy driver state register */
extern struct buf *ngeteblk();


/* rename buf structure variables */
#define	acts	io_s1
#define	cylin	b_resid
#define ccyl	jrqsleep

STATIC unsigned char idscanflag[IDNDRV];
STATIC unsigned char idnoscan;
STATIC unsigned int idscancnt[IDNDRV];

STATIC void
idscan ()
{
	int s;
	register i;
	s = spl6();
	for(i = 0 ; i < IDNDRV ; i++)
	{				
		if(idtab[i].b_actf != IDNULL)
			if (idscanflag[i] == IDNULL)
			{			
				idscancnt[i]++;
				if(idscancnt[i] % 30 == 0)
					cmn_err(CE_WARN,"IDRECAL called %d times for drive %d\n",idscancnt[i],i);
				idrecal (IDNOUNIT);
			}	
			else 
				idscanflag[i] = IDNULL;
	} 				
	splx (s);
	timeout (idscan, 0, (10*HZ));
}
	
STATIC void
#ifdef __STDC__
idsetblk (struct buf *bufhead, u_char cmd, daddr_t blkno, dev_t dev) 
#else
idsetblk (bufhead, cmd, blkno, dev) 
struct buf *bufhead;
u_char cmd;
daddr_t blkno;
dev_t dev;
#endif
{
	clrbuf (bufhead);
	bufhead->b_flags |= cmd;
	bufhead->b_blkno = blkno;
	bufhead->b_edev = (dev|IDNODEV);
	bufhead->b_proc = 0x00;
	bufhead->b_flags &= ~B_DONE;
	if (cmd == B_WRITE)
		bufhead->b_bcount = idsect0[iddn(getminor(dev))].pdinfo.bytes;
}

/* Set up the initial values for pdsector and clear defect map */

STATIC void
idsetdef(unit)
int unit;
{
	register int j;

	/* initialize sector 0 */
	idsect0[unit].pdinfo.driveid = 0x02;
	idsect0[unit].pdinfo.sanity = 0x00;
	idsect0[unit].pdinfo.version = 0x01;
	idsect0[unit].pdinfo.cyls = 1;
	idsect0[unit].pdinfo.tracks = 1;
	idsect0[unit].pdinfo.sectors = 18;
	idsect0[unit].pdinfo.bytes = 512;

	/* initialize mach defect management tables */
	for (j=0; j<(IDDEFSIZ/8); j++) {
		iddefect[unit].map[j].bad.full = (unsigned long)~0;
		iddefect[unit].map[j].good.full = (unsigned long)~0;
	}
}

/* idopen - first call reads in physical description, vtoc, and defect info */
/* ARGSUSED */
int
idopen(devp,flag,otyp,cr)
dev_t *devp;
int flag;
int otyp;
cred_t *cr;
{
	dev_t dev = *devp;
	struct buf *geteblk();
	struct buf *bufhead;
	register int unit;
	int defcnt;
	caddr_t defaddr;
	int error = 0;

	if (idnoscan == IDSET) {
		idnoscan = IDNULL;
		idscan ();
	}
	unit = iddn(getminor(dev));
	if (idstatus[unit].equipped == IDNULL) {
		/* no disk out there */
		return(ENXIO);
	}
	while (idstatus[unit].open == IDOPENING)
		(void) sleep((caddr_t)&idstatus[unit],PZERO);
	if (idstatus[unit].open == IDNOTOPEN) {
		idstatus[unit].open = IDOPENING;

		/* set up default values in the pdsector */
		idsetdef(unit);

		/* read physical description sector */
		bufhead = geteblk();
		idsetblk (bufhead, B_READ, IDPDBLKNO, dev);
		idstrategy(bufhead);
		iowait(bufhead);
		if (bufhead->b_flags & B_ERROR) {
			cmn_err(CE_WARN,"\nhard disk:  cannot read sector 0 on drive %d\n",unit);
			goto badopen;
		}
		bcopy(bufhead->b_un.b_addr,
		  (caddr_t)&idsect0[unit], sizeof(struct pdsector));


 		if (idsect0[unit].pdinfo.sanity == VALIDINFO) {
 			idsetdef(unit);
 			cmn_err(CE_WARN,"\nhard disk:  Drive %d is in the 1.0 layout.  It can not be used until the conversion is made to the current layout.\n",unit);
 			goto opendone;
 		}else if (idsect0[unit].pdinfo.sanity != VALID_PD) {
			cmn_err(CE_WARN,"\nhard disk:  Bad sanity word on drive %d.\n",unit);
			goto badopen;
		}

		/* read the defect map */
		if (idsect0[unit].pdinfo.defectsz > IDDEFSIZ) {
			cmn_err (CE_WARN, "\nhard disk: too little space allocated in driver for defect table on drive %d\n", unit);
			goto badopen;
		}
		for (defcnt = 0; defcnt < (idsect0[unit].pdinfo.defectsz/idsect0[unit].pdinfo.bytes);defcnt++) {
			idsetblk (bufhead, B_READ, idsect0[unit].pdinfo.defectst+defcnt, dev);
			idstrategy(bufhead);
			iowait(bufhead);
			if (bufhead->b_flags & B_ERROR) {
				cmn_err(CE_WARN,"\nhard disk:  Cannot read defect map on drive %d\n",unit);
				goto badopen;
			}
			defaddr = (caddr_t)((int)&iddefect[unit])+(defcnt*idsect0[unit].pdinfo.bytes);
			bcopy(bufhead->b_un.b_addr, defaddr, idsect0[unit].pdinfo.bytes);
		}

		/* read in the vtoc */
		idsetblk (bufhead,B_READ, idsect0[unit].pdinfo.logicalst+IDVTOCBLK, dev);
		idstrategy(bufhead);
		iowait(bufhead);
		if (bufhead->b_flags & B_ERROR) {
			cmn_err(CE_WARN,"\nhard disk:  Cannot read the VTOC on drive %d\n",unit);
			goto badopen;
		}
		bcopy(bufhead->b_un.b_addr, (caddr_t)&idvtoc[unit], sizeof(struct vtoc));
		if (idvtoc[unit].v_sanity != VTOC_SANE) {
			cmn_err(CE_WARN,"\nhard disk: Bad sanity word in VTOC on drive %d.\n",unit);
			goto opendone;
		}

		/* open is complete - wakeup sleeping processes and return buffer */
		
		idstatus[unit].open = IDISOPEN;
		goto opendone;
badopen:
		error = ENXIO;
opendone:

		if (idstatus[unit].open != IDISOPEN)
			idstatus[unit].open = IDNOTOPEN;
		wakeup((caddr_t)&idstatus[unit]);
		brelse(bufhead);
	}
	return(error);
}

/* ARGSUSED */
int
idclose(dev,flag,otyp,cr)
dev_t dev;
int flag;
int otyp;
cred_t *cr;
{
	return(0);
}

/* reset and initialize controller, initialize controller table */
/* check for drive ready and recalibrate drives */
void
idinit()
{
	register int i, j;
	int iplsave;
 	dev_t ddev;

	/* ENTER CRITICAL REGION */
	iplsave = spl6();

	/*  controller initialization - specify parameter structure  */
	idspec_s.mode = 0x18;
	idspec_s.dtlh = 0xe2;
	idspec_s.dtll = 0x00;
	idspec_s.etn = 0xef; 
	idspec_s.esn = 0x11;
	idspec_s.gpl2 = 0x0d;
	idspec_s.rwch = 0x00;
	idspec_s.rwcl = 0x80;

	idspec_f.mode = 0x1f;
	idspec_f.dtlh = 0xe2;
	idspec_f.dtll = 0x00;
	idspec_f.etn = 0xef;
	idspec_f.esn = 0x11;
	idspec_f.gpl2 = 0x0d;
	idspec_f.rwch = 0x00;
	idspec_f.rwcl = 0x80;

	for (i=0, j=0; i < IDNDRV; i++, j++) { 
		/* initialize drive state */
		idstatus[i].open = IDNOTOPEN;
		idstatus[i].state = IDIDLE;
		idstatus[i].equipped = IDNULL;
		idpcacheaddr[i] = (unsigned int) vtop((caddr_t)&idcache[j],IDNULL);
		if (((idpcacheaddr[i] & MSK64K)+0x200)>BND64K) {
			j++;
			idpcacheaddr[i]=(unsigned int)vtop((caddr_t)&idcache[j],IDNULL);
		}
	}
	idrecal (IDNOUNIT);
	idnoscan = IDSET;
	idscanflag[0] = IDSET;
	idscanflag[1] = IDSET;
	/* EXIT CRITICAL REGION */
	splx (iplsave);
	if ((j = (int)itoemajor(idiskmaj, -1)) != -1) {
		for (i = 0; i < IDNDRV; i++)
			if (idstatus[i].equipped != IDNULL) {
				ddev = makedevice(j, idmkmin(i));
				hdeeqd(cmpdev(ddev), IDPDBLKNO, EQD_ID);
			}
	}
} /* end idinit */

void
idstrategy (bufhead)
register struct buf *bufhead;
{
	register struct iobuf *drvtab;	/* drive status pointer */
	daddr_t	lastblk;		/* last block in partition */
	register int unit;		/* drive unit id */
	int	partition;		/* drive partition number */
	int	iplsave;		/* saved interrupt level */
 	int	sectoff;		/* start sector of this partition */

	/* initialize local variables */
	partition = idslice (getminor (bufhead->b_edev));
	unit = iddn (getminor (bufhead->b_edev));
	if (idstatus[unit].equipped == IDNULL)
		goto diskerr;

	if (idnodev (getminor (bufhead->b_edev))) {
		lastblk = (idsect0[unit].pdinfo.sectors * idsect0[unit].pdinfo.tracks * idsect0[unit].pdinfo.cyls);
		sectoff = 0x00;
	} else {
		/* check for invalid VTOC */
		if (idvtoc[unit].v_sanity != VTOC_SANE) {
			goto diskerr;
		}
		/* check for read only partition */
		if (((idvtoc[unit].v_part[partition].p_flag & V_RONLY)==V_RONLY)&&((bufhead->b_flags & B_READ)!=B_READ)) {
			cmn_err (CE_WARN, "\nhard disk: partition %d on drive %d is marked read only\n", partition, unit);
			goto diskerr;
		}
		lastblk = idvtoc[unit].v_part[partition].p_size;
 		sectoff = (idvtoc[unit].v_part[partition].p_start 
 			+ idsect0[unit].pdinfo.logicalst);
	}
	drvtab = &idtab[unit];

	/* if the requested block does not exist within requested partition */
	if ((bufhead->b_blkno >= lastblk) ||
	    (bufhead->b_blkno < IDFRSTBLK)  ||
	    (bufhead->b_blkno+((bufhead->b_bcount-1)/idsect0[unit].pdinfo.bytes) >= lastblk)) {
		if ((bufhead->b_blkno == lastblk) && (bufhead->b_flags&B_READ)) {
			/* this case is here to help read ahead */
			bufhead->b_resid = bufhead->b_bcount;
			iodone(bufhead);
			return;
		}
		goto diskerr;
	}


	/* ENTER CRITICAL REGION */
	iplsave = spl6();
 	bufhead->cylin	= ((bufhead->b_blkno+sectoff)
 			/(idsect0[unit].pdinfo.sectors
 			*idsect0[unit].pdinfo.tracks)); 
	(void) drv_getparm(LBOLT, &bufhead->b_start); /* time stamp request */
	idtime[unit].io_cnt++;    /* inc operations count */
	/*	Increment disk block count */

	idtime[unit].io_bcnt +=
		(bufhead->b_bcount + NBPSCTR-1) >> SCTRSHFT;
	drvtab->qcnt++;		  /* inc drive current request count */


	/* link buffer header to drive worklist */
	bufhead->av_forw = IDNULL;
	if (drvtab->b_actf == IDNULL) {
		idscanflag[unit] = IDSET;
		drvtab->b_actf = bufhead;
		drvtab->b_actl = bufhead;
		drvtab->acts = (int)bufhead;
		idsetup (unit);
		idseek (unit);
	} else {
 		register struct buf *ap, *cp;
 		if (((int)idtime[unit].io_cnt&0x0f) == 0)
 			drvtab->acts = (int)drvtab->b_actl;
 		for (ap = (struct buf *)drvtab->acts; 
		  (cp = ap->av_forw) != 0; ap = cp) {
 			int s1, s2;
 			if ((s1 = ap->cylin - bufhead->cylin)<0)
 				s1 = -s1;
 			if ((s2 = ap->cylin - cp->cylin)<0)
 				s2 = -s2;
 			if (s1 < s2)
 				break;
 		}
 		ap->av_forw =  bufhead;
 		if ((bufhead->av_forw = cp) == NULL)
 			drvtab->b_actl = bufhead;
 		bufhead->av_back = ap;
	}
	/* EXIT CRITICAL REGION */
	splx (iplsave);
	return;

diskerr:
	bufhead->b_flags |= B_ERROR;
	bufhead->b_error = ENXIO;
	iodone (bufhead);
	return;
}

STATIC void
idsetup (unit)	/* This routine fills the drive command buff from buff head */
unsigned int unit;
{
	register int	blkcnt;		/* number of blocks this job */
	register struct buf *bufhead;	/* buffer header */
	register struct iobuf *drvtab;	/* head of drive worklist */
	register unsigned char *user, *driver;
	union diskaddr daddress;	/* disk address for current job */
	caddr_t vaddress;		/* virtual memory address */
	int paddress;			/* physical memory address */
	int	sectoff;		/* sector offset into drive */
	int 	sectno;			/* sector number on cylinder */
	int	partition;		/* drive partition number */
	struct defect *deftab;		/* pointer to the defect table */
	union diskaddr lastsect;	/* last sector for this job */
	int lsectoff;			/* offset of the last sector in job */
	int i;
	int defcnt, bytes;
	unsigned char partial;
	
	/* initialize local variables */
	drvtab = &idtab[unit];
	bufhead = drvtab->b_actf;
	deftab = iddefect[unit].map;
	partition = idslice (getminor (bufhead->b_edev));
	if (idnodev (getminor (bufhead->b_edev)))
		sectoff = 0x00;
	else
		sectoff = (idvtoc[unit].v_part[partition].p_start + idsect0[unit].pdinfo.logicalst);

	/* if no work on worklist */
	if (bufhead == IDNULL) 
		return;

	/* if this is the first time this job has come through, time stamp it 
	   and save buffer header information */
	if (drvtab->b_active == 0) {
		idsvaddr[unit].b_addr = bufhead->b_un.b_addr;
		idsvaddr[unit].b_blkno = bufhead->b_blkno;
		idsvaddr[unit].b_bcount = bufhead->b_bcount;
		(void) drv_getparm(LBOLT, &drvtab->io_start);
	}

	blk1[unit] = idsvaddr[unit].b_blkno + sectoff;

	/* increase activity count */
	drvtab->b_active++;

	/* clear result information */
	idstatus[unit].retries = IDRETRY;
	idstatus[unit].reseeks = IDRESEEK;


	/* compute disk address */
	bufhead->cylin = ((idsvaddr[unit].b_blkno+sectoff)/(idsect0[unit].pdinfo.sectors*idsect0[unit].pdinfo.tracks)); /* start cylinder */
	sectno = (idsvaddr[unit].b_blkno+sectoff)%(idsect0[unit].pdinfo.sectors*idsect0[unit].pdinfo.tracks);	/* offset into start cylinder */

	/* load disk address */
	daddress.part.pcnh = (bufhead->cylin>>8)&0xff;
	daddress.part.pcnl = bufhead->cylin&0xff;
	daddress.part.phn = sectno/idsect0[unit].pdinfo.sectors;
	daddress.part.psn = sectno%idsect0[unit].pdinfo.sectors;

	/* get physical address from buffer header */
	vaddress = idsvaddr[unit].b_addr;
	paddress = vtop(vaddress, bufhead->b_proc);
	if (paddress==IDNULL) 
		cmn_err(CE_PANIC,"\nhard disk: Bad address returned by VTOP\n");
	/* blocks to do this job */
 	blkcnt = idsvaddr[unit].b_bcount/idsect0[unit].pdinfo.bytes;

	/* chop the job up */
	/* make sure we don't overrun track boundary */
	if ((daddress.part.psn+blkcnt) > idsect0[unit].pdinfo.sectors)
		blkcnt = idsect0[unit].pdinfo.sectors - daddress.part.psn;

	/* check for 64K-byte boundary overrun  or partial sector r/w */
	partial = 0;
 	if (idsvaddr[unit].b_bcount < idsect0[unit].pdinfo.bytes)
		partial = 1;
	if ((((paddress & MSK64K)+(blkcnt*idsect0[unit].pdinfo.bytes)) > BND64K)
 	|| (partial)) {
		blkcnt = (BND64K - (paddress & MSK64K)) / idsect0[unit].pdinfo.bytes;
		/* if sector r/w crosses 64-Kbyte boundary or partial sector */
		if ((blkcnt == 0) || partial) {
			blkcnt = 1;
			/* if its a write to disk, copy form user to driver */
			if ((bufhead->b_flags&B_READ) != B_READ) {
				bytes = idsect0[unit].pdinfo.bytes;
				if (partial) {
					register unsigned int *zp;
 					bytes = idsvaddr[unit].b_bcount;
					zp = (unsigned int *)idpcacheaddr[unit];
					for (i=0; i<128; i++)
						*zp++ = 0x00000000;
				}
				user = (unsigned char *) paddress;
				driver = (unsigned char *) idpcacheaddr[unit];
				for (i=0; i<bytes; i++)
					*driver++ = *user++;
			}
			paddress = idpcacheaddr[unit];
		}
	}

	/* look for any defective sectors in this job */
	for (defcnt=0;(defcnt<(IDDEFSIZ/8))&&(daddress.full>deftab->bad.full); defcnt++) {
		deftab++;
	}
	/* determine the address of the last sector for this job */
	lastsect.part.pcnh = daddress.part.pcnh;
	lastsect.part.pcnl = daddress.part.pcnl;
	lastsect.part.phn = (sectno+blkcnt-1)/(idsect0[unit].pdinfo.sectors);
	lastsect.part.psn = (sectno+blkcnt-1)%(idsect0[unit].pdinfo.sectors);

	if (defcnt < (IDDEFSIZ/8) && lastsect.full >= deftab->bad.full) {
		if (daddress.full == deftab->bad.full) {
			daddress.full = deftab->good.full;
			blkcnt=1;
		} else {
			lsectoff = (deftab->bad.part.phn*(idsect0[unit].pdinfo.sectors))+deftab->bad.part.psn;
			blkcnt = lsectoff-sectno;
		}
	}
			
		
	/* load seek parameters */
	idseekparam[unit].pcnh = daddress.part.pcnh;
	idseekparam[unit].pcnl = daddress.part.pcnl;

	/* load transfer parameters */
	idxferparam[unit].phn = daddress.part.phn;
	idxferparam[unit].lcnh = ~daddress.part.pcnh;
	idxferparam[unit].lcnl = daddress.part.pcnl;
	idxferparam[unit].lhn = daddress.part.phn;
	idxferparam[unit].lsn = daddress.part.psn;
	idxferparam[unit].scnt = (u_char)blkcnt;
	idxferparam[unit].bcnt = blkcnt*idsect0[unit].pdinfo.bytes;
	idxferparam[unit].necop = (bufhead->b_flags&B_READ) ? IDREAD:IDWRITE;
	idxferparam[unit].dmacop = (bufhead->b_flags&B_READ) ? WDMA:RDMA;
	idxferparam[unit].b_addr = paddress;
	idxferparam[unit].unitno = unit;
	/* if the head number is greater than 7 */
	if (idxferparam[unit].phn >= IDMAXHD) 
		idxferparam[unit].unitno += IDADDEV;
	
	/* adjust remaining byte count and start address */
	if (idxferparam[unit].b_addr != idpcacheaddr[unit]) {
		idsvaddr[unit].b_bcount -= (blkcnt*idsect0[unit].pdinfo.bytes);
		idsvaddr[unit].b_addr += (blkcnt*idsect0[unit].pdinfo.bytes);
	}
	idsvaddr[unit].b_blkno = idsvaddr[unit].b_blkno + blkcnt;
	return;
} /* end idsetup */

STATIC void
idrecal (unit)
register int unit;
{
	unsigned int i, j;
	unsigned char retval[2];
	register struct buf *bufhead;
	register struct iobuf *drvtab;
	register struct idstatstruct *stat;
	unsigned short cyl;
 	int sects, tracks;

	if (unit != IDNOUNIT)
		idstatus[unit].reseeks--;
	(void)idldcmd(IDRESET, NULL, 0, IDINTON);
	/* wait for controller to reset */
	for (i=0; i < 1000; i++)
	;
	/* re-specify controller characteristics */
	(void)idldcmd(IDSPECIFY, (u_char *)&idspec_s, IDSPECCNT, IDINTOFF);
	/* clear out not-ready interrupts from nonexisting drives */
	for (i=0; i<10000; i++)
	;
	if (ID->statcmd & IDSINTRQ) {
		(void)idldcmd(IDSENSEINT, NULL, 0, IDINTOFF);
		while ((ID->statcmd & IDSINTRQ) != IDSINTRQ)
		;
		(void)idldcmd(IDSENSEINT, NULL, 0, IDINTOFF);
	}
	/* init each drive attached to controller */
	for (i=0; i < IDNDRV; i++) {
		stat = &idstatus[i];
		/* check for drive ready */
		if (idldcmd(IDSENSEUS|i, NULL, 0,IDINTOFF)==IDFAIL) {
			if (stat->equipped == IDSET) {
				stat->equipped = IDNULL;
				idflush (i);
			}
			continue;
		}
		stat->ustbyte = ID->fifo;
		if ((stat->ustbyte & IDREADY) != IDREADY) {
			if (stat->equipped == IDSET) {
				stat->equipped = IDNULL;
				idflush (i);
			}
			continue;
		}
		stat->equipped = IDSET;
		retval[i] = IDFAIL;
		for (j=0; ((retval[i] == IDFAIL) && (j<4)); j++) {
			if (idldcmd(IDRECAL|i|IDBUFFERED, NULL, 0, IDINTOFF) == IDFAIL)
				continue;
			while ((ID->statcmd & IDSINTRQ) != IDSINTRQ)
			;
			if (idldcmd(IDSENSEINT, NULL,0,IDINTOFF)==IDFAIL)
				continue;
			stat->istbyte = ID->fifo;
			if ((stat->istbyte & (IDSEEKEND|IDSEEKERR)) != IDSEEKEND)
				continue;
			idtab[i].ccyl = 0;
			stat->state = IDIDLE;
			retval[i] = IDPASS;
		}
		if (retval[i] == IDFAIL) {
			stat->equipped = IDNULL;
			cmn_err(CE_WARN,"\nhard disk: cannot recal drive %d\n", i);
			idflush (i);
		}
	}
	(void)idldcmd(IDSPECIFY, (u_char *)&idspec_f, IDSPECCNT, IDINTOFF);
	if (unit != IDNOUNIT) {
		stat = &idstatus[unit];
		drvtab = &idtab[unit];
		if ((stat->reseeks == 0) && (drvtab->b_actf != IDNULL)) {
			time_t mylbolt;

			bufhead = drvtab->b_actf;
			drvtab->b_active = IDNULL;
			drvtab->b_actf = bufhead->av_forw;
			bufhead->b_flags |= B_ERROR;
			bufhead->b_error |= EIO;
			bufhead->b_resid = 0;
			drvtab->qcnt--;
 			if (bufhead == (struct buf *)drvtab->acts)
 				drvtab->acts = (int)drvtab->b_actf;
			/* update status information */
			(void) drv_getparm(LBOLT, &mylbolt);
			idtime[unit].io_resp += mylbolt - bufhead->b_start;
			idtime[unit].io_act += mylbolt - drvtab->io_start;
	
			stat->state = IDIDLE;
			cyl=((idseekparam[unit].pcnh<<8)|idseekparam[unit].pcnl);
  			idelog[unit].diskdev = cmpdev(bufhead->b_edev) & ~(IDNODEV|idslice((-1)));
 			sects = idsect0[unit].pdinfo.sectors;
 			tracks = idsect0[unit].pdinfo.tracks;
  			idelog[unit].blkaddr = 
 				(cyl*sects*tracks) 
 				+ (stat->lhn*sects) 
				+ stat->lsn;
 			idelog[unit].readtype = HDECRC;
 			idelog[unit].severity = HDEUNRD;
 			idelog[unit].bitwidth = 0;
			(void) drv_getparm(TIME, &idelog[unit].timestmp);
			for (i=0;i<12;++i)
				idelog[unit].dskserno[i]=idsect0[unit].pdinfo.serial[i];
 			hdelog (&idelog[unit]);
			cmn_err (CE_WARN,"\nhard disk: cannot access sector %d, head %d, cylinder %d, on drive %d\n",
				stat->lsn, stat->lhn, cyl, unit);
			/* return buffer header to UNIX */
			iodone (bufhead);
			
			if (drvtab->b_actf != IDNULL)
				idsetup (unit);
		}
	}
	for (i=0; i < IDNDRV; i++)
		if (idtab[i].b_actf != IDNULL)
			idseek (i);
}

/* start seek for drive specified */
STATIC void
idseek (unit)
register int unit;
{
	unsigned int other;
	int iplsave;

	other = (unit^1);
	idstatus[unit].state = IDSEEK0;
	iplsave = spl6();
	if ((idstatus[other].state & IDBUSY) == IDBUSY) {
		idstatus[unit].state |= IDWAITING;
		splx(iplsave);
		return;
	}
	idstatus[unit].state |= IDBUSY;
	splx(iplsave);
	if (idtab[unit].ccyl ==
 	  ((idseekparam[unit].pcnh<<8)|(idseekparam[unit].pcnl))) {
		idxfer (unit); return;
	}
		
	/* set drive current cylinder */
	idtab[unit].ccyl=
	  ((idseekparam[unit].pcnh<<8)|(idseekparam[unit].pcnl));
	(void)idldcmd (IDSEEK|unit|IDBUFFERED, 
	  (u_char *)&idseekparam[unit], IDSEEKCNT, IDINTON);
}

STATIC void
idtimeout (unit) 
register int unit;
{
	int iplsave;
	iplsave = spl6 ();

dma_access (CH0IHD, idxferparam[unit].b_addr, idxferparam[unit].bcnt,
	    DMNDMOD, idxferparam[unit].dmacop);
(void)idldcmd(idxferparam[unit].necop|idxferparam[unit].unitno,
	(u_char *)&idxferparam[unit],IDXFERCNT,IDINTON);
	splx (iplsave);
}

STATIC void
idxfer (unit)
register int unit;
{
	unsigned int other;
	int iplsave;
	unsigned ifcount;
	other = (unit^1);
	idstatus[unit].state = IDXFER;
	iplsave = spl6();
	if ((idstatus[other].state & IDBUSY) == IDBUSY) {
		idstatus[unit].state |= IDWAITING;
		splx(iplsave);
		return;
	}
	idstatus[unit].state |= IDBUSY;
	splx(iplsave);
	idstatus[unit].retries--;
	if (idstatus[unit].retries == 0) {
		idstatus[unit].retries = IDRETRY;
		idrecal (unit);
		return;
	}
	if ((ifstate & IFFMAT1) == IFFMAT1) {
		timeout (idtimeout, (caddr_t)unit, (2*HZ)/5);
		return;
	}
	if ((ifstate&IFBUSYF) == IFBUSYF) {
		iplsave = spltty ();
		DMAC->CBPFF = IDNULL;
		ifcount = 0;
		ifcount = DMAC->C1WC;
		ifcount |= (DMAC->C1WC<<8);
		if (ifcount != IFDMACNT) {
			timeout (idtimeout, (caddr_t)unit, HZ/22);
			splx (iplsave);
			return;
		}
		splx (iplsave);
	}
		
	/* load the DMAC */
dma_access (CH0IHD, idxferparam[unit].b_addr, idxferparam[unit].bcnt,
	    DMNDMOD, idxferparam[unit].dmacop);
if (idldcmd(idxferparam[unit].necop|idxferparam[unit].unitno,
	 (u_char *)&idxferparam[unit],IDXFERCNT,IDINTON) == IDFAIL) 
		return;
	return;
} /* end idxfer */

void
idint()
{
	register struct buf *bufhead;
	register unsigned char statreg;
	register unsigned int unit;
	register unsigned char *driver, *user;
	struct iobuf *drvtab;
	caddr_t vaddress;	/* virtual memory address of user space */
	unsigned char istbyte;
	unsigned int other;
	caddr_t fromaddr;
	int i, bytes;
	time_t mylbolt;

statreg = ID->statcmd;

#ifdef KPERF
	if (kpftraceflg)
		kperf_write(KPT_INTR, idint, curproc);
#endif /* KPERF */

/* check spurious interrupt */
if ((statreg & (IDSINTRQ|IDENDMASK)) == 0) {
	/* increment spurious interrupt count */
	idspurintr[0]++; return;
}

/* establish unit for command end interrupt */
if ((statreg & IDENDMASK) != 0) {
	if ((idstatus[0].state & IDBUSY) == IDBUSY)
		unit = 0;
	else if ((idstatus[1].state & IDBUSY) == IDBUSY)
		unit = 1;
	else {
		idspurintr[1]++;
		ID->statcmd = IDCLCMNDEND;
		return;
	}
	idscanflag[unit] = IDSET;
}
if ((statreg & IDSINTRQ) == IDSINTRQ) {
	/* if the controller is busy, mask the interrupt */
	if ((statreg & IDCBUSY) == IDCBUSY) {
		if ((ID->statcmd & IDCBUSY) == IDCBUSY)
			ID->statcmd = IDMASKSRQ;
		return;
	}
	if ((idstatus[0].state & IDBUSY) || (idstatus[1].state & IDBUSY)) {
		if ((statreg & IDENDMASK) == 0) {
			ID->statcmd = IDMASKSRQ; return;
		}
	}
	else {
		if (idldcmd(IDSENSEINT,NULL, 0,IDINTOFF)==IDFAIL) {
			idrecal(IDNOUNIT); return;
		}
		istbyte = ID->fifo;
		unit = istbyte & IDUNITADD;
		if (unit >= IDNDRV) {
			idspurintr[2]++; return;
		}
		idstatus[unit].istbyte = istbyte;
		if ((idstatus[unit].istbyte & IDSEEKMSK) != IDSEEKEND) {
			idrecal (unit); return;
		}
		if ((idstatus[unit].state & IDSEEK1) != IDSEEK1) {
			idspurintr[3]++; return;
		}
	}
}

	switch (idstatus[unit].state & (IDSEEK0|IDSEEK1|IDXFER)) {
	/* Driver expected seek complete? */
	case IDSEEK0:
		ID->statcmd = IDCLCMNDEND;
		other = (unit^1);
		if ((statreg & IDENDMASK) != IDCMDNRT) {
			idrecal (unit); return;
		}
		idstatus[unit].state = IDSEEK1;
		if ((idstatus[other].state & IDWAITING) == IDWAITING) {
			if ((idstatus[other].state & IDSEEK0) == IDSEEK0) {
				idseek (other); return;
			}
			if ((idstatus[other].state & IDXFER) == IDXFER) {
				idxfer (other); return;
			}
		}
		return;

	case IDSEEK1:
		if ((idstatus[unit].istbyte & IDSEEKMSK) != IDSEEKEND) {
			idrecal (unit); return;
		}
		idxfer (unit); return;

	case IDXFER:
		/* access extended access information */
		idstatus[unit].statreg = statreg;

		/*
		 * There is a bug in the integral disk controller chip. It
		 * generates a spurious DMA request when one reads extended
		 * status after a fault in which a reset is requested. This
		 * hangs the arbiter pending a DMA which will never actually
		 * occur. We work around this by disabling the DMA controller
		 * hard disk channel when the disk controller primary status
		 * shows "reset requested".
		 */
		if (statreg & IDRESETRQ)
			DMAC->WMKR = CH0IHD | DIS_DMAC;

		idstatus[unit].estbyte = ID->fifo;
		idstatus[unit].phn = ID->fifo;
		idstatus[unit].lcnh = ID->fifo;
		idstatus[unit].lcnl = ID->fifo;
		idstatus[unit].lhn = ID->fifo;
		idstatus[unit].lsn = ID->fifo;
		idstatus[unit].scnt = ID->fifo;

		ID->statcmd = IDCLCMNDEND;
		idstatus[unit].state &= ~IDBUSY;
		other = (unit^1);

		/* format controller has lost control of drive ? */
		if ((statreg & IDRESETRQ) || (statreg & IDERROR)) {
			idrecal (unit); return;
		}
		/* command terminated abnormally ? */
		if (statreg&IDCMDABT) {
			if (idstatus[unit].estbyte & (IDDMAOVR|IDEQUIPTC|IDDATAERR)) {
				idxfer (unit); return;
			}
			idrecal (unit); return;
		}
		if ((idstatus[other].state & IDWAITING) == IDWAITING) {
			if ((idstatus[other].state & IDSEEK0) == IDSEEK0) {
				idseek (other); goto wrapup;
			}
			if ((idstatus[other].state & IDSEEK1) == IDSEEK1) {
				goto wrapup;
			}
			idxfer (other);
		}
		break;
	default:
		cmn_err(CE_WARN,"ID didn't get IDSEEK0, IDSEEK1, or IDXFER!"); 
	}

wrapup:
	drvtab = &idtab[unit];
	bufhead = drvtab->b_actf;

	/* If a write job and the block number is the VTOC block
	   update the memory image of the VTOC */
	if (((bufhead->b_flags&B_READ) != B_READ)
	     && (blk1[unit] <= (idsect0[unit].pdinfo.logicalst + IDVTOCBLK))) {
		if ((blk1[unit] == idsect0[unit].pdinfo.logicalst)
		    && (idxferparam[unit].bcnt > idsect0[unit].pdinfo.bytes)) {
			fromaddr = (caddr_t)vtop(bufhead->b_un.b_addr
			           + idsect0[unit].pdinfo.bytes, bufhead->b_proc);
			bcopy(fromaddr, (caddr_t)&idvtoc[unit], sizeof(struct vtoc));
		}

		if ((blk1[unit] == (idsect0[unit].pdinfo.logicalst + IDVTOCBLK))
		    && (idxferparam[unit].bcnt >= sizeof(struct vtoc))) {
			fromaddr = (caddr_t)vtop(bufhead->b_un.b_addr , bufhead->b_proc);
			bcopy(fromaddr, (caddr_t)&idvtoc[unit], sizeof(struct vtoc));
		}
	}

	/* if buffering for 64K-byte boundary crossing or partial sector r/w */
	if (idxferparam[unit].b_addr==idpcacheaddr[unit]) {
		bytes = idsect0[unit].pdinfo.bytes;
		if (idsvaddr[unit].b_bcount < bytes)
			bytes = idsvaddr[unit].b_bcount;
		/* if reading disk, copy out to user space */
		if (idxferparam[unit].necop==IDREAD) {
			vaddress = idsvaddr[unit].b_addr;
			user = (u_char *)vtop(vaddress, bufhead->b_proc);
			driver = (unsigned char *)idpcacheaddr[unit];
			for (i=0; i<bytes; i++)
				*user++ = *driver++;
		}
		idsvaddr[unit].b_addr += bytes;
		idsvaddr[unit].b_bcount -= bytes;
	}
	/* if not done with multi-sector job */
	if (idsvaddr[unit].b_bcount > 0) {
		idsetup (unit);
		idseek(unit);
		return;
	}
	
	/* re-initialize drive worklist header information and unlink buffer header */
	idstatus[unit].state = IDIDLE;
	drvtab->b_active = IDNULL;
	drvtab->b_actf = bufhead->av_forw;
	bufhead->b_resid = 0;
	drvtab->qcnt--;
 	if (bufhead == (struct buf *)drvtab->acts)
 		drvtab->acts = (int)drvtab->b_actf;
	/* update status information */
	(void) drv_getparm(LBOLT, &mylbolt);
	idtime[unit].io_resp += mylbolt - bufhead->b_start;
	idtime[unit].io_act += mylbolt - drvtab->io_start;

	/* return buffer header to UNIX */
	iodone (bufhead);
	
 	/* if no active jobs for drive */
	if (drvtab->b_actf != IDNULL) {
		idsetup (unit);	/* load job parameters in command buffer */
		idseek(unit);
	}
}

STATIC void
idflush (unit)
register unsigned unit;
{
	register struct buf *bufhead;
	register struct iobuf *drvtab;
	drvtab = &idtab[unit];
	while (drvtab->b_actf != IDNULL) {
		bufhead = drvtab->b_actf;
		drvtab->b_active = IDNULL;
		drvtab->b_actf = bufhead->av_forw;
		bufhead->b_resid = bufhead->b_bcount;
		bufhead->b_flags |= B_ERROR;
		bufhead->b_error |= EIO;
		drvtab->qcnt--;
 		if (bufhead == (struct buf *)drvtab->acts)
 			drvtab->acts = (int)drvtab->b_actf;
		idstatus[unit].state = IDIDLE;
		/* return buffer header to UNIX */
		iodone (bufhead);
	}
	cmn_err(CE_WARN,"\nhard disk: drive %d out of service\n", unit);
}

STATIC void
idbreakup(bp)
register struct buf *bp;
{
	dma_pageio(idstrategy, bp);
}

/* ARGSUSED */
int
idread(dev, uiop, cr)
register dev_t dev;
register uio_t *uiop;
register cred_t *cr;
{
	register minor_t i;
	i = getminor(dev);
	return(physiock(idbreakup, (struct buf *)0, dev, B_READ,
	  (daddr_t)idvtoc[iddn(i)].v_part[idslice(i)].p_size, uiop));
}

/* ARGSUSED */
int
idwrite(dev, uiop, cr)
register dev_t dev;
register uio_t *uiop;
register cred_t *cr;
{
	register minor_t i;
	i = getminor(dev);
	return(physiock(idbreakup, (struct buf *)0, dev, B_WRITE,
	  (daddr_t)idvtoc[iddn(i)].v_part[idslice(i)].p_size, uiop));
}

void
idprint (dev,str)
dev_t dev;
char *str;
{
	cmn_err(CE_NOTE,
		"%s on integral hard disk %d, partition %d\n",
		str, iddn(getminor(dev)), idslice(dev));
}

int
idsize(dev)
dev_t dev;
{
	register int unit;
	register long nblks;
	int error = 0;
	dev_t tempdev = dev;

	unit = iddn(getminor(dev));
	if (idstatus[unit].open != IDISOPEN) {
		/*
		 * Initialize.  (The VTOC must be read in order to get the
		 * partition table.)
		 */
		error = idopen(&tempdev, 0, OTYP_LYR, (cred_t *)0);
		if (error)
			return(-1);
		error = idclose(tempdev, 0, OTYP_LYR, (cred_t *)0);
		if (error)
			return(-1);
	}
	if (idvtoc[unit].v_sanity != VTOC_SANE)
		nblks = -1;
	else
		nblks = idvtoc[unit].v_part[idslice(getminor(dev))].p_size;
	return nblks;
}

/* routine to load hard disk controller */
STATIC int
#ifdef __STDC__
idldcmd(u_char command, u_char *params, u_char paramcnt, int intopt)
#else
idldcmd(command, params, paramcnt, intopt)
u_char command; /* command opcode */
u_char *params;	/* pointer to parameter list */
u_char intopt; 	/* interrupt option */
int paramcnt; 	/* number of parameters for this command */
#endif
{
	while (ID->statcmd & IDCBUSY) /* wait for controller not busy */
		;
	ID->statcmd = IDCLFIFO;		/* clear parameter fifo */
	while (paramcnt != 0) {
		ID->fifo = *params++;
		paramcnt--;
	}
	ID->statcmd = command;	/* load command opcode into controller */
	if (intopt)
		return (IDPASS);
	/* wait for command end from controller */
	while (ID->statcmd & IDCBUSY)
	;
	if ((ID->statcmd & IDENDMASK) != IDCMDNRT) {
		ID->statcmd = IDCLCMNDEND;
		return (IDFAIL);
	}
	ID->statcmd = IDCLCMNDEND;
	return (IDPASS);
}

ididle()
{
	register int i;
	for (i=0;i<IDNDRV;i++)
		if (idtab[i].b_actf != IDNULL)
			return(1);
	return(0);
}

/* ARGSUSED */
int
idioctl(dev,cmd,arg,flag,cr,rvalp)
dev_t dev;
int cmd;
struct io_arg *arg;
int flag;
cred_t *cr;
int *rvalp;
{
	struct buf *geteblk();
	struct buf *bufhead;
	int errno, xfersz;
	register int unit;
	daddr_t block;
	caddr_t mem;
	unsigned int count, numbytes, defblock;
	struct io_arg karg;	/* copy of user arg struct */

	if(copyin((char *)arg, (char *)&karg, sizeof(struct io_arg))) {
		errno = V_BADREAD;
		suword(&arg->retval, errno);
		return(0);
	}

	mem = (caddr_t)karg.memaddr;
	unit = iddn (getminor (dev));

	switch (cmd) {

	case V_PREAD:
		bufhead = geteblk();
		block = karg.sectst;
		count = karg.datasz;
		while (count) {
			idsetblk (bufhead, B_READ, block, dev);
			idstrategy(bufhead);
			iowait(bufhead);
			if (bufhead->b_flags & B_ERROR) {
				errno = V_BADREAD;
				suword (&arg->retval,errno);
				goto ioctldone;
			}
			xfersz = min (count, bufhead->b_bcount);
			if (copyout(bufhead->b_un.b_addr, mem, xfersz) != 0) {
				errno = V_BADREAD;
				suword (&arg->retval,errno);
				goto ioctldone;
			}
			block+=2;
			count -= xfersz;
			mem += xfersz;
		}
		break;

	case V_PWRITE:
		bufhead = geteblk();
		block = karg.sectst;
		count = karg.datasz;
		defblock = idsect0[unit].pdinfo.defectst;
		numbytes = 0;
		while (count) {
			idsetblk (bufhead, B_WRITE, block, dev);
			xfersz = min (count, bufhead->b_bcount);
			if (copyin (mem, bufhead->b_un.b_addr, xfersz) != 0) {
				errno = V_BADWRITE;
				suword(&arg->retval, errno);
				goto ioctldone;
			}
			idstrategy(bufhead);
			iowait(bufhead);
			if (bufhead->b_flags & B_ERROR) {
				errno = V_BADWRITE;
				suword(&arg->retval, errno);
				goto ioctldone;
			}

			/* update memory image if special data */
			if (bufhead->b_blkno ==  IDPDBLKNO) {
				bcopy(bufhead->b_un.b_addr, 
			  	  (caddr_t)&idsect0[unit],
				  xfersz);
				defblock = idsect0[unit].pdinfo.defectst;
			}
			if (bufhead->b_blkno == defblock) {
				defblock++;
				bcopy(bufhead->b_un.b_addr,
				  (caddr_t)(((u_int)&iddefect[unit])+numbytes),
				  xfersz);
				numbytes += xfersz;
			} 

			block+=1;
			count -= xfersz;
			mem += xfersz;
		}
		break;

	case V_PDREAD:
		bufhead = geteblk();
		idsetblk (bufhead, B_READ, IDPDBLKNO, dev);
		idstrategy(bufhead);
		iowait(bufhead);
		if (bufhead->b_flags & B_ERROR) {
			errno = V_BADREAD;
			suword (&arg->retval,errno);
			goto ioctldone;
		}
		if (copyout(bufhead->b_un.b_addr, mem,
		  idsect0[unit].pdinfo.bytes) != 0) {
			errno = V_BADREAD;
			suword (&arg->retval,errno);
			goto ioctldone;
		}
		break;

	case V_PDWRITE:
		bufhead = geteblk();
		idsetblk (bufhead, B_WRITE, IDPDBLKNO, dev);
		if (copyin (mem, bufhead->b_un.b_addr,
		  idsect0[unit].pdinfo.bytes) != 0) {
			errno = V_BADWRITE;
			suword(&arg->retval, errno);
			goto ioctldone;
		}
		idstrategy(bufhead);
		iowait(bufhead);
		if (bufhead->b_flags & B_ERROR) {
			errno = V_BADWRITE;
			suword(&arg->retval, errno);
			goto ioctldone;
		}
		break;

	case V_GETSSZ:
		suword((int *)karg.memaddr, idsect0[unit].pdinfo.bytes);
		return(0);

	default:
		return(EIO);
	}

ioctldone:
	brelse(bufhead);
	return(0);
}


extern	dev_t	dumpdev;
extern	int	dumpbad;	/* DUMDEV was bad; parameters not set */
extern	dev_t	ddumpdev;	/* DUMPDEV for cdump to use */
extern	int	dumpunit;	/* which disk to dump on */
extern	int	dumppart;	/* which partition to dump on */
extern	int	dumpoff;	/* offset into dump partition */
extern	int	dumppartst;	/* partition starting block */
extern	int	dumpsize;	/* size of dump (swap) partition */
extern	int	dumplogicalst;	/* first usable block */

void
iddumpinit()
{
	int error = 0;

	ddumpdev = dumpdev;
	dumpunit = iddn(dumpdev);
	dumppart = idslice(dumpdev);

	/* open the device to make sure it is there */
	error = idopen(&dumpdev, 0, OTYP_LYR, (cred_t *)0);
	if (error == 0)
		error = idclose(dumpdev, 0, OTYP_LYR, (cred_t *)0);
	if (error) {
		cmn_err( CE_WARN,
			"\nopen failed dumdev %x: error %d\n", dumpdev, error);
		return;
	}

	/* check for empty partition */
	if ( idvtoc[dumpunit].v_part[dumppart].p_size == 0 ) {
		cmn_err( CE_WARN,
			"\nDUMPDEV disk %d: partition %d is zero length\n",
			dumpunit, dumppart);
		return;
	}

	/* check for read only partition */
	if ( (idvtoc[dumpunit].v_part[dumppart].p_flag & V_RONLY)==V_RONLY) {
		cmn_err( CE_WARN,
			"\nDUMPDEV disk %d: partition %d is marked read only\n",
			dumpunit, dumppart);
		return;
	}

	dumpsize = idvtoc[dumpunit].v_part[dumppart].p_size;
	dumppartst = idvtoc[dumpunit].v_part[dumppart].p_start;
	dumplogicalst =	idsect0[dumpunit].pdinfo.logicalst;
	dumpbad = 0;
}
