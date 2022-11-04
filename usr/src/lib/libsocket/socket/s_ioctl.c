/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:s_ioctl.c	1.5"

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
#ifdef DEBUG
#include <stdio.h>
#endif
#include <fcntl.h>
#include <sys/sockio.h>
#include <sys/filio.h>
#include <sys/types.h>
#include <sys/stropts.h>
#include <sys/signal.h>
#include <sys/termios.h>
#include <errno.h>

int
fcntl(des, cmd, arg)
	register int	des;
	register int	cmd;
	int		arg;

{
	int		res;

	switch(cmd) {
	case F_SETOWN:
#ifdef DEBUG
fprintf(stdout, "fcntl: got fcntl-F_SETOWN\n");
#endif
		return ioctl(des, FIOSETOWN, &arg);

	case F_GETOWN:
#ifdef DEBUG
fprintf(stdout, "fcntl: doing fcntl-F_GETOWN\n");
#endif
		if (ioctl(des, FIOGETOWN, &res) < 0)
			return -1;
		return res;

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

#ifdef DEBUG
fprintf(stdout, "ioctl: got ioctl request %x\n", request);
#endif
	switch(request) {
	case FIOASYNC:
	case FIOSETOWN:
	case FIOGETOWN:
	case SIOCSPGRP:
	case SIOCGPGRP:
	case SIOCATMARK:
		if(_ioctl(des, I_FIND, "sockmod") == 0) {
			if (request == SIOCSPGRP || request == SIOCGPGRP ||
						request == SIOCATMARK) {
#ifdef DEBUG
fprintf(stdout, "ioctl: %d not socket\n", des);
#endif
				errno = ENOTSOCK;
				return -1;
			}
			else	break;
		}

		switch(request) {
		case FIOASYNC:
			/*  Turn on or off async I/O.
			 */
#ifdef DEBUG
fprintf(stdout, "ioctl: got FIOASYNC\n");
#endif
			retval = 0;
			if (*(int *)arg) {
				if (_ioctl(des, I_GETSIG, &retval) < 0 &&
							 errno != EINVAL)
					return -1;

				retval |= (S_RDNORM|S_WRNORM);
				return _ioctl(des, I_SETSIG, retval);
			}

			/* Get current disposition.
			 */
			if (_ioctl(des, I_GETSIG, &retval) < 0) {
				if (errno == EINVAL)
					return 0;
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
	
		case SIOCGPGRP:
		case FIOGETOWN:
#ifdef DEBUG
fprintf(stdout, "ioctl: got SIOCGPGRP/FIOGETOWN\n");
#endif
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
#ifdef DEBUG
fprintf(stdout, "ioctl: got SIOCSPGRP/FIOSETOWN\n");
#endif
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
#ifdef DEBUG
perror("ioctl: I_SETSIG failed\n");
#endif
				return -1;
			}
			pid = getpid();
			if (_ioctl(des, SIOCSPGRP, &pid) < 0) {
#ifdef DEBUG
perror("ioctl: SIOCSPGRP failed\n");
#endif
				return -1;
			}
			return 0;
	
		case SIOCATMARK:
#ifdef DEBUG
fprintf(stdout, "ioctl: got SIOCATMARK\n");
#endif
			if ((retval = _ioctl(des, I_ATMARK, LASTMARK)) < 0)
				return -1;
#ifdef DEBUG
fprintf(stdout, "ioctl: I_ATMARK returned %d\n", retval);
#endif
			*(int *)arg = retval;
			return 0;
		}

	default:
		break;
	}
#ifdef DEBUG
fprintf(stdout, "ioctl: match failed, calling regular ioctl\n");
#endif
	return _ioctl(des, request, arg);
}




