/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:cunix/edt.c	1.6"

#include <sys/types.h>
#include <sys/localtypes.h>
#include <stdio.h>
#include <a.out.h>
#include <sys/boothdr.h>
#include <sys/sys3b.h>
#include <sys/edt.h>
#include <sys/extbus.h>
#include <sys/error.h>
#include <sys/dproc.h>
#include <sys/gen.h>
#include <sys/ledt.h>
#include <sys/vtoc.h>
#include <sys/fcntl.h>
#include <sys/sbd.h>
#include <sys/cunix.h>

/*
 * This file contains all the code to do edt processing. Two edt tables
 * are managed, the local edt table which is the internal edt table from
 * sys3b and the extended edt for extended devices like SCSI. The extended
 * edt is written on the last 25 blocks of the boot device.
 */


/*
 * pointer to local edt structure.
 *
 */
struct edtsize {
	unsigned short ndev;
	unsigned short nsub;
} edtsize;
struct edtsize *local_edt;

int edt_count;
struct s3bconf	*sys3bconfig;		/* ==> malloc()'ed copy */


static void ldebusgen();
B_EDT *	findxedt();

static void xedtscan();
static void xalloc_s3bconf();


struct xs3bconf *xs3bconfstart = NULL;	/* pointer to extended configuration
                                         * table.
                                         */

int xmajnum = HTCMAJ;				/* First HA major number */

/* adjxmaj() allocates major numbers for  host adaptor */

boolean
adjxmaj(cnt)
register int cnt;			/* number of major's needed */
{
	if(CHKMAJ(xmajnum, cnt))

                /* major numbers allocated are within range */

		xmajnum -= cnt;
	else
		return(FALSE);

	return(TRUE);
}



/* marktc() mark target controller TC entry, assign major
 * number and adjust HA entry for this TC
 */

static 
void
marktc(hadp, tcdp, maj, maxlu, maxnumtc, maxnumlu, haslot)
register struct driver *hadp, *tcdp;		/* ha, tc driver pointers */
register int maj, maxlu;			/* major number, max logical
						 * units per this TC
register int maxnumtc, maxnumlu, haslot;	 * max target controllers per
						 * ha, ha's max logical units
						 * per TC
						 */
{

	tcdp->flag |= ISEBUS;			/* set extended bus flag */
	tcdp->maj[tcdp->nctl] = maj;		/* external major for this TC */

	if(++tcdp->nctl >= MAXCNTL){
		/* MAXCNTL exceed for this HA */

		error(ER87, haslot);
		exit(1);
	}


 	if((int)(hadp->ntc_lu += maxlu) > (int)(hadp->nctl*(maxnumtc - 1)*maxnumlu)){
		/* MAX LU's exceeded for this HA */
		/* maxnumtc's has 1 subtracted because the HA takes up
		   one TC slot so there is really 1 less TC than the number
		   of slots available. */ 

		error(ER88, hadp->name);
		exit(1);
	}
	edt_count++;
	return;
}

/* fndxbus() checks edt for extended bus type (indir_dev) */


 void
fndxbus(edtp, slot)
register struct edt *edtp;
int slot;
{
	struct driver *dp;

	if (edtp->indir_dev)
		/* Found HA two level device */

		if ((dp=searchdriver(edtp->dev_name))!=NULL && !(dp->flag & EXCLUDE))
			/*
			 * Driver exists in /boot and not EXCLUDED,
			 * therefor load and execute bus generator
			 * program for this HA.
			 */

			(void)ldebusgen(dp, slot, edtp->dev_name);	/* load and execute HA gen program */
}
	

