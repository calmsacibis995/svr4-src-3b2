/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:getsocknm.c	1.5"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stream.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include <sys/timod.h>
#include <sys/sockmod.h>
#include <sys/socketvar.h>
#include <sys/socket.h>
#include <sys/tiuser.h>
#include <sys/signal.h>

extern int	errno;

int
getsockname(s, name, namelen)
	register int			s;
	register struct sockaddr	*name;
	register int			*namelen;
{
	register struct _si_user	*siptr;

	if ((siptr = _s_checkfd(s)) == NULL)
		return -1;

	if (name == NULL || namelen == NULL) {
		errno = EINVAL;
		return -1;
	}

	return _getsockname(siptr, name, namelen);
}

int
_getsockname(siptr, name, namelen)
	register struct _si_user	*siptr;
	register struct sockaddr	*name;
	register int			*namelen;
{
	register void			(*sigsave)();
	struct netbuf			netbuf;

	netbuf.len = 0;
	netbuf.maxlen = siptr->ctlsize;
	netbuf.buf = siptr->ctlbuf;

	/* get it from sockmod.
	 */
        sigsave = sigset(SIGPOLL, SIG_HOLD);
	if (ioctl(siptr->fd, TI_GETMYNAME, (caddr_t)&netbuf) < 0) {
                (void)sigset(SIGPOLL, sigsave);
                return -1;
        }
        (void)sigset(SIGPOLL, sigsave);

	*namelen = _s_cpaddr(siptr, name, *namelen, netbuf.buf, netbuf.len);
	return 0;
}
