/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:cunix/machdep.c	1.9"

#include <sys/types.h>
#include <sys/localtypes.h>
#include <stdio.h>
#include <a.out.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/boothdr.h>
#include <sys/sysmacros.h>
#include <sys/sys3b.h>
#include <sys/edt.h>
#include <sys/extbus.h>
#include <sys/error.h>
#include <sys/dproc.h>
#include <sys/machdep.h>
#include <sys/gen.h>
#include <sys/off.h>
#include <sys/ledt.h>
#include <sys/vtoc.h>
#include <sys/fcntl.h>
#include <sys/sbd.h>
#include <sys/iobd.h>
#include <sys/cunix.h>
#include <vm/bootconf.h>


dev_t		 mirrordev[2] = { (dev_t) NODEV, (dev_t) NODEV };
dev_t		 rootdev = (dev_t)NODEV;
dev_t		 dumpdev = (dev_t) NODEV;
dev_t		 swapdev = (dev_t) NODEV;	/* only used if swaping to dev */
struct bootobj	 swapfile;

int	swapdone = -1;		/* swap has not been set up yet*/
int	VTOC_major = -1;	/* board slot of boot device (major number) */
int	VTOC_minor = -1;	/* device number on controller */
int	VTOC_root = -1;		/* partition number */
int	VTOC_swap = -1;		/* partition number */
int	VTOC_nswap = -1;	/* partition size */


char		*MAJOR;			/* real address */
char		*MINOR;			/* real address */

int	      (**io_init)();		/* ==> malloc()'ed copy */
int	      (**next_init)();		/* ==> next entry in *io_init[] */

int	      (**io_start)();		/* ==> malloc()'ed copy */
int	      (**next_start)();		/* ==> next entry in *io_start[] */

int	      (**pwr_clr)();		/* ==> malloc()'ed copy */
int	      (**next_pwrclr)();	/* ==> next entry in *pwr_clr[] */

int	      (**io_poll)();		/* ==> malloc()'ed copy */
int	      (**next_poll)();		/* ==> next entry in *io_poll[] */

int	      (**io_halt)();		/* ==> malloc()'ed copy */
int	      (**next_halt)();		/* ==> next entry in *io_halt[] */



/*
 * Return the size of memory.
 */

 address
sizemem()
{
	return(sys3b(S3BMEM,0,0) + MAINSTORE);
}


/*
 * Build_io_subsys()
 *
 * This routine will generate the UNIX kernel's interface to the I/O
 * subsystem.  The following things will be generated here:
 *
 *	MAJOR and MINOR translate tables
 *	cdevcnt, and the initial cdevsw table
 *	bdevcnt, and the initial bdevsw table
 *	the xx_addr[] array if required
 *	interrupt routines and PCBs
 *
 *	rootdev
 *	dumpdev
 *	mirrordev
 *	swapfile
 *
 *	space for the io_init[], io_start[], pwr_clr[], io_poll[],
 *	and io_halt[] arrays
 */

 void