static void
ldebusgen(dp, slot, haname)
register struct driver *dp;
register slot;
char *haname;
{

	extern address maxmem;
	int cnt;
	B_EDT *xedtp;
	

	if(maxmem < IMEG+MAINSTORE){
		error(ER79);	/* need a minimum of 1 meg to configure 
				 * HA device, TC under this HA will be
				 * ignored.
				 */
		return;
	}

	if ( !(xedtp = findxedt(slot))){
		fprintf(stderr,"No controllers under HA %s in slot %d configured\n",haname,slot);
		fprintf(stderr,"Extended EDT not up to date, run filledt\n");
		return;
	}

	/* check to see if major numbers assigned are within range */

	cnt = xedtp->max_tc;	/* number of majors assigned */

	if(cnt < 1 || cnt > 15){
			/* invalid TC count */
		error(ER83, cnt, slot); /* Invalid HA in slot X */
		exit(1);
	}else

		if(!adjxmaj(cnt)){
			/* major numbers are not within range */

			error(ER84, xmajnum, xmajnum-cnt+1, slot);
			exit(1);
		}


	xedtscan(xedtp, dp, slot);		/* scan HA edt to configure
						 * target controllers
						 */
}

/*
 * findxedt - load extended edt from boot block.
 *
 */

 B_EDT *
findxedt(hslot)
int hslot;
{
	static char *xedtstartp=NULL;
	long *xhead,*sanity;
	B_EDT *xedtp;
	int startsector;
	struct vtoc vtoc;
	struct pdinfo *pdinfo;
	struct pdsector pdsector;
	struct io_arg	args;
	int fd,i;

	if (!xedtstartp){
		if ((xedtstartp = (char *)malloc(BSIZE * XEDTSIZE)) == (char *)NULL){
			panic(stderr,"unable to malloc space for extended edt\n");
			exit(1);
		}
	if ((fd=open(raw_disk,O_RDONLY)) < 0){
		error(ER80,raw_disk);
		exit(1);
		}
	readpd(fd,&pdsector);
	pdinfo = &pdsector.pdinfo;

	readvtoc(fd,pdinfo,&vtoc);

	startsector = vtoc.v_part[7].p_start + vtoc.v_part[7].p_size - 25 - 1;

	args.sectst = pdinfo->logicalst + startsector;
	args.memaddr = (unsigned long)xedtstartp; 
	args.datasz = BSIZE * XEDTSIZE;
	if (ioctl(fd, V_PREAD, &args) < 0){
		panic(stderr,"unable to read xedt, ioctl failed\n");
		exit(1);
		}
	close(fd);
	}

	sanity = (long *)xedtstartp;
	xhead = (long *)xedtstartp + 1;
	if (*sanity == XEDTMAGIC)
		for(i=0; i< 15; i++)
			if (xhead[i] == (long)0)
				return(0);
			else{
				xedtp = (B_EDT *)(xedtstartp + xhead[i]);
 				if (xedtp->ha_slot == hslot){
					return(xedtp);
				}
			}
	
	return(0);
}



/* xedtscan scan HA extended edt and initialize 
 * TC driver entry
 */

 static void
xedtscan(xedt, hadp, haslot)
register B_EDT *xedt;
register struct driver *hadp;
register int haslot;
{
	register struct driver *tcdp;		/* pointer to TC driver entry */
	register B_TC *xtcp;			/* pointer to TC */
 	int  i, maxlu;
	unsigned int maj;			/* maj number of TC */
	char *tcname;			/* name of TC */

	i = 0;			/* loop counters */
	xtcp = xedt->tc;		/* xtcp pointer to TC array */

	/* scan HA edt and configure those devices that are equipped,
	 * a check is done to ensure we don't kill ourselves 
	 * by running off the end of the extended edt.
	 */

	while(i < (int)xedt->n_tcs ){

		if(xtcp->equip_stat){ /* found equipped TC */
			i++;
 			maj = xtcp->maj;	/* major number for this TC */
 			maxlu = xtcp->max_lu;	/* max number of lu's for this TC type */
			tcname = (char *)xtcp->name;
			if((tcdp=searchdriver(tcname)) != NULL){
 				marktc(hadp, tcdp, maj, maxlu, xedt->max_tc, xedt->max_lu, haslot);
				/* allocate extended config table entry 
			 	* for TC.
			 	*/
	
				xalloc_s3bconf(maj, tcname, haslot);

			}
			else 
				error(ER85,tcname,haslot);
					
		}

		xtcp = (B_TC *)((caddr_t)xtcp + xedt->tc_size); /* look at next TC */
	}

}
 
