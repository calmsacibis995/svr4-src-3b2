/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/if.c	1.15"
/* 
 *
 *		3B2 Computer UNIX Integral Floppy Disk Driver
 *
 */

#include "sys/types.h"
#include "sys/param.h"
#include "sys/sbd.h"
#include "sys/dma.h"
#include "sys/csr.h"
#include "sys/iu.h"
#include "sys/immu.h"
#include "sys/conf.h"
#include "sys/signal.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/user.h"
#include "sys/proc.h"
#include "sys/errno.h"
#include "sys/buf.h"
#include "sys/elog.h"
#include "sys/iobuf.h"
#include "sys/systm.h"
#include "sys/cmn_err.h"
#include "sys/vtoc.h"
#include "sys/open.h"
#include "sys/file.h"
#include "sys/tuneable.h"
#include "sys/cred.h"
#include "sys/uio.h"
#include "sys/sysmacros.h"	/* define before ddi.h */
#include "sys/ddi.h"
#include "sys/inline.h"
#include "sys/debug.h"
#include "vm/vm_hat.h"
#include "sys/if.h"

int ifdevflag = 0;

#if defined(__STDC__)
STATIC void ifbreakup(struct buf *);
STATIC void ifbufsave(struct buf *, struct ifccmd *);
STATIC void ifxfer(void);
STATIC void ifspinup(int);
STATIC void ifspindn(int);
STATIC void ifsetup(void);
STATIC void ifsetblk(struct buf*bufhead, u_char cmd, daddr_t blkno, dev_t dev);
STATIC void ifflush(void);
STATIC void ifrest(void);
STATIC void ifseek(void);
STATIC void ifscan(void);
extern void ifstrategy(struct buf *);
#else
STATIC void ifbreakup();
STATIC void ifbufsave();
STATIC void ifxfer();
STATIC void ifspinup();
STATIC void ifspindn();
STATIC void ifsetup();
STATIC void ifsetblk();
STATIC void ifflush();
STATIC void ifrest(void);
STATIC void ifseek();
STATIC void ifscan();
extern void ifstrategy();
#endif

#undef	IFMAXXFER		/* ...because if.h definition differs */
#define	IFMAXXFER	2	/* Transfer retries (hw does five) */

/*
 * Partitioning.
 */
struct	{
	daddr_t nblocks;	/* number of blocks in disk partition */
	int 	cyloff;		/* starting cylinder # of partition */
} if_sizes[8] = {
	990,   24,	/* partition 0 -	cyl 24-78 (root)      */
	810,   34,	/* partition 1 -	cyl 34-78 	      */
	612,   45,	/* partition 2 -	cyl 45-78 	      */
	414,   56,	/* partition 3 -	cyl 56-78 	      */
	216,   67,	/* partition 4 -	cyl 67-78 	      */
	1404,  1,	/* partition 5 -	cyl 1-78  (init)      */
	1422,  0,	/* partition 6 -	cyl 0-78  (full disk) */
	18,    0 	/* partition 7 -	cyl 0     (boot)      */
} ;

struct ifccmd ifccmd;

STATIC struct iobuf iftab;	/* drive and controller info */
STATIC struct iotime ifstat;	/* drive status info */

STATIC int ifcsrset;		/* Drive motor is enabled */
STATIC int ifslow;		/* Drive motor is not yet up to speed */

STATIC int ifspurint;		/* counter for spurious interrupts */
extern paddr_t if_addr[];	/* local bus addr of disk controller */



#define acts io_s1		/* space for driver save info */
#define cylin b_resid		/* bytes not transferred on error */
#define ccyl jrqsleep		/* process sleep counter on jrq full */


#define ifslice(x) (x&7)
#define ifformatdev(x) ((x>>3)&1)
#define ifnodev(x) ((x>>4)&1)

#define NULL 0
#define SET  1


extern int ifloppy;
#define IF ((struct ifdev *) &ifloppy)

extern int duart;
#define CONS ((struct duart *) &duart)

#define IFOQUIET 0x00
#define IFOWAIT  0x01
#define IFOGOOD 0x02
#define IFOBAD 0x03

STATIC int ifopenst;	/* flag for determining drive ready at open */
			/* with above defined flags	*/
STATIC int iftimeo;	/* ifspindn() timeout ID (NULL when no timeout) */
STATIC int ifisopen;	/* Drive open or awaiting end of I/O */
STATIC int ifclosed;	/* Drive closed */
STATIC int iflag;	/* FLAG FOR COMMAND INTERRUPT INTERPRETATION */
STATIC int ifotyp[OTYPCNT];
STATIC int ifisroot;	/* is this floppy the root device */
unsigned int ifstate;	/* used for formatting-I/O contention */


STATIC int ifskcnt;		/* JOB RETRY COUNTERS */
STATIC int ifxfercnt;
STATIC int iflstdcnt;


extern char u400;
STATIC int ifsidesw;

STATIC struct ifsave {
	caddr_t b_addr;
	daddr_t b_blkno;
	unsigned int b_bcount;
}ifsvaddr;

