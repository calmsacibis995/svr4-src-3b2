/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)krpc:rpc/cpjsleep.c	1.2"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)cpjsleep.c 1.3 89/01/27 SMI"
#endif

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

int cpjsleeps;
/*
 *	This function waits for timo clock ticks for something
 *
 *	Returns:
 *		-1	on failure
 *		 -2	on timeout 
 *		 0	if desired event has occurred
 *
 *	Most of the code is from strwaitq().
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/stream.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/strsubr.h>
#include <sys/tihdr.h>
#include <sys/timod.h>
#include <sys/tiuser.h>
#include <sys/t_kuser.h>
int cpjflag;

static void ktli_poll();

int 
cpjsleep(ptr, prio,timo)
register int timo;
int ptr;
int prio;

{
	register int s;
	register int timeid;
	register int status;


	/* set timer and sleep.
	 */
	s = splstr();
	cpjflag = 0;
	if (timo > 0) {
		if ((timeid = timeout(ktli_poll, ptr, timo)) < 0) {
			printf("cpjsleep: Can't set timer\n");
			return -1;
		}
	}
	if (sleep((caddr_t)ptr, prio|PCATCH)) {
		if (timo > 0)
			untimeout(timeid);
		(void) splx(s);
		u.u_error = EINTR;
		return -1;
	}

	if(cpjflag != 0){
		printf("cpjsleep:  woken from sleep -- ip or ether lost buffer free callback.\n");
		cpjsleeps++;
		status = -2 ;
	}
	else status = 0;	

	if (timo > 0)
		untimeout(timeid);
	(void)splx(s);

	return(status);	
}



static void
ktli_poll(ptr)
int ptr;

{
	cpjflag = -1;
	wakeup(ptr);
}

/******************************************************************************/