/* xalloc_s3bconf - build xedt config table (2 level bus devices). The 
 * xconfig() routine will copy the contents of this table to the 
 * the system sys3bconf table; The system sys3bconfig table is available
 * to users via sys3b(). 
 */

 static void
xalloc_s3bconf(maj, tcname, haslot)
register int maj;			/* TC external major number */
register char *tcname;				/* name of TC driver */
register int haslot;			/* HA slot TC resides on */
{

	struct xs3bconf *p;

	if(NULL == (p = (struct xs3bconf*)malloc(sizeof(struct xs3bconf))))
		panic("No memory for extended sys3bconfig structure");

	p->next = xs3bconfstart;
	p->maj = maj;
	p->haslot = haslot;
	strncpy(p->tcname, tcname, sizeof(p->tcname));
	xs3bconfstart = p;
}


/* xconfig - build sys3b config for extended bus devices */
 void
xconfig(p)
register struct xs3bconf *p;
{

	register struct s3bc *s3bc;
	register struct driver *dp;

	while(p != NULL){
		/*
		 * if this EDT entry was ignored, remember it so a warning is
		 * not re-issued when the absolute boot file is loaded
		 */
		if (ignoredt(p->tcname)){
			s3bc = &sys3bconfig->driver[sys3bconfig->count++];
			s3bc->timestamp = 0;
			strncpy(s3bc->name, p->tcname, sizeof(s3bc->name));
			s3bc->flag = S3BC_IGN;
			s3bc->board = 0;
		}else 

			/*
			 * if this TC was not ignored, then it should have been
			 * configured; if not, then don't include it in the sys3bconfig
			 * structure -- this will cause a warning to be re-issued when
			 * the absolute boot file is loaded
			 */
			if ((dp=searchdriver(p->tcname)) != NULL &&  (dp->flag & LOAD)){
				/*
				 * put this TC into the sys3bconf structure
				 */

				s3bc = &sys3bconfig->driver[sys3bconfig->count++];
				s3bc->timestamp = dp->timestamp;
				strncpy(s3bc->name, p->tcname, sizeof(s3bc->name));
 				s3bc->flag = (S3BC_DRV|S3BC_TCDRV);
				s3bc->board = p->maj;		/* major number of TC */
			}

		xs3bconfstart = p->next;	/* look at next entry */
		free((char *)p);
		p=xs3bconfstart;
	}
}

/*
 * initialize local edt table. data used by escan and edtscan.
 *
 */

 void
initedt()
{

	struct edtsize edtsize;
	int size;

	sys3b(S3BEDT,&edtsize,sizeof(struct edtsize));
	size = edtsize.ndev * sizeof(struct edt);
	size += edtsize.nsub * sizeof(struct subdevice) + sizeof(int);
	local_edt = (struct edtsize *)malloc(size);
	sys3b(S3BEDT,local_edt,size);
}

/*
 * check the number of occurences of a partcular device.
 *
 */

escan(opt_code)
register short int opt_code;
{

	struct edt *ebuf;
	register i,j,count;
	

	count = local_edt->ndev;
	ebuf = (struct edt *) (local_edt + 1);
	for(i=0,j=0; i<count;i++){
		if (ebuf->opt_code == opt_code)
			j++;
		ebuf++;
	}
	return(j);
}

/*
 * Edtscan(function)
 *
 * Search the EDT table, calling function() for each EDT entry.  
 *
 */
 void
edtscan(function)
register void (*function)();
{

	struct edt *ebuf;
	register i,count;

	count = local_edt->ndev;
	ebuf = (struct edt *) (local_edt + 1);
	for(i=0; i<count;i++){
		(*function)(ebuf,ebuf->opt_slot);
		ebuf++;
	}
}