STATIC struct  {
	u_char buf[512];
}ifcache[2];

STATIC u_int ifcacheaddr;
STATIC struct iftrkfmat *fmat_buf;	
STATIC struct ifformat kifmat;
STATIC struct io_arg kifargs;
extern struct buf *ngeteblk();

/* XXX */
STATIC u_char tmptrkbuf[14336];

void
ifstart()
{
	unsigned int fmataddr;
	int szbuf;
#if 0
	int i;
	unsigned int noneed;
	int memchng;
#endif

	/*
	 * get smallest number of pages which
	 * contain the format buffer.
	 */

	szbuf = btoc(sizeof(struct iftrkfmat));

#if 0
	memchng = 2 * szbuf - 1;
	if (availrmem - memchng < tune.t_minarmem  ||
	   availsmem - memchng < tune.t_minasmem)
		return;
	/*
	 * Ask for physically contiguous pages with 
	 * with nosleep option.
	 */

	if((fmataddr = getcpages(memchng, 1)) == NULL) {
		return;
	}
#endif 
	fmataddr = (unsigned int)(&tmptrkbuf);

	/*
	 * Check if buffer crosses 64K boundary to
	 * work around DMA limitation
	 */

	if (((fmataddr&MSK64K) + sizeof(struct iftrkfmat)) > BND64K) {
		fmat_buf = (struct iftrkfmat *)(fmataddr + ctob(szbuf) - NBPC);
#if 0
		noneed = fmataddr;
#endif
	} else {
		fmat_buf = (struct iftrkfmat *)fmataddr;
#if 0
		noneed = fmataddr + ctob(szbuf);
#endif
	}

#if 0
	/*
	 * Free pages for which there is no need.
	 */

	for (i = 0; i < szbuf - 1; i++) {
		freepage((int)kvtopfn(noneed + ctob(i)));
	}
	availrmem -= szbuf;
	availsmem -= szbuf;
#endif
	return;
}

void
ifcopy(faddr, taddr, count)
unsigned int *faddr;
unsigned int *taddr;
unsigned int count;
{
	unsigned int *fptr;
	unsigned int *tptr;
	int i;

	tptr = taddr;
	fptr = faddr;
	for (i=0; i<(count/4); i++)
		*tptr++ = *fptr++;
}

STATIC u_char ifalive;
STATIC u_char ifnoscan;

/*
 * ifscan()
 *
 * Check for lost interrupts.
 */
STATIC void
ifscan()
{
	int	s;

	s = spl6();
	if (iftab.b_actf != NULL)
		if (ifalive == NULL)
			ifflush();
		else
			ifalive = NULL;
	splx(s);
	timeout(ifscan, (caddr_t)0, 10*HZ);
}

STATIC void
#ifdef __STDC__
ifsetblk(struct buf *bufhead, u_char cmd, daddr_t blkno, dev_t dev)
#else
ifsetblk(bufhead, cmd, blkno, dev)
struct buf *bufhead;
u_char cmd;
daddr_t blkno;
dev_t dev;
#endif
{
	clrbuf(bufhead);
	bufhead->b_flags |= cmd;
	bufhead->b_blkno = blkno;
	bufhead->b_edev = (dev|IFNODEV);
	bufhead->b_proc = 0x00;
	bufhead->b_flags &= ~B_DONE;
	if (cmd == B_WRITE)
		bufhead->b_bcount = 512;
}

/* FLOPPY - OPEN ROUTINE */

/* ARGSUSED */
ifopen(devp, flag, otyp, cr)
dev_t *devp;
int flag;
int otyp;
cred_t *cr;
{
	int iplsave;
	int error = 0;

	if (((getminor(*devp)) & 0x7f) >= 8) {
		return(ENXIO);
	}
	if (ifnoscan == SET) {
		ifnoscan = NULL;
		ifscan ();
	}

	iplsave = splhi();

	/* make sure the disk drive is ready */

	while (ifopenst != IFOQUIET){
		sleep((caddr_t)&ifopenst,PRIBIO);
	}
	if (ifisopen == NULL){
		ifopenst = IFOWAIT;
		CONS->scc_sopbc = F_LOCK;
		CONS->scc_sopbc = F_SEL;
		ifspinup(0);
		while (ifopenst == IFOWAIT){
			sleep ((caddr_t)&ifopenst,PRIBIO);
		}
		if (ifopenst != IFOGOOD){
			error = ENXIO;
		}
		ifopenst = IFOQUIET;
		wakeup((caddr_t)&ifopenst);
	}
	if (!error) {
		if((flag & FWRITE) && (IF->statcmd & IFWRPT))
			error = EROFS;
		else {
			ifisopen = SET;
			ifclosed = NULL;
			if (otyp == OTYP_LYR)
				++ifotyp[OTYP_LYR];
			else if (otyp < OTYPCNT)
				ifotyp[otyp] |= 1 << (*devp & 0x7f);
		}
	}
	if (error && ifisopen == NULL)
		ifspindn(0);
	splx(iplsave);
	return(error);
}

