/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kernel:os/main.c	1.28"
#include "sys/types.h"
#include "sys/param.h"
#include "sys/psw.h"
#include "sys/sbd.h"
#include "sys/sysmacros.h"
#include "sys/pcb.h"
#include "sys/immu.h"
#include "sys/systm.h"
#include "sys/signal.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/vfs.h"
#include "sys/vnode.h"
#include "sys/proc.h"
#include "sys/time.h"
#include "sys/file.h"
#include "sys/evecb.h"
#include "sys/hrtcntl.h"
#include "sys/hrtsys.h"
#include "sys/priocntl.h"
#include "sys/procset.h"
#include "sys/events.h"
#include "sys/evsys.h"
#include "sys/asyncsys.h"
#include "sys/var.h"
#include "sys/debug.h"
#include "sys/conf.h"
#include "sys/cmn_err.h"

#include "vm/as.h"
#include "vm/seg_vn.h"

int	physmem;	/* Physical memory size in clicks.	*/
int	maxmem;		/* Maximum available memory in clicks.	*/
int	freemem;	/* Current available memory in clicks.	*/
vnode_t	*rootdir;
long	dudebug;
int 	nodevflag = D_OLD; /* If an old driver, devsw flag entry points here */

extern int	icode[];
extern int	szicode;
extern int	userstack[];


/*
 * Initialization code.
 * fork - process 0 to schedule
 *      - process 1 execute bootstrap
 *
 * loop at low address in user mode -- /etc/init
 * cannot be executed.
 */

main()
{
	register int	(**initptr)();
	extern int	(*io_init[])();
	extern int	(*init_tbl[])();
	extern int	(*io_start[])();
	extern int	sched();
	extern int	pageout();
	extern int	fsflush();
	extern int	kmem_freepool();
	int error = 0;

	startup();
	clkstart();
	cred_init();
	dnlc_init();

	/*
	 * Set up credentials.
	 */
	u.u_cred = crget();

	/*
	 * Call all system initialization functions.
	 */

	for (initptr= &io_init[0]; *initptr; initptr++)
		(**initptr)();
	for (initptr= &init_tbl[0]; *initptr; initptr++)
		(**initptr)();
	for (initptr= &io_start[0]; *initptr; initptr++)
		(**initptr)();

        /*
         * Set the scan rate and other parameters of the paging subsystem.
	 */
	setupclock();

	u.u_error = 0;		/* XXX kludge for SCSI driver */
	vfs_mountroot();	/* Mount the root file system */

	cmn_err(CE_CONT,
		"Available memory   = %d\n\n", ctob(freemem));

	prt_where = PRW_CONS;

	printf("***********************************************************************\n\n");
	printf("Copyright (c) 1984, 1986, 1987, 1988, 1989  AT&T - All Rights Reserved\n\n");
	printf("THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T INC.\n");
	printf("The copyright notice above does not evidence any actual or\n");
	printf("intended publication of such source code.\n\n");
	printf("***********************************************************************\n\n");


	u.u_start = hrestime.tv_sec;

	/*
	 * This call to inituname must come after 
	 * root has been mounted.
	 */
	inituname();

	/*
	 * This call to swapconf must come after 
	 * root has been mounted.
	 */
	swapconf();

	/*
	 * Initialize file descriptor info in uarea.
	 * NB:  getf() in fio.c expects u.u_nofiles >= NFPCHUNK
	 */
	u.u_nofiles = NFPCHUNK;

        schedpaging();

	/*
	 * Make init process; enter scheduling loop with system process.
	 */

	if (newproc(NP_INIT, NULL, &error)) {
		register proc_t *p = u.u_procp;

		p->p_cstime = p->p_stime = p->p_cutime = p->p_utime = 0;

		/*
		 * Set up the text region to do an exec
		 * of /etc/init.  The "icode" is in misc.s.
		 */

		/*
		 * Allocate user address space.
		 */
		p->p_as = as_alloc();

		/*
		 * Make a text segment for icode
		 */
		(void) as_map(p->p_as, UVTEXT,
		    szicode, segvn_create, zfod_argsp);


		if (copyout((caddr_t)icode, (caddr_t)(UVTEXT), szicode))
			cmn_err(CE_PANIC, "main: copyout of icode failed");

		/*
		 * Allocate a stack segment
		 */

		(void) as_map(p->p_as, userstack,
		    ctob(SSIZE), segvn_create, zfod_argsp);

		u.u_pcb.sub = (int *)((uint)userstack + ctob(SSIZE));
		return UVTEXT;
	}
	if (newproc(NP_SYSPROC, NULL, &error)) {
		u.u_procp->p_cstime = u.u_procp->p_stime = 0;
		u.u_procp->p_cutime = u.u_procp->p_utime = 0;
		bcopy("pageout", u.u_psargs, 8);
		bcopy("pageout", u.u_comm, 7);
		pageout();
		cmn_err(CE_PANIC, "main: return from pageout()");
	}

	if (newproc(NP_SYSPROC, NULL, &error)) {
		u.u_procp->p_cstime = u.u_procp->p_stime = 0;
		u.u_procp->p_cutime = u.u_procp->p_utime = 0;
		bcopy("fsflush", u.u_psargs, 8);
		bcopy("fsflush", u.u_comm, 7);
		fsflush();
		cmn_err(CE_PANIC, "main: return from fsflush()");
	}

	if (aio_config() && aiodmn_spawn() != 0) {
		aiodaemon();
		cmn_err(CE_PANIC, "main: return from aiodaemon()");
	}

	if (newproc(NP_SYSPROC, NULL, &error)) {
		/*
		 * use "kmdaemon" rather than "kmem_freepool"
		 * will be more intelligble for ps
		 */
		u.u_procp->p_cstime = u.u_procp->p_stime = 0;
		u.u_procp->p_cutime = u.u_procp->p_utime = 0;
		bcopy("kmdaemon", u.u_psargs, 10);
		bcopy("kmdaemon", u.u_comm, 9);
		kmem_freepool();
		cmn_err(CE_PANIC, "main: return from kmem_freepool()");
	}

	bcopy("sched", u.u_psargs, 6);
	bcopy("sched", u.u_comm, 5);
	return (int)sched;
}