/*
 * Config(edtp, lba, elb)
 *
 * Fill the sys3bconfig structure from the EDT.  
 *
 */
 static
 void
config(edtp,elb)
	register struct edt *edtp;
	int elb;
	{

	register struct s3bc *s3bc;
	register struct driver *dp;
	int lba=0;

#ifdef u3b15
	/*
	 * ignore IOA controlled devices
	 */
	if (edtp->ioa_edti != 0)
		return;
#endif

	/*
	 * if this EDT entry was ignored, remember it so a warning is
	 * not re-issued when the absolute boot file is loaded
	 */
	if (ignoredt(edtp->dev_name))
		{
		s3bc = &sys3bconfig->driver[sys3bconfig->count++];

		s3bc->timestamp = 0;
		strncpy(s3bc->name, edtp->dev_name, sizeof(s3bc->name));
#ifdef u3b2
		if (!strcmp("SBD", edtp->dev_name))
			s3bc->timestamp =  sys3b(S3BMEM,0,0);
#endif
		s3bc->flag = S3BC_IGN;
		s3bc->board = 0;
		return;
		}

	/*
	 * if this EDT entry was not ignored, then it should have been
	 * configured; if not, then don't include it in the sys3bconfig
	 * structure -- this will cause a warning to be re-issued when
	 * the absolute boot file is loaded
	 */
	if ((dp=searchdriver(edtp->dev_name)) == NULL || ! (dp->flag & LOAD))
		return;

	/*
	 * don't include LBE's at unsupported board addresses or any of the
	 * devices located on the unsupported LBE's -- this will cause a warning
	 * to be re-issued when the absolute boot file is loaded
	 */
	if (dp->flag & ISLBE)
		{
		if (elb != 14 && elb != 15)
			return;
		}
	else
		if (lba != 0)
			{
			register i;
			boolean found;

			if (lba != 14 && lba != 15)
				/* device on an unsupported LBE */
				return;

			/* make sure that this device is actually being configured */
			found = FALSE;
			for (i=0; i<(int)dp->nctl; ++i)
				found = found || (dp->maj[i] == LBEMAJ(lba,elb));

			if (! found)
				return;
			}

	/*
	 * put this EDT entry into the structure
	 */
	s3bc = &sys3bconfig->driver[sys3bconfig->count++];

	s3bc->timestamp = dp->timestamp;
	strncpy(s3bc->name, edtp->dev_name, sizeof(s3bc->name));
	s3bc->flag = S3BC_DRV;
	s3bc->board = (lba<<4) | elb;
	}


/* Alloc_sys3bconfig(count)
 *
 * Allocate the sys3bconfig structure and copy it to kernel data space
 */
 void
alloc_sys3bconfig(count)
	int count;
	{

	register struct driver *dp;
	register struct s3bc *s3bc;

	if (NULL == (sys3bconfig = (struct s3bconf *)malloc(sizeof(struct s3bconf) +
								count*sizeof(struct s3bc))))
		panic("No memory for sys3bconfig structure");

	/*
	 * populate the sys3bconfig structure with EDT entries
	 */
	sys3bconfig->count = 0;

	edtscan(config);

	if (xs3bconfstart != NULL)
		xconfig(xs3bconfstart);

	/*
	 * finish the initialization by adding the entries for the software
	 * drivers and configurable modules
	 */
	dp = driver;

	do	{
		if (! (dp->flag & LOAD))
			continue;

                if (dp->flag & INEDT || dp->flag & ISEBUS)
			continue;

		s3bc = &sys3bconfig->driver[sys3bconfig->count++];

		s3bc->timestamp = dp->timestamp;
		strncpy(s3bc->name, dp->name, sizeof(s3bc->name));
		s3bc->flag = (dp->opthdr->flag & NOTADRV)? S3BC_MOD : S3BC_DRV;
		s3bc->board = 0;
		}
		while (dp = dp->next);

	generate(G_DATA, "sys3bconfig", sizeof(struct s3bconf)+(sys3bconfig->count-1)*sizeof(struct s3bc), sys3bconfig);

	}


