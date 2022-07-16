/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)kernel:os/cxenix.c	1.5"

/*
 *	@(#) cxenix.c 1.2 88/04/08 kern-os:cxenix.c
 */

#include "sys/param.h"
#include "sys/types.h"
#include "sys/systm.h"
#include "sys/signal.h"
#include "sys/psw.h"
#include "sys/pcb.h"
#include "sys/immu.h"
#include "sys/cred.h"
#include "sys/user.h"
#include "sys/proc.h"
#include "sys/errno.h"
#include "sys/reg.h"
#include "sys/cmn_err.h"

int	nosys();  
int	chsize(); 
int	creatsem();
int	ftime();    
int	locking();   
int	nap(); 
int	nbwaitsem();
int	opensem();
int	proctl(); 
int	rdchk(); 
int	sdenter(); 
int	xsdfree();    
int	sdget();   
int	sdgetv(); 
int	sdleave();  
int	sdwaitv();
int	sigsem();
int	waitsem();

/*
 * XENIX-special system calls.  In order to save space in the system
 * call table, and to minimize conflicts with other unix systems,
 * all custom XENIX calls are done via the cxenix call.
 * The cxentry table is the switch used to transfer
 * to the appropriate routine for processing a cxenix sub-type system call.
 * Each row contains the number of arguments expected,
 * a switch that tells systrap() in trap.c whether a setjmp() is not necessary,
 * and a pointer to the routine.  Note that all cxenix system calls will
 * do the setjmp() in systrap(), for XENIX compatibility.
 */

struct sysent cxentry[] = {
	0, 0, nosys,			/* 0 = obsolete (XENIX shutdown) */
	3, 0, locking,			/* 1 = XENIX file/record lock */
	2, 0, creatsem,			/* 2 = create XENIX semaphore */
	1, 0, opensem,			/* 3 = open XENIX semaphore */
	1, 0, sigsem,			/* 4 = signal XENIX semaphore */
	1, 0, waitsem,			/* 5 = wait on XENIX semaphore */
	1, 0, nbwaitsem,		/* 6 = nonblocking wait on XENIX sem */
	1, 0, rdchk,			/* 7 = read check */
	0, 0, nosys,			/* 8 = obsolete (XENIX stkgrow) */
	0, 0, nosys,			/* 9 = obsolete (XENIX ptrace) */
	2, 0, chsize,			/* 10 = change file size */
	1, 0, ftime,			/* 11 = V7 ftime*/
	1, 0, nap,			/* 12 = nap */
	4, 0, sdget,			/* 13 = create/attach XENIX shdata */
	1, 0, xsdfree,			/* 14 = free XENIX shdata */
	2, 0, sdenter,			/* 15 = enter XENIX shdata */
	1, 0, sdleave,			/* 16 = leave XENIX shdata */
	1, 0, sdgetv,			/* 17 = get XENIX shdata version */
	2, 0, sdwaitv,			/* 18 = wait for XENIX shdata version */
	0, 0, nosys,			/* 19 = obsolete (XENIX brkctl) */
	0, 0, nosys,			/* 20 = unused (reserved for XENIX) */
	0, 0, nosys,			/* 21 = obsolete (XENIX nfs_sys) */
	0, 0, nosys,			/* 22 = obsolete (XENIX msgctl) */
	0, 0, nosys,			/* 23 = obsolete (XENIX msgget) */
	0, 0, nosys,			/* 24 = obsolete (XENIX msgsnd) */
	0, 0, nosys,			/* 25 = obsolete (XENIX msgrcv) */
	0, 0, nosys,			/* 26 = obsolete (XENIX semctl) */
	0, 0, nosys,			/* 27 = obsolete (XENIX semget) */
	0, 0, nosys,			/* 28 = obsolete (XENIX semop) */
	0, 0, nosys,			/* 29 = obsolete (XENIX shmctl) */
	0, 0, nosys,			/* 30 = obsolete (XENIX shmget) */
	0, 0, nosys,			/* 31 = obsolete (XENIX shmat) */
	3, 0, proctl,			/* 32 = proctl */
	0, 0, nosys,			/* 33 = execseg */
	0, 0, nosys,			/* 34 = unexecseg */
	0, 0, nosys,			/* 35 = obsolete (XENIX swapadd) */
};

/* number of cxenix subfunctions */	
int ncxentry = sizeof(cxentry)/sizeof(struct sysent);

/*
 *      cxenix - XENIX custom system call dispatcher
 */

/* ARGSUSED */
int
cxenix(uap, rvp)
	char *uap;
	rval_t *rvp;
{
	register int subfunc;
	register struct user *uptr = &u;
	register struct sysent *callp;
	register int *ap;
	register u_int i;
	int ret;

	ap = (int *)uptr->u_pcb.regsave[K_AP];
	subfunc = (*ap++ >> 8) & 0xff;
	if (subfunc >= ncxentry)
		return EINVAL;
	callp = &cxentry[subfunc];

	/* get cxenix arguments in U block */
	for (i = 0; i < callp->sy_narg; i++)
		uptr->u_arg[i] = lfuword(ap++);
	uptr->u_ap = uptr->u_arg;

	/* do the system call */
	ret = (*callp->sy_call)(uptr->u_ap, rvp);
	return ret;

}