/* FLOPPY - CLOSE ROUTINE */

/* ARGSUSED */
ifclose(dev, flag, otyp, cr)
dev_t dev;
int flag;
int otyp;
cred_t *cr;
{
	register int	i;
	register int	s;

	s = spl6();
	if (otyp == OTYP_LYR)
		--ifotyp[OTYP_LYR];
	else if (otyp < OTYPCNT)
		ifotyp[otyp] &= ~(1 << (dev & 0x7f));
	if (!ifisroot) {
		for (i = 0; i < OTYPCNT && ifotyp[i] == 0; ++i)
			;
		if (i == OTYPCNT)
			ifclosed = SET;
	}
	if (iftab.b_actf == NULL)
		ifspindn(0);
	splx(s);
	return(0);
}

/* FLOPPY INITIALIZATION OF POINTERS UPON KERNEL REQUEST */
void
ifinit()
{
	ifopenst = IFOQUIET;
	iftimeo = NULL;
	ifslow = NULL;
	ifstate=IFIDLEF;

	iftab.io_addr = (paddr_t)&ifloppy;
	iftab.io_start = NULL;
	iftab.b_actf = NULL;
	iftab.b_actl = NULL;
	iftab.qcnt = NULL;
	iftab.b_forw = NULL;
	iftab.b_forw = NULL;
	ifisopen = NULL;
	ifclosed = SET;
	ifnoscan = SET;
	ifalive = SET;
	fmat_buf = NULL;
	/* assign physical address of temporary cache */
	ifcacheaddr = (unsigned int) vtop((caddr_t)&ifcache[0], 0);
	if (((ifcacheaddr&MSK64K)+0x200)>BND64K)
		ifcacheaddr = (unsigned int) vtop((caddr_t)&ifcache[1], 0);

}

void
ifstrategy(bp)
register struct buf *bp;
{
	register struct iobuf *dp;

	daddr_t lastblk;		/* last block in partition */
	int part;		/* partition number */
	int iplsave;		/* save interrupt priority */
	int ifbytecnt;

	part = ifslice(getminor(bp->b_edev));
	lastblk = if_sizes[part].nblocks;
 	bp->cylin = bp->b_blkno/(IFNUMSECT*IFNTRAC)+if_sizes[part].cyloff;
	ifbytecnt = bp->b_bcount;

	if ((ifformatdev(getminor(bp->b_edev)) == SET) || (ifnodev(getminor(bp->b_edev)) == SET)){
		lastblk = (IFNUMSECT * IFTRACKS);
		ifbytecnt = (IFBYTESCT*IFNUMSECT);
	}

	dp = &iftab;

	/* CHECK FOR PARTITION OVERRUN I.E. BLOCK OUT OF BOUNDS */
	if ((bp->b_blkno<0)||
	(bp->b_blkno>=lastblk)||
	(bp->b_blkno+(ifbytecnt/IFBYTESCT))>lastblk){
		if ((bp->b_blkno == lastblk) && (bp->b_flags & B_READ))
			bp->b_resid = bp->b_bcount;
		else{
			bp->b_flags |= B_ERROR;
			bp->b_error = ENXIO;
		}
		iodone(bp);	/* JOB TERMINATION */
		return;
	}


	iplsave = spl6();	/* save previous IPL */

	(void) drv_getparm(LBOLT, &bp->b_start);
	ifstat.io_cnt++;
	ifstat.io_bcnt += (bp->b_bcount + NBPSCTR-1) >> SCTRSHFT; /* inc disk block count */
	dp->qcnt++;

	bp->av_forw = NULL;		/* mark request as last on list */
	if (dp->b_actf == NULL){		/* if no request for drive */
		dp->b_actf = bp;	/* link to front of worklist */
		dp->b_actl = bp;
		dp->acts = (int)bp;
		ifalive = SET;
		ifspinup(0);
 	} else {				/* link to end of list */
 		register struct buf *ap, *cp;
 		if (((int)ifstat.io_cnt&0x0f) == 0)
 			dp->acts = (int) dp->b_actl;
 		for (ap = (struct buf*)dp->acts; (cp = ap->av_forw) != 0; ap = cp) {
 			int s1, s2;
 			if ((s1 = ap->cylin - bp->cylin) < 0)
 				s1 = -s1;
 			if ((s2 = ap->cylin - cp->cylin) < 0)
 				s2 = -s2;
 			if (s1 < s2)
 				break;
 		}
 		ap->av_forw = bp;
 		if ((bp->av_forw = cp) == NULL)
 			dp->b_actl = bp;
 		bp->av_back = ap;
	}
	/* RETURN TO ORIGINAL IPL */
	splx(iplsave);



}	

STATIC void
ifrest()	/* THIS FUNCTION IS SIGNIFICANT ONLY TO SEEK ERRORS */
{
	iflag = IFRESTORE;		/* INTERRUPT ON RESTORE FLAG */
	ifxfercnt = 0;
	IF->statcmd = (IFREST|IFSTEPRATE);	/* RESTORE HEADS TO TRACK 00 */
}