/*
 * Catch(edtp,elb)
 *
 * Catch the EXCLUDE entries in the EDT by checking each entry
 * in the EDT against ignoredt().
 */
 void
catch(edtp,elb)
	register struct edt *edtp;
	int elb;
	{
	int lba =0;

#ifdef u3b15
	/*
	 * ignore IOA controlled devices
	 */
	if (edtp->ioa_edti != 0)
		return;
#endif

	if (searchdriver(edtp->dev_name) == NULL)
		/*
		 * there was no driver found for this device
		 */
		{
		if (! ignoredt(edtp->dev_name))
			{
			if (lba == 0)
				/* Driver not found for <edtp->dev_name> device (board code <n>) */
				error(ER38, edtp->dev_name, elb);
			else
				/* Driver not found for <edtp->dev_name> device (LBE <n>, board code <n>) */
				error(ER39, edtp->dev_name, lba, elb);
			}
		}
	}


/*
 * Mark(edtp, lba, elb)
 *
 * Mark the driver for this edt entry if such a driver exists.  Lba and elb are:
 *
 *				lba	elb
 *				----	----
 *	non-LBE device		   0	3-15
 *	LBE device		3-15	1-15
 */
#ifdef u3b15
 static
 void
mark(edtp, lba, elb)
	register struct edt *edtp;
	int lba, elb;
	{

	register struct driver *dp;


	/*
	 * ignore IOA controlled devices
	 */
	if (edtp->ioa_edti != 0)
		return;

	/*
	 * count the total number of EDT entries; this will be needed later
	 * when the s3bconf structure is constructed
	 */
	++edt_count;

	if ((dp=searchdriver(edtp->dev_name)) == NULL)
		/*
		 * no driver for this device found; since the /etc/system
		 * file has not yet been processed, it is not known whether
		 * this situation is an error or whether this entry is to
		 * be ignored
		 */
		return;

	/*
	 * make sure LBE's are only found at local bus addresses 14 & 15;
	 * edtscan() insures that LBE's only exist on the local bus
	 */
	if (dp->flag & ISLBE)
		if (elb != 14 && elb != 15)
			{
			/* LBE ignored at board code <elb>; LBE must be at board code 14 or 15 */
			error(ER17, elb);
			return;
			}

	/*
	 * mark this driver
	 */
	dp->flag |= INEDT;

	if (lba == 0)
		dp->maj[dp->nctl] = elb;
	else
		if (lba == 14 || lba == 15)
			/*
			 * device is on a valid LBE
			 */
			{
			dp->maj[dp->nctl] = LBEMAJ(lba,elb);
			dp->sys_bits[dp->nctl] = (edtp->lb_baddr & PSYS) >> 27;
			}

	if (++dp->nctl >= MAXCNTL)
		/* MAXCNTL exceeded */
		error(ER2);
	}
#endif

#ifdef u3b2
 void
mark(edtp,elb)
	register struct edt *edtp;
	int  elb;
	{
	register struct driver *dp;


	/*
	 * count the total number of EDT entries; this will be needed later
	 * when the s3bconf structure is constructed
	 */
	++edt_count;

	if ((dp=searchdriver(edtp->dev_name)) == NULL)
		/*
		 * no driver for this device found; since the /etc/system
		 * file has not yet been processed, it is not known whether
		 * this situation is an error or whether this entry is to
		 * be ignored
		 */
		return;

	/*
	 * mark this driver
	 */
	dp->flag |= INEDT;

	dp->maj[dp->nctl] = elb;

	if (++dp->nctl >= MAXCNTL)
		/* MAXCNTL exceeded */
		error(ER2);
	}
#endif

