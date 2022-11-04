/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)krpc:krpc/authdesubr.c	1.8"
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)authdesubr.c 1.3 89/03/19 SMI"
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

/*
 * Miscellaneous support routines for kernel implentation of AUTH_DES
 */

/*
 *  rtime - get time from remote machine
 *
 *  sets time, obtaining value from host
 *  on the udp/time socket.  Since timeserver returns
 *  with time of day in seconds since Jan 1, 1900,  must
 *  subtract 86400(365*70 + 17) to get time
 *  since Jan 1, 1970, which is what get/settimeofday
 *  uses.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/socket.h>
#include <sys/sysmacros.h>
#include <netinet/in.h>
#include <rpc/rpc.h>
#include <sys/stream.h>
#include <sys/strsubr.h>
#include <sys/cred.h>
#include <sys/utsname.h>
#include <sys/cmn_err.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/systeminfo.h>

#ifndef DEBUG
#define cmn_err(CE_NOTE, msg)		/* turn off debugging */
#endif

#define USEC_PER_SEC 1000000
#define TOFFSET (86400U*(365*70 + (70/4)))
#define WRITTEN (86400U*(365*86 + (86/4)))

/*
rtime_wakeup(so)
	struct socket *so;	
{
	so->so_error = ETIMEDOUT;
	sbwakeup(so, &so->so_rcv);
}
*/


rtime(addrp, timep, wait)
	struct netbuf *addrp;
	struct timeval *timep;
	struct timeval *wait;
{
	int error, timo;
	u_long thetime;
	/* int s; */
	int dummy;
	register struct t_kunitdata *unitdata;
	register TIUSER *tiptr;
	/* extern int clone_no, udp_no; */
	struct vnode *vp;

	if ((error = lookupname ("/dev/udp", UIO_SYSSPACE, FOLLOW, NULLVPP, &vp)) != 0)
		return -1;
	if ((tiptr = t_kopen(NULL, vp->v_rdev, O_RDWR|O_NDELAY,
					(struct t_info *)NULL)) == NULL) {
		VN_RELE(vp);
		return -1;
	}
	VN_RELE(vp);
	if (t_kbind(tiptr, NULL, NULL) < 0) {
		t_kclose(tiptr, 1);
		return -1;
	}
	if ((unitdata = (struct t_kunitdata *)t_kalloc(tiptr, T_UNITDATA,
						/* LINTED pointer alignment */
                                                T_UDATA|T_ADDR)) == (struct t_kunitdata *)NULL) {
                printf("rtime: t_kalloc %d\n", u.u_error);
		t_kclose(tiptr, 1);
                return -1;
        }

	unitdata->addr.len = addrp->len;
	bcopy(addrp->buf, unitdata->addr.buf, unitdata->addr.len);
	unitdata->udata.buf = (caddr_t)&dummy;
	unitdata->udata.len = sizeof(dummy);

	if (t_ksndudata(tiptr, unitdata, NULL) < 0) {
		t_kclose(tiptr, 1);
		return -1;
	}

	timo = (int)(wait->tv_sec * HZ + (wait->tv_usec * HZ) / USEC_PER_SEC);
	if (t_kspoll(tiptr, timo, READWAIT) < 0) {
		t_kclose(tiptr, 1);
		return -1;
	}

	if (t_krcvudata(tiptr, unitdata, &error) < 0) {
		t_kclose(tiptr, 1);
		return -1;
	}

	if (unitdata->udata.len < sizeof(u_long)) {
		printf("rtime: bad rcvd length %d\n", unitdata->udata.len);
		t_kclose(tiptr, 1);
		return -1;
	}

	t_kclose(tiptr, 1);

	/* LINTED pointer alignment */
	thetime = *(u_long *)unitdata->udata.buf;

	if (thetime < WRITTEN) {
		cmn_err(CE_NOTE, "time returned is too far in past");
		return(-1);
	}
	timep->tv_sec = thetime - TOFFSET;
	timep->tv_usec = 0;
	return(0);
}


/*
 * Short to ascii conversion
 */
static char *
sitoa(s, i)
	char *s;
	short i;
{
	char *p;
	char *end;
	char c;

	if (i < 0) {
		*s++ = '-';		
		i = -i;
	} else if (i == 0) {
		*s++ = '0';
	}

	/*
	 * format in reverse order
	 */
	for (p = s; i > 0; i /= 10) {	
		*p++ = (i % 10) + '0';
	}
	*(end = p) = 0; 

	/*
	 * reverse
	 */
	while (p > s) {
		c = *--p;
		*p = *s;
		*s++ = c;
	}
	return(end);
}

static char *
atoa(dst, src)
	char *dst;	
	char *src;
{
	while (*dst++ = *src++)
		;
	return(dst-1);
}

/*
 * What is my network name?
 * WARNING: this gets the network name in sun unix format. 
 * Other operating systems (non-unix) are free to put something else
 * here.
 */

void
getnetname(netname)
	char *netname;
{
	char *p;

	p = atoa(netname, "unix.");
	if (u.u_cred->cr_uid == 0) {
		p = atoa(p, utsname.nodename);
	} else {
		p = sitoa(p, (short)u.u_cred->cr_uid);
	}
	*p++ = '@';
	p = atoa(p, srpc_domain);
}
