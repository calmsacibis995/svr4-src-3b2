/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:s_ioctl.c	1.6"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#include <fcntl.h>
#include <sys/param.h>
#include <sys/sockio.h>
#include <sys/filio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stropts.h>
#include <sys/signal.h>
#include <sys/termios.h>
#include <sys/socket.h>
#include <sys/sockmod.h>
#include <errno.h>
#include <syslog.h>

#ifndef NULL
#define	NULL	0
#endif /* NULL */

#define		ISSOCK(A)	(ioctl((A), I_FIND, "sockmod"))

int
fcntl(des, cmd, arg)
	register int	des;
	register int	cmd;
	int		arg;

{
	int		res;
	int		retval;

	switch(cmd) {
	case F_SETOWN:
		SOCKDEBUG((struct _si_user *)NULL,
					"fcntl: got fcntl-F_SETOWN\n", 0);
		return ioctl(des, FIOSETOWN, &arg);

	case F_GETOWN:
		SOCKDEBUG((struct _si_user *)NULL,
					"fcntl: got fcntl-F_GETOWN\n", 0);
		if (ioctl(des, FIOGETOWN, &res) < 0)
			return -1;
		return res;

	case F_SETFL:
		SOCKDEBUG((struct _si_user *)NULL,
					"fcntl: got fcntl-F_SETFL\n", 0);
		if (ISSOCK(des)) {
			res = arg & FASYNC;
			if (_s_dofioasync(des, &res) < 0)
				return -1;
		}
		return _fcntl(des, cmd, arg);

	case F_GETFL: {
		register int flags;

		SOCKDEBUG((struct _si_user *)NULL,
					"fcntl: got fcntl-F_GETFL\n", 0);
		if ((flags = _fcntl(des, cmd, arg)) < 0)
			return -1;
		if (ISSOCK(des)) {
			/* See if SIGIO is in force.
			 */
			if (ioctl(des, I_GETSIG, &retval) < 0) {
				if (errno != EINVAL)
					return -1;
				else	errno = 0;
			}
			else
			if (retval & (S_RDNORM|S_WRNORM))
				flags |= FASYNC;
		}
		return flags;
	}

	default:
		return _fcntl(des, cmd, arg);
	}
}

int
ioctl(des, request, arg)
	register int	des;
	register int	request;
	int		arg;

{
	pid_t		pid;
	int		retval;

	SOCKDEBUG((struct _si_user *)NULL,
				"ioctl: got ioctl request %x\n", request);
	switch(request) {
	case FIOASYNC:
	case FIOSETOWN:
	case FIOGETOWN:
	case SIOCSPGRP:
	case SIOCGPGRP:
	case SIOCATMARK:
		if (ISSOCK(des) == 0) {
			if (request == SIOCSPGRP || request == SIOCGPGRP ||
						request == SIOCATMARK) {
				SOCKDEBUG((struct _si_user *)NULL,
						"ioctl: %d not socket\n", des);
				errno = ENOTSOCK;
				return -1;
			}
			else	break;
		}

		switch(request) {
		case FIOASYNC:
			/* Facilitate SIGIO.
			 */
			return _s_dofioasync(des, arg);
	
		case SIOCGPGRP:
		case FIOGETOWN:
			SOCKDEBUG((struct _si_user *)NULL,
					"ioctl: got SIOCGPGRP/FIOGETOWN\n", 0);
			retval = 0;
			if (_ioctl(des, I_GETSIG, &retval) < 0) {
				if (errno == EINVAL) {
					retval = 0;
				}
				else	return -1;
			}
			if (retval & (S_RDBAND|S_BANDURG|S_RDNORM|S_WRNORM))
				*(pid_t *)arg = getpid();
			else	*(pid_t *)arg = 0;
			return 0;
	
		case SIOCSPGRP:
		case FIOSETOWN:
			/* Facilitate receipt of SIGURG.
			 *
			 * We are forgiving in that if a
			 * process group was specified rather
			 * than a process id, we will only
			 * fail it if the process group
			 * specified is not the callers.
			 */
			SOCKDEBUG((struct _si_user *)NULL,
					"ioctl: got SIOCSPGRP/FIOSETOWN\n", 0);
			pid = *(pid_t *)arg;
			if (pid < 0) {
				pid = -pid;
				if (pid != getpgrp()) {
					errno = EINVAL;
					return -1;
				}
			}
			else	{
				if (pid != getpid()) {
					errno = EINVAL;
					return -1;
				}
			}
			retval = 0;
			if (_ioctl(des, I_GETSIG, &retval) < 0 &&
							 errno != EINVAL) 
				return -1;

			retval |= (S_RDBAND|S_BANDURG);
			if (_ioctl(des, I_SETSIG, retval) < 0) {
				(void)syslog(LOG_ERR,
					"ioctl: I_SETSIG failed %d\n", errno);
				return -1;
			}
			return 0;
	
		case SIOCATMARK:
			SOCKDEBUG((struct _si_user *)NULL,
						"ioctl: got SIOCATMARK\n", 0);
			if ((retval = _ioctl(des, I_ATMARK, LASTMARK)) < 0)
				return -1;
			*(int *)arg = retval;
			return 0;
		}

	default:
		break;
	}
	SOCKDEBUG((struct _si_user *)NULL,
			"ioctl: match failed, calling regular ioctl\n", 0);
	return _ioctl(des, request, arg);
}


/* Enable or disable asynchronous I/O
 */
static int
_s_dofioasync(des, arg)
	register int	des;
	register int	*arg;
{
	int	retval;

	SOCKDEBUG((struct _si_user *)NULL, "_s_dofioasync: Entered, %d\n",
						*arg);
	/*  Turn on or off async I/O.
	 */
	retval = 0;
	if (*arg) {
		/* Turn ON SIGIO.
		 */
		if (_ioctl(des, I_GETSIG, &retval) < 0) {
			if (errno != EINVAL)
				return -1;
			else	errno = 0;
		}
		else	{
			/* Don't bother if already on.
		 	*/
			if ((retval & (S_RDNORM|S_WRNORM)) ==
						 (S_RDNORM|S_WRNORM))
				return 0;
		}

		SOCKDEBUG((struct _si_user *)NULL,
			"_s_dofioasync: Setting SIGIO\n", 0);
		retval |= (S_RDNORM|S_WRNORM);
		return _ioctl(des, I_SETSIG, retval);
	}

	/* Turn OFF SIGIO.
	 */
	SOCKDEBUG((struct _si_user *)NULL,
			"_s_dofioasync: Re-setting SIGIO\n", 0);
	if (_ioctl(des, I_GETSIG, &retval) < 0) {
		if (errno == EINVAL) {
			errno = 0;
			return 0;
		}
		else	return -1;
	}
	if ( !(retval & (S_RDNORM|S_WRNORM)))
		/* Async not in effect.
		 */
		return 0;

	/* Clear out the old.
	 */
	(void)_ioctl(des, I_SETSIG, 0) ;

	/* Bring in the new.
	 */
	if (retval &= ~(S_RDNORM|S_WRNORM))
		return _ioctl(des, I_SETSIG, retval);
	return 0;
}


