/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:io/sddrv.c	1.13"
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

void
sdstrategy(bp) 
struct buf *bp;
{
	if (bp->b_edev & 0x80)
		ifstrategy(bp);
	else
		idstrategy(bp);
	return;
}
