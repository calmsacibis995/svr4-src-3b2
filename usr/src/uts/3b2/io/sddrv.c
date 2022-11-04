/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/sddrv.c	1.10.1.6"
#include "sys/types.h"
#include "sys/sbd.h"
#include "sys/param.h"
#include "sys/conf.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/immu.h"
#include "sys/proc.h"
#include "sys/systm.h"
#include "sys/sysinfo.h"
#include "sys/errno.h"
#include "sys/signal.h"
#include "sys/debug.h"
#include "sys/user.h"
#include "sys/buf.h"
#include "sys/csr.h"
#include "sys/open.h"
#include "sys/iobuf.h"
#include "sys/conf.h"
#include "sys/var.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/cmn_err.h"
#include "sys/inline.h"
#include "sys/kmem.h"
#include "vm/page.h"
#include "sys/cred.h"
#include "sys/uio.h"
#include "sys/ddi.h"
#include "sys/vtoc.h"
#include "sys/id.h"
#include "sys/if.h"

extern int basyncnt;

#if defined(__STDC__)
extern int idsize(dev_t);
extern int idwrite(dev_t, struct uio *, struct cred *);
extern int idread(dev_t, struct uio *, struct cred *);
extern void idinit(void);
extern void idint();
extern int idopen(dev_t *,int,int,struct cred *);
extern int idclose(dev_t,int,int,struct cred *);
extern void idprint (dev_t,char *);
extern int idioctl(dev_t,int,struct io_arg *,int,struct cred *,int *);
extern void idstrategy(struct buf *);
extern int ifsize(dev_t);
extern int ifwrite(dev_t, struct uio *, struct cred *);
extern int ifread(dev_t, struct uio *, struct cred *);
extern void ifstart(void);
extern void ifinit(void);
extern void ifint(void);
extern int ifopen(dev_t *,int,int,struct cred *);
extern int ifclose(dev_t,int,int,struct cred *);
extern void ifprint (dev_t,char *);
extern int ifioctl(dev_t,int,struct io_arg *,int,struct cred *,int *);
extern void ifstrategy(struct buf *);
#else
extern int idsize();
extern int idwrite();
extern int idread();
extern void idinit();
extern void idint();
extern int idopen();
extern int idclose();
extern void idprint();
extern int idioctl();
extern void idstrategy();
extern int ifsize();
extern int ifwrite();
extern int ifread();
extern void ifstart();
extern void ifinit();
extern void ifint();
extern int ifopen();
extern int ifclose();
extern void ifprint();
extern int ifioctl();
extern void ifstrategy();
#endif

int sddevflag = 0;

void
sdstart()
{
	ifstart();
}

void
sdinit()
{
	idinit();
	ifinit();
}

sdopen(devp,flag,otyp,cr)
dev_t *devp;
int flag;
int otyp;
cred_t *cr;
{
	if (*devp & 0x80)
		return(ifopen(devp,flag,otyp,cr));
	else
		return(idopen(devp,flag,otyp,cr));
}

sdclose(dev,flag,otyp,cr)
dev_t dev;
int flag;
int otyp;
cred_t *cr;
{
	if (dev & 0x80)
		return(ifclose(dev,flag,otyp,cr));
	else
		return(idclose(dev,flag,otyp,cr));
}

int
sdioctl(dev,cmd,arg,flag,cr,rvalp)
dev_t dev;
int cmd;
struct io_arg *arg;
int flag;
cred_t *cr;
int *rvalp;
{
	if (dev & 0x80)
		return(ifioctl(dev,cmd,arg,flag,cr,rvalp));
	else
		return(idioctl(dev,cmd,arg,flag,cr,rvalp));
}

void
sdprint(dev,str)
char *str;
{
	if (dev & 0x80)
		ifprint(dev, str);
	else
		idprint(dev, str);
}

sdread(dev, uiop, cr)
dev_t dev;
uio_t *uiop;
cred_t *cr;
{
	if (dev & 0x80)
		return(ifread(dev, uiop, cr));
	else
		return(idread(dev, uiop, cr));
}

sdwrite(dev, uiop, cr)
dev_t dev;
uio_t *uiop;
cred_t *cr;
{
	if (dev & 0x80)
		return(ifwrite(dev, uiop, cr));
	else
		return(idwrite(dev, uiop, cr));
}

sdsize(dev)
dev_t dev;
{
	if (dev & 0x80)
		return(ifsize(dev));
	else
		return(idsize(dev));
}

/* ARGSUSED */
void
sdint(dev)
dev_t dev;
{

	idint();
	if (Rcsr & CSRDISK)
		ifint();
}

STATIC void
sdiodone(bp)
	register struct buf *bp;
{
	register struct buf *parentbp;

	if (bp->b_chain == (struct buf *)NULL) {
		biodone(bp);
		return;
	}
	bp->b_iodone = (int(*)()) NULL;

	parentbp = bp->b_chain;
	if (bp->b_error)
		parentbp->b_error = bp->b_error;
	else if (bp->b_oerror)
		parentbp->b_error = bp->b_oerror;
	if (bp->b_flags & B_ASYNC) {
		pageio_done(bp);
		parentbp->b_reqcnt--;
		if ((bp->b_flags & B_READ) == 0)
			basyncnt--;
		if (parentbp->b_reqcnt == 0) {
			biodone(parentbp);
			return;
		}
	} else {
		biodone(bp);
		return;
	}
}