/*
 * ifspinup()
 *
 * Spin up the floppy. Waits two seconds for the
 * drive speed to stabilize.
 */
STATIC void
ifspinup(timed)
	int timed;
{
	register int	s;

	if (timed) {
		s = spl6();
		ifslow = NULL;
	}
	if (iftimeo) {
		untimeout(iftimeo);
		iftimeo = NULL;
	}
	if (ifcsrset) {
		if (!ifslow) {
			if (ifopenst == IFOWAIT)
				ifrest();
			else {
				ifsetup();
				ifseek();
			}
		}
	} else {
		Wcsr->s_flop = SET;
		ifslow = ifcsrset = SET;
		timeout(ifspinup, (caddr_t)1, 2 * HZ);
	}
	if (timed)
		splx(s);
}

/*
 * ifspindn()
 *
 * Spin down the drive. Waits two seconds for additional
 * I/O before actually turning off the motor.
 */
STATIC void
ifspindn(timed)
	int timed;
{
	register int	s;

	if (timed) {
		s = spl6();
		ifcsrset = iftimeo = NULL;
		Wcsr->c_flop = NULL;
		splx(s);
	} else {
		if (ifclosed) {
			ifisopen = NULL;
			CONS->scc_ropbc = F_LOCK;
			CONS->scc_ropbc = F_SEL;
		}
		if (ifcsrset && iftimeo == NULL)
			iftimeo = timeout(ifspindn, (caddr_t)1, 2 * HZ);
	}
}

/*
 * ifflush()
 *
 * Flush the first job on the queue.
 */
STATIC void
ifflush()
{
	register struct buf	*bp;
	register struct buf	*lp;

	iflag = IFNONACTIVE;
	ifstate &= ~(IFBUSYF|IFFMAT1);
	bp = iftab.b_actf;
	iftab.b_active = NULL;
	iftab.b_errcnt = NULL;
	iftab.b_actf = bp->av_forw;
	bp->b_resid = bp->b_bcount;
	bp->b_flags |= B_ERROR;
	iftab.qcnt--;
	if (bp == (struct buf *) iftab.acts)
		iftab.acts = (int) iftab.b_actf;
	if (iftab.b_actl == bp) {
		/*
		 * Should use av_back, but it is not set.
		 */
		if ((lp = iftab.b_actf) == 0)
			while (lp->av_forw)
				lp = lp->av_forw;
		iftab.b_actl = lp;
	}
	iodone(bp);
	cmn_err(CE_WARN, "\nfloppy disk timeout; request flushed\n");
	if (iftab.b_actf)
		ifspinup(0);
	else
		ifspindn(0);
}

/* FILL COMMAND BUFFER WITH INFORMATION FROM THE BUFFER HEADER */
		/* AND COMPUTE DISK ACCESS ADDRESS */