build_io_subsys()
	{

	register struct driver *dp;
	register struct master *mp;
	register i, j;
	int number_drivers =0;		/* total number of drivers/modules to be loaded */
	int nC, order;
	int rcount, xcount, icount;	/* counters to generate interrupt unit numbers */
	int count;
	int vector, vec[MAXCNTL];
	char interrupt_name[7];		/* assembler interrupt routine: int?? or lbe??? */
	char handler_name[9];		/* C language interrupt routine: xxxx?int */
	char pcb_name[7];		/* kernel PCB name: kpcb?? */
	int (**LBE)();			/* if LBE's exist, we need to generate "LBE[2][256]" */
	static char hex[] ={ "0123456789ABCDEF" };

#ifdef u3b15
	struct mmuseg xx_addr[MAXCNTL+1];
	static struct mmuseg mmuseg ={1, 0, KRWE|UNONE, NCPS-1, 0, 0};
#endif

#ifdef u3b2
#define	KVIOBASE	0x60000		/* virtual origin of I/O board 1 */

	paddr_t xx_addr[MAXCNTL+1];
#endif

	/*
	 * call generate to allocate MAJOR, MINOR, cdevcnt, cdevsw[],
	 * bdevcnt, and bdevsw[]
	 */
	generate(G_IOSYS);

#ifdef u3b15
	if ((dp=searchdriver("LBE")) != NULL && dp->flag & LOAD)
		{
		generate(G_UDATA, "LBE", 2*256*sizeof(*LBE));
		LBE = (int(**)()) REAL(Xsym_name("LBE")->value, data_locctr);
		}
#endif

	/*
	 * run through the driver linked-list, and initialize MAJOR, MINOR, and
	 * generate interrupt routines and PCBs
	 */
	dp = driver;

	do	{
		if (! (dp->flag & LOAD))
			continue;

		mp = dp->opthdr;

		++number_drivers;

		/*
		 * if xx_addr[] array is needed, build it
		 */
		if (dp->flag & INEDT && mp->flag & REQADDR)
			{
			char name[10];

			for (i=0; i<(int)dp->nctl; ++i)
				{
#ifdef u3b15
				xx_addr[i] = mmuseg;
				xx_addr[i].base = (dp->maj[i] & 0x0F) * NCPS;

				if (ONLBE(dp->maj[i]))
					xx_addr[i].sys = dp->sys_bits[i];
#endif
#ifdef u3b2
#ifndef FALCON
				xx_addr[i] = (paddr_t)(KVIOBASE + (dp->maj[i]-1)*17*NBPS);
#else
           			xx_addr[i] = (paddr_t) IO_ADDR(dp->maj[i]);
#endif
#endif
				}


			/* xx_addr[] must be terminated by NULL for sysdef(1M) */
			*((int**)(&xx_addr[dp->nctl])) = NULL;

			strcat(strcpy(name,mp->prefix), "_addr");

			generate(G_DATA, name, (dp->nctl+1)*sizeof(xx_addr[0]), xx_addr);
			}

		if ((mp->flag & NOTADRV) || ((mp->flag & (CHAR | BLOCK | FUNMOD | FUNDRV)) == FUNMOD))
			/*
			 * nothing else to do if not a driver
			 */
			continue;

		/*
		 * set MAJOR[] and MINOR[] appropriately
		 */
		if (mp->flag & SOFT)
			/*
			 * if a software driver, MAJOR set here
			 */
			MAJOR[mp->soft] = dp->int_major;
		else
			/*
			 * set MAJOR and MINOR for hardware drivers
			 */
                        if (dp->flag & (INEDT|ISEBUS) && mp->flag & (BLOCK|CHAR|FUNDRV))

				for (i=0; i<(int)dp->nctl; ++i)
					{
					MAJOR[ dp->maj[i] ] = dp->int_major;
					MINOR[ dp->maj[i] ] = i * mp->ndev;
					}

		/*
		 * generate interrupt routines and PCBs
		 */
		if (mp->nvec != 0)
			{

			for (i=0; i<(int)dp->nctl; ++i)
				vec[i] = (dp->maj[i] & 0x0F) * 16;

			/*
			 * set nC to the number of controllers for which
			 * interrupt routines are to be generated; this
			 * is done to enable the pseudo software drivers to
			 * be treated the same as a normal peripheral board
			 */
			if (mp->flag & SOFT || dp->flag & ISEBUS)
				{
				if (mp->vec != 0)
					{
					nC = 1;
					vec[0] = mp->vec;
					order = XR;
					}
				else
					nC = 0;

#ifdef u3b15
				if (0 == strcmp("CONSOLE",dp->name))
					/* the CC console driver vectors are ordered RX */
					order = RX;
#endif
				}
			else
				{
				nC = dp->nctl;
				order = XR;
				}

			icount = rcount = xcount = 0;

			/*
			 * for each controller ...
			 */
			for (i=0; i<nC; ++i)
				{
				boolean on_lbe;	/* TRUE if device on a LBE */
				int lbe_unit;	/* if on_lbe, then lbe_unit = 0 or 1 */

				/*
				 * if device is on a LBE, then determine the unit number
				 */
				if (ONLBE(dp->maj[i]))
					{
					register struct driver *p = searchdriver("LBE");
					unsigned char lba;

					on_lbe = TRUE;
					lba = LBELBA((int)dp->maj[i]);

					lbe_unit = 0;
					while (lba != p->maj[lbe_unit])
						++lbe_unit;
					}
				else
					on_lbe = FALSE;

				/*
				 * for each interrupt vector
				 */
				for (j=0; j<(int)mp->nvec; ++j)
					{
					vector = vec[i] + j;

					if (on_lbe)
						{
						strcpy(interrupt_name, "lbe???");
						interrupt_name[5] = "01"[lbe_unit];
						}
					else
						strcpy(interrupt_name, "int??");

					strcpy(pcb_name, "kpcb??");
					interrupt_name[3] = pcb_name[4] = hex[ vector/16 ];
					interrupt_name[4] = pcb_name[5] = hex[ vector%16 ];

					strcpy(handler_name, mp->prefix);

					if (mp->ndev != 0 && (int)mp->nvec/(int)mp->ndev == 2)
						/*
						 * transmit/receive vectors are:
						 *	transmit[ndev]
						 *	receive[ndev]
						 * except for the CC console
						 *	receive[ndev]
						 *	transmit[ndev]
						 */
						switch (order | ((j<(int)mp->ndev)? T : F))
							{
						case RX | T:
						case XR | F:
							strcat(handler_name, "r");
							count = rcount++;
							break;
						case RX | F:
						case XR | T:
							strcat(handler_name, "x");
							count = xcount++;
							break;
							}
					else
						count = icount++;

					strcat(handler_name, "int");

					generate(G_IRTN, interrupt_name, handler_name, count);

					if (on_lbe)
						/*
						 * no PCB is needed, just fill in LBE[][]
						 */
						{
						Xrelocate((address) &LBE[lbe_unit*256 + vector], R_DIR32, interrupt_name,&data_rel);
						continue;
						}

					if (dp->flag & ISLBE)
						/*
						 * each PCB at a different interrupt priority
						 */
						generate(G_PCB, pcb_name, interrupt_name, 14-2*j, vector);
					else
						generate(G_PCB, pcb_name, interrupt_name, mp->ipl, vector);
					}
				}
			}
		}
		while (dp = dp->next);

	/*
	 * generate the system devices
	 */
	generate(G_DATA, "rootdev", sizeof(rootdev), &rootdev);
#ifdef u3b15
	generate(G_DATA, "dumpdev", sizeof(dumpdev), &dumpdev);
#endif
      	generate(G_DATA, "mirrordev", sizeof(mirrordev), &mirrordev[0]);
	generate(G_DATA, "swapfile", sizeof(swapfile), &swapfile);

	/*
	 * get the space for the io_init[], io_start[], pwr_clr[], io_poll[],
	 * and io_halt[] arrays;
	 * these arrays are initialized by loadriver() and allocated by
	 * relocatable()
	 */
	if ((io_init=next_init=(int(**)())malloc((unsigned)(number_drivers+1)*(sizeof(*io_init)
		+sizeof(*io_start)+sizeof(*pwr_clr)+sizeof(*io_poll)+sizeof(*io_halt)))) == NULL)
		panic("No memory for io_init[], io_start[], pwr_clr[], io_poll[] or io_halt[]");

	io_start = next_start = io_init + number_drivers + 1;
	pwr_clr = next_pwrclr = io_start + number_drivers + 1;
	io_poll = next_poll = pwr_clr + number_drivers + 1;
	io_halt = next_halt = io_poll + number_drivers + 1;
	}