#define MAXIOREQ 28  /* this is really arbitrary */

void
sdstrategy(bp) 
struct buf *bp;
{
	register struct page *pp;
	register int i, flags;
	struct buf *bufp[MAXIOREQ];
	int bytescnt, s, req, err;
	int blkincr;
	
	if (bp->b_bcount <= PAGESIZE) {
		if (bp->b_edev & 0x80)
			ifstrategy(bp);
		else
			idstrategy(bp);
		return;
	}
	blkincr = PAGESIZE / NBPSCTR;
	req = bp->b_bcount / PAGESIZE;
	if (bp->b_bcount % PAGESIZE)
		req++;
	bp->b_reqcnt = req;
	
	pp = bp->b_pages;
	if (pp == NULL) {
		ASSERT((int)bp->b_un.b_addr > MAINSTORE
			&& (int)bp->b_un.b_addr < UVBASE);
		(void)buf_breakup(sdstrategy, bp);
		return;
	}
	i = 0;
	do {
		/* Assumption: pages in the list was sorted. */
		if (i == (req - 1)) {	/* last I/O */
		    if (bp->b_bcount % PAGESIZE)
		    	bytescnt = bp->b_bcount % PAGESIZE;
		    else 
		    	bytescnt = PAGESIZE;
		} else {
		    bytescnt = PAGESIZE;
		}
		flags = (bp->b_flags & B_ASYNC) ? B_ASYNC : 0;
		flags |= (bp->b_flags & B_READ) ? B_READ : B_WRITE;
		bufp[i] = pageio_setup(pp, bytescnt, bp->b_vp, flags);
		bufp[i]->b_edev = bp->b_vp->v_rdev;
		bufp[i]->b_blkno = bp->b_blkno + (blkincr * i);
		bufp[i]->b_chain = bp;
		bufp[i]->b_iodone = (int(*)())sdiodone;
		bufp[i]->b_flags &= ~B_PAGEIO;
		bufp[i]->b_un.b_addr = (caddr_t)pfntokv(page_pptonum(pp));
		if (bp->b_edev & 0x80)
			ifstrategy(bufp[i]);
		else
			idstrategy(bufp[i]);
		i++;
		pp = pp->p_next;
	} while (pp != bp->b_pages);

	if (!(bp->b_flags & B_ASYNC)) {
		err = 0;
		for (i=0; i<req; i++) {
	    		if (err)
				(void) biowait(bufp[i]);
	    		else
				err = biowait(bufp[i]);
	    		pageio_done(bufp[i]);
	    		bp->b_reqcnt--;
		}
		/* arbitrary picking up one of the errors */
		if (err) 
			bp->b_error = err;
		ASSERT(bp->b_reqcnt == 0);
	    	ASSERT((bp->b_flags & B_DONE) == 0);
	    	s = spl6();
		biodone(bp);
	    	splx(s);
	}	
}

STATIC char sbuffer[512];
STATIC struct buf sbuf;
STATIC int sbuf_inuse = 0;

int
dksize(dev)
	dev_t dev;
{
	register int maj, error, max, min, middle;
	register char saved_error;
	int (*size)();
#if 0
	int old;
#endif

	if ((maj = getmajor(dev)) >= bdevcnt)
		return(-1);
	size = bdevsw[maj].d_size;
	if (size != nodev)
		return((*size)(dev));

	/* Avoid multiple access of the buffer header */
	while (sbuf_inuse)
		(void) sleep((caddr_t)&sbuf, PSWP+2);
	sbuf_inuse = 1;

	saved_error = u.u_error;
	u.u_error = 0;
	(void)(*bdevsw[maj].d_open)(dev, 0, OTYP_BLK);
	if (u.u_error) {
		(void)(*bdevsw[maj].d_close)(dev, 0, OTYP_BLK);
		u.u_error = saved_error;
		sbuf_inuse = 0;
		return(-1);
	}

#if 0
	if (*bdevsw[maj].d_flag & D_OLD) {
		old = 1;
		(void)(*bdevsw[maj].d_open)(dev, 0, OTYP_BLK);
		error = u.u_error;
	} else {
		old = 0;
		error = (*bdevsw[maj].d_open)(dev, 0, OTYP_BLK, cr);
	}
	if (error) {
		if (old)
			u.u_error = saved_error;
		return ENXIO;
	}
#endif
	bzero((caddr_t)&sbuf, sizeof(struct buf));
	sbuf.b_edev = dev;
	sbuf.b_bcount = NBPSCTR;
	sbuf.b_un.b_addr = &sbuffer[0];

	max = 8 * 1024 * 1024;
	min = 0;
	do {
		middle = (max + min) / 2;
		sbuf.b_blkno = middle;
		sbuf.b_flags = B_KERNBUF | B_BUSY | B_READ;
		sbuf.b_error = 0;
		(*bdevsw[maj].d_strategy)(&sbuf);
		error = biowait(&sbuf);
		if (error) 
			max = middle;
		else
			min = middle;
	} while (max != (min + 1));
	
	(void)(*bdevsw[maj].d_close)(dev, 0, OTYP_BLK);
	u.u_error = saved_error;
	sbuf_inuse = 0;
	wakeup((caddr_t)&sbuf);
	return (max - 1);
}