STATIC void
ifsetup()
{
	register struct ifccmd *cp;
	register struct buf *bp;
	register struct iobuf *dp;
	int bytes;
	int blkcnt;
	int partit;
	int cyl;
	int sid;

	dp = &iftab;
	bp = dp->b_actf;
	cp = &ifccmd;

	ifskcnt = NULL;	/* RESET FLAGS FOR RETRIES */
	ifxfercnt = NULL;
	iflstdcnt = NULL;


	if (bp == NULL)	/* VERIFY THERE IS A JOB TO DO */
		return;
	if (dp->b_active == NULL){
		(void) drv_getparm(LBOLT, &dp->io_start); /* TIME STAMP JOB */
		ifsvaddr.b_bcount = bp->b_bcount;	/* SAVE ADDRESS FOR 64K */
		ifsvaddr.b_blkno = bp->b_blkno;		/*      BOUNDS	        */
		ifsvaddr.b_addr = bp->b_un.b_addr;
	}

	dp->b_active = SET;		/* MARK DRIVE AS ACTIVE */

	if (ifformatdev(getminor(bp->b_edev)) == SET){ /* FORMAT / VERIFY ? */
		cp->trknum = ifsvaddr.b_blkno/(IFNUMSECT*IFNTRAC);
		ifsidesw = ((ifsvaddr.b_blkno%(IFNUMSECT*2))/IFNUMSECT)*2;
		cp->baddr = vtop(ifsvaddr.b_addr,bp->b_proc); 
		if (cp->baddr == NULL)
			cmn_err(CE_PANIC,"\nfloppy disk Bad address returned from VTOP\n");
		if ((bp->b_flags & B_READ) == B_READ) {
			cp->c_opc = (IFRDS|IFSLENGRP1|IFMSDELAY|ifsidesw);
			cp->sectnum = (ifsvaddr.b_blkno%IFNUMSECT)+1;
			cp->bcnt = IFBYTESCT;
			ifsvaddr.b_bcount -= IFBYTESCT;
			ifsvaddr.b_blkno++;
			ifsvaddr.b_addr += IFBYTESCT;
		}
		else {
			cp->c_opc = (IFWRTRK|IFMSDELAY|ifsidesw);
			ifsvaddr.b_bcount -= bp->b_bcount;
			cp->bcnt = bp->b_bcount;
		}
	} else {
	    	partit = ifslice(getminor(bp->b_edev));

		/* find start cylinder in partition */
		if (ifnodev(getminor(bp->b_edev)))
			cyl = 0x00;
		else
	    		cyl = if_sizes[partit].cyloff;	
		/* find cylinder offset in partition */
	    	bp->cylin = ifsvaddr.b_blkno/(IFNUMSECT*IFNTRAC)+cyl;
		/* compute sector offset into cylinder  */
	    	cp->sectnum = (ifsvaddr.b_blkno%IFNUMSECT)+1;
		/* compute side */
	    	ifsidesw = ((ifsvaddr.b_blkno%(IFNUMSECT*2))/IFNUMSECT)*2;	

		/* load command buffer */
	    	cp->trknum = bp->cylin;
	    	cp->c_bkcnt = blkcnt = 1;
	    	cp->bcnt = bytes = IFBYTESCT;
	    	cp->c_opc = ((bp->b_flags & B_READ) ? IFRDS : IFWTS)|IFSLENGRP1|IFMSDELAY;
	    	cp->c_opc |= ifsidesw; /* FOR HEAD SWITCH */
	    	cp->baddr = vtop(ifsvaddr.b_addr,bp->b_proc);
	    	if (cp->baddr==NULL) 
		    	cmn_err(CE_PANIC,"\nfloppy disk: Bad address returned by VTOP\n");
		/* crossing 128K-byte boundary */
		sid = secnum((int)ifsvaddr.b_addr);
		if ( (sid == 2) || (sid == 3)  )  {   /* if user address */
			if ((((long)ifsvaddr.b_addr&MSK_IDXSEG)+IFBYTESCT) 
			> (MSK_IDXSEG+1))  {
				ifbufsave(bp,cp);
			}
		}
			
		/* crossing 64K-byte boundary or partial sector transfer */
		if ((((cp->baddr&MSK64K) + IFBYTESCT) > BND64K) 
		|| (ifsvaddr.b_bcount < IFBYTESCT)) {
			ifbufsave(bp,cp);
		}

		/* keep track of byte count, and blk and mem addresses  */
	    	ifsvaddr.b_blkno += blkcnt;
	    	if (cp->baddr != ifcacheaddr) {
		    	ifsvaddr.b_addr += bytes;
			ifsvaddr.b_bcount -= bytes;	
		}
	}
}   

/* Routine for coping the user buffer to a driver buffer          */
/* Used for 64K-byte bounds, 128K-byte bounds and partial sectors */

STATIC void
ifbufsave(bufhead,cmdp)
struct buf *bufhead;
struct ifccmd *cmdp;
{
	register u_char *if_dmem, *if_umem;
	register unsigned int *zp;
	int i, bytes;
	
	bytes = cmdp->bcnt;
	if ((bufhead->b_flags&B_READ) != B_READ) {
		if (ifsvaddr.b_bcount<IFBYTESCT) {
			bytes = ifsvaddr.b_bcount;
			zp = (unsigned int *) ifcacheaddr;
			for(i=0; i<IFBYTESCT/4; i++)
				*zp++ = 0x00000000;
		}
		if_dmem = (u_char *) ifcacheaddr;
		if_umem = (u_char *) cmdp->baddr;
		for(i=0; i<bytes; i++)
			*if_dmem++ = *if_umem++;
	} 
	cmdp->baddr = ifcacheaddr;
}


STATIC void
ifxfer()	/* DATA TRANSFER IS IMPLEMENTED */
{

	register struct ifccmd *cp;
	u_char cmd;

	ifxfercnt++;
	cp = &ifccmd;
	iflag = IFXFER;		/* INTERRUPT ON TRANSFER FLAG */
	ifstate |= IFBUSYF;
	if ((ifstate & IFFMAT0) == IFFMAT0)
		ifstate |= IFFMAT1;

	if ((cp->c_opc & IFCOPC) == IFWTS || (cp->c_opc & IFCOPC) == IFWRTRK)
		cmd = RDMA;	/* SET DIRECTION FOR DMA */
	else
		cmd = WDMA;

	/* INITIALIZE DMA FOR TRANSFER */
	dma_access(CH1IFL,cp->baddr,cp->bcnt,SNGLMOD,cmd);
	if ((cp->c_opc & IFCOPC) != IFWRTRK)
		IF->sector = cp->sectnum;	/* LOAD CONTROLLER REGS */
	IF->statcmd = cp->c_opc;	/* WITH SECTOR NO. AND CMD */

}

/* ROUTINE FOR SEEKING TO DESIRED TRACK */
STATIC void
ifseek()
{
	register struct ifccmd *cp;

	cp = &ifccmd;

	ifskcnt++;
	if (IF->track == cp->trknum){	/* CHECK FOR BEING ON TRACK */
		ifxfer();
		return;
	}
	iflag = IFSEEKATT;		/* INTERRUPT ON SEEK FLAG */
	IF->data = cp->trknum;		/* LOAD DESTINATION TRACK */
	if ((cp->c_opc & IFCOPC) == IFWRTRK)
		IF->statcmd = IFSEEK | IFSTEPRATE;
	else
		IF->statcmd = IFSEEK | IFSTEPRATE | IFVERIFY;
}