/* 
 * Proc_vtoc()
 *
 * Set major, minor, and root values from vtoc and edt.
 *
 */

proc_vtoc()
{
	struct stat sb;
	struct pdsector pdsector;
	struct vtoc vtoc;
	int i,fd;

	if (stat(raw_disk,&sb)){
		error(ER80,raw_disk);
		exit(1);
	}

	if ((fd=open(raw_disk,O_RDONLY)) < 0){
		error(ER80,raw_disk);
		exit(1);
	}
	readpd(fd,&pdsector);
	readvtoc(fd,&pdsector.pdinfo,&vtoc);
	for ( i=0; i < V_NUMPAR; i++) {
		if (vtoc.v_part[i].p_tag == V_ROOT)
			VTOC_root = i;
		if (vtoc.v_part[i].p_tag == V_SWAP) {
                        VTOC_swap = i;
                        VTOC_nswap = vtoc.v_part[i].p_size;
		}
	}
	
	if (sizeof(sb.st_rdev) > 2){
		/* new dev format */
		VTOC_major = getemajor(sb.st_rdev);
		VTOC_minor = (geteminor(sb.st_rdev)/16) * 16;
	} else {
		/* old dev format */
		VTOC_major = major(sb.st_rdev);
		VTOC_minor = (minor(sb.st_rdev)/16) * 16;
	}
	close(fd);
}



/*
 * readpd()
 *
 * Read physical device information.
 */
 int
readpd(fd,pdsector)
int		fd;
struct pdsector	*pdsector;
{
	struct io_arg	args;

	args.sectst = 0;
	args.memaddr = (unsigned long) pdsector;
	args.datasz = sizeof(struct pdsector);
	if (ioctl(fd, V_PDREAD, &args) < 0){
		error(ER8);
		exit(1);
		}
	if (args.retval == V_BADREAD){
		error(ER8);
		exit(1);
		}
}

/*
 * readvtoc()
 *
 * Read a partition map.
 */
 int
readvtoc(fd, pdinfo, vtoc)
int		fd;
struct pdinfo	*pdinfo;
struct vtoc	*vtoc;
{
	struct io_arg	args;

	args.sectst = pdinfo->logicalst + 1;
	args.memaddr = (unsigned long) vtoc;
	args.datasz = sizeof(struct vtoc);
	if (ioctl(fd, V_PREAD, &args) < 0){
		error(ER8);
		exit(1);
		}
	if (args.retval == V_BADREAD){
		error(ER8);
		exit(1);
		}
	if (vtoc->v_sanity != VTOC_SANE){
		error(ER9);
		exit(1);
		}
}