/* INTERRUPT HANDLER - STATUS - COMMAND INFORMATION INTERPRETER */
void
ifint()
{
	register struct buf *bp;
	register struct iobuf *dp;
	register struct ifccmd *cp;
	u_char dstat;
	register u_char *if_dmem, *if_umem;
	unsigned int i;
	int bytes;
	time_t mylbolt;

	dstat = IF->statcmd;

	if (iflag == IFNONACTIVE) {
		++ifspurint;
		return;
	}

	ifalive = SET;

	if (ifopenst == IFOWAIT) {
		iflag = IFNONACTIVE;
		ifopenst = (dstat & (IFNRDY | IFSKERR)) == 0 ? IFOGOOD : IFOBAD;
		wakeup((caddr_t)&ifopenst);
		return;
	}

	dp = &iftab;
	if ((bp = dp->b_actf) == NULL)
		return;		/* ...should never happen */
	cp = &ifccmd;

	if (dstat & IFNRDY)
		goto diskerr;

	switch (iflag) {
	case IFRESTORE:
		iflag = IFNONACTIVE;
		iutime(12, ifseek);
		return;
	case IFSEEKATT:
		iflag = IFNONACTIVE;
		if ((dstat & IFSKERR) == 0)
			if ((cp->c_opc & IFCOPC) == IFWRTRK)
				iutime(12, ifxfer);
			else
				ifxfer();
		else if (ifskcnt > IFMAXSEEK)
			goto diskerr;
		else
			ifrest();
		return;
	case IFXFER:
		iflag = IFSETTLE;
		ifstate &= ~(IFBUSYF | IFFMAT1);
		if ((cp->c_opc & IFCOPC) == IFWTS)
			iutime(1, ifint);
		else
			ifint();
		return;
	case IFSETTLE:
		iflag = IFNONACTIVE;
		if (dstat & IFWRPT)
			goto diskerr;
		
		if ((dstat & IFCRCERR) && ifxfercnt <= IFMAXXFER) {
			ifxfer();
			return;
		}

		if (dstat & (IFCRCERR | IFRECNF)) {
			if (ifskcnt > IFMAXSEEK)
				goto diskerr;
			else
				ifrest();
			return;
		}

		if (dstat & IFLSTDATA) {
			iflstdcnt++;
			if (iflstdcnt <= IFMAXLSTD){
				ifxfer();
				return;
			}
			iflstdcnt = NULL;
			goto diskerr;
		}
		goto goodend;
		break;
	default:
		cmn_err(CE_PANIC, "ifint");
	}

diskerr:
	cmn_err(CE_NOTE,"\nFloppy Access Error: Consult the Error Message Section");
	cmn_err(CE_CONT,"of the System Administration Utilities Guide");
	cmn_err(CE_CONT,"\n");
	bp->b_flags |= B_ERROR;
	bp->b_error |= EIO;

goodend:
	/* if the data is in the temporary cache */
	if (cp->baddr == ifcacheaddr) {
		bytes = IFBYTESCT;
		if (ifsvaddr.b_bcount < IFBYTESCT)
			bytes = ifsvaddr.b_bcount;
		/* if read, copy out to user */
		if ((bp->b_flags&B_READ) == B_READ) {
			if_dmem = (u_char *) ifcacheaddr;
			if_umem = (u_char *) vtop(ifsvaddr.b_addr,bp->b_proc);
			for(i=0;i<bytes;i++)
				*if_umem++ = *if_dmem++;
		}
		/* update pointer to user address space */
		ifsvaddr.b_addr += bytes;
		ifsvaddr.b_bcount -= bytes;
	}

	/* if no errors and more to do, then go again */
	if (((bp->b_flags & B_ERROR)==0) && (ifsvaddr.b_bcount != 0)){
		ifsetup();
		ifseek();
		return;
	}

	dp->b_active = NULL;
	dp->b_errcnt = NULL;
	dp->b_actf = bp->av_forw;
	bp->b_resid = NULL;
	dp->qcnt--;

 	if (bp == (struct buf *) dp->acts)
 		dp->acts = (int) dp->b_actf;
	if (dp->b_actl == bp)
		dp->b_actl = NULL;

	(void) drv_getparm(LBOLT, &mylbolt);
	ifstat.io_resp += mylbolt - bp->b_start;
	ifstat.io_act += mylbolt - dp->io_start;
	iodone(bp);

	if (dp->b_actf != NULL){
		ifsetup();
		ifseek();
		return;
	}
	ifspindn(0);
}

STATIC void
ifbreakup(bp)
register struct buf *bp;
{
	dma_pageio(ifstrategy, bp);
}

/* READ DEVICE ROUTINE */

/* ARGSUSED */
ifread(dev, uiop, cr)
dev_t dev;
uio_t *uiop;
cred_t *cr;
{
	return(physiock(ifbreakup, NULL, dev, B_READ,
	    if_sizes[getminor(dev)&07].nblocks, uiop));
}

/* WRITE DEVICE ROUTINE */

/* ARGSUSED */
ifwrite(dev, uiop, cr)
dev_t dev;
uio_t *uiop;
cred_t *cr;
{
	return(physiock(ifbreakup, NULL, dev, B_WRITE,
	    if_sizes[getminor(dev)&07].nblocks, uiop));
}

/* LOCAL PRINT ROUTINE */

void
ifprint(dev,str)
dev_t dev;
char *str;
{
	cmn_err(CE_NOTE,"%s on floppy drive, slice %d\n", str, dev&7);
}

int
ifsize(dev)
	dev_t dev;
{
	register int part;

	part = ifslice(getminor(dev));
	return (if_sizes[part].nblocks);
}

ifidle()
{
	if (iftab.b_actf != NULL)
		return(1);
	return(0);
}

/* ARGSUSED */
int
ifioctl(dev,cmd,arg,mode,cr,rvalp)
dev_t dev;
struct io_arg *arg;
int cmd, mode;
cred_t *cr;
int *rvalp;
{
	proc_t *p;
	int error = 0;
	
	switch(cmd){
	case IFBCHECK:{

		struct ifformat ifmat;
		paddr_t ifbaddr;

		if (drv_getparm(UPROCP, (ulong *)&p) < 0) {
			error = EINVAL;
			break;
		}
		if (copyin((caddr_t)arg, (caddr_t)&ifmat, sizeof(struct ifformat))) {
			error = EFAULT;
			break;
		}
		ifbaddr = vtop(ifmat.data, p);
		if (ifbaddr == 0){
			cmn_err(CE_WARN,"\nfloppy disk: Bad address returned from VTOP\n");
			error = EFAULT;
			break;
		}
		if (((ifbaddr & MSK64K)+ifmat.size) > BND64K){
			error = EFAULT;
			break;
		}
		break;
	}
	case IFFORMAT:{

		register struct buf *bp;
		struct iftrkfmat *trkpt;
		int bpbcount;
		caddr_t bpbaddr;

		if (drv_getparm(UPROCP, (ulong *)&p) < 0) {
			error = EINVAL;
			break;
		}
		trkpt = (struct iftrkfmat *)arg;
		if (copyin((caddr_t)trkpt, (caddr_t)fmat_buf, sizeof(struct iftrkfmat)) != 0) {
			error = EFAULT;
			break;
		}
		bp = geteblk();
		bp->b_error = 0;
		bp->b_edev = (dev | IFPTN);
		bpbcount = bp->b_bcount;
		bpbaddr= bp->b_un.b_addr;
		bp->b_bcount = sizeof(struct iftrkfmat);
		bp->b_proc = p;
		bp->b_un.b_addr =  ((caddr_t)fmat_buf);
		bp->b_flags = (B_BUSY | B_WRITE);
		bp->b_blkno = ((fmat_buf->dsksct[0].TRACK*(IFNUMSECT*2))+(fmat_buf->dsksct[0].SIDE*IFNUMSECT));
		ifstate |= IFFMAT0;
		ifstrategy(bp);
		iowait(bp);
		bp->b_bcount = bpbcount;
		bp->b_un.b_addr = bpbaddr;
		brelse(bp);
		ifstate &= ~IFFMAT0;
		break;
	}
	case IFCONFIRM:{
		register struct buf *bp;
		struct iftrkfmat *trkpt;
		struct ifformat *argpt;
		int bpbcount;
		caddr_t bpbaddr;

		if (drv_getparm(UPROCP, (ulong *)&p) < 0) {
			error = EINVAL;
			break;
		}
		argpt = (struct ifformat *)arg;
		if (copyin((caddr_t)argpt, (caddr_t)&kifmat, sizeof(struct ifformat))) {
			error = EFAULT;
			break;
		}
		/* LINTED */
		trkpt = (struct iftrkfmat *)(kifmat.data);
		bp = geteblk();
		bp->b_error = 0;
		bp->b_flags = (B_BUSY | B_READ);
		bp->b_edev = (dev | IFPTN);
		bpbcount = bp->b_bcount;
		bpbaddr = bp->b_un.b_addr;
		bp->b_bcount = (IFNUMSECT*IFBYTESCT);
		bp->b_proc = p;
		bp->b_un.b_addr = ((caddr_t) fmat_buf);
		bp->b_blkno = ((kifmat.iftrack*(IFNUMSECT*2))+(kifmat.ifside*IFNUMSECT));
		ifstrategy(bp);
		iowait(bp);
		bp->b_bcount = bpbcount;
		bp->b_un.b_addr = bpbaddr;
		brelse(bp);
		if (copyout((caddr_t)fmat_buf, (caddr_t)trkpt, sizeof (struct iftrkfmat)) != 0) {
			error = EFAULT;
			break;
		}
		break;
	}
	case V_PREAD:{
		struct io_arg *ifargs;
		struct buf *geteblk();
		struct buf *bufhead;
		int errno, xfersz;
		daddr_t block;
		unsigned int mem, count; 

		ifargs = (struct io_arg *)arg;
		if (copyin((caddr_t)ifargs, (caddr_t)&kifargs, sizeof(struct io_arg))) {
			error = EFAULT;
			break;
		}
		block = (daddr_t)kifargs.sectst;
		mem = kifargs.memaddr;
		count = kifargs.datasz;
		bufhead = geteblk();
		while (count) 	{
			ifsetblk (bufhead, B_READ, block, dev);
			ifstrategy(bufhead);
			iowait(bufhead);
			if (bufhead->b_flags & B_ERROR)	{
				errno = V_BADREAD;
				suword(&ifargs->retval,errno);
				goto preaddone;
			}
			xfersz = min(count,bufhead->b_bcount);
			if (copyout(bufhead->b_un.b_addr, (caddr_t)mem, xfersz) != 0)	{
				errno = V_BADREAD;
				suword(&ifargs->retval,errno);
				goto preaddone;
			}
			block+=2;
			count -= xfersz;
			mem += xfersz;
		}
preaddone:
		bufhead->b_bcount = NBPSCTR;
		brelse(bufhead);
		break;
	}
	case V_PWRITE:{
		struct io_arg *ifargs;
		struct buf *geteblk();
		struct buf *bufhead;
		int errno, xfersz;
		daddr_t block;
		unsigned int mem, count; 

		if ((error = drv_priv(cr)) != 0)
			break;
		ifargs = (struct io_arg *)arg;
		if (copyin((caddr_t)ifargs, (caddr_t)&kifargs, sizeof(struct io_arg))) {
			error = EFAULT;
			break;
		}
		block = kifargs.sectst;
		mem = kifargs.memaddr;
		count = kifargs.datasz;
		bufhead = geteblk();
		while (count) {
			ifsetblk(bufhead,B_WRITE,block,dev);
			xfersz = min(count,bufhead->b_bcount);
			if (copyin((caddr_t)mem,bufhead->b_un.b_addr,xfersz) != 0) {
				errno = V_BADWRITE;
				suword(&ifargs->retval,errno);
				goto pwritedone;
			}
			ifstrategy(bufhead);
			iowait(bufhead);
			if (bufhead->b_flags & B_ERROR)	{
				errno = V_BADWRITE;
				suword(&ifargs->retval,errno);
				goto pwritedone;
			}
			block +=1;
			count -= xfersz;
			mem += xfersz;
		}
pwritedone:
		bufhead->b_bcount = NBPSCTR;
		brelse(bufhead);
		break;
	}
	case V_PDREAD:{
		struct io_arg *ifargs;
		struct buf *geteblk();
		struct buf *bufhead;
		int errno;

		ifargs = (struct io_arg *)arg;
		if (copyin((caddr_t)ifargs, (caddr_t)&kifargs, sizeof(struct io_arg))) {
			error = EFAULT;
			break;
		}
		bufhead = geteblk();
		ifsetblk (bufhead, B_READ, IFPDBLKNO, dev);
		ifstrategy(bufhead);
		iowait(bufhead);
		if (bufhead->b_flags & B_ERROR)	{
			errno = V_BADREAD;
			suword(&ifargs->retval,errno);
			goto pdrddone;
		}
		if (copyout(bufhead->b_un.b_addr, (caddr_t)kifargs.memaddr, IFBYTESCT)  != 0)	{
			errno = V_BADREAD;
			suword(&ifargs->retval,errno);
			goto pdrddone;
		}
pdrddone:
		bufhead->b_bcount = NBPSCTR;
		brelse(bufhead);
		break;
	}
	case V_PDWRITE:{
		struct io_arg *ifargs;
		struct buf *geteblk();
		struct buf *bufhead;
		int errno;

		if ((error = drv_priv(cr)) != 0)
			break;
		ifargs = (struct io_arg *)arg;
		if (copyin((caddr_t)ifargs, (caddr_t)&kifargs, sizeof(struct io_arg))) {
			error = EFAULT;
			break;
		}
		bufhead = geteblk();
		ifsetblk(bufhead,B_WRITE,IFPDBLKNO,dev);
		if (copyin((caddr_t)kifargs.memaddr,bufhead->b_un.b_addr,IFBYTESCT) != 0) {
			errno = V_BADWRITE;
			suword(&ifargs->retval,errno);
			goto pdwrtdone;
		}
		ifstrategy(bufhead);
		iowait(bufhead);
		if (bufhead->b_flags & B_ERROR)	{
			errno = V_BADWRITE;
			suword(&ifargs->retval,errno);
			goto pdwrtdone;
		}
pdwrtdone:
		bufhead->b_bcount = NBPSCTR;
		brelse(bufhead);
		break;
	}
	default:
		error = EIO;
		break;
	}
	return(error);
}
