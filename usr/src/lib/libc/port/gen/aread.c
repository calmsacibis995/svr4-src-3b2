/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libc-port:gen/aread.c	1.3"

#include "synonyms.h"
#include <sys/types.h>
#include <sys/evecb.h>
#include <sys/priocntl.h>
#include <sys/procset.h>
#include <sys/hrtcntl.h>
#include <sys/signal.h>
#include <sys/events.h>

#ifdef i386

#include <sys/tss.h>

#else	/* 3b2, default */

#include <sys/psw.h>
#include <sys/pcb.h>

#endif

#include <sys/user.h>
#include <sys/asyncio.h>
#include <sys/asyncsys.h>
#include <sys/errno.h>

extern	int	errno;

#define	NULL	0
#define	READ	3
#define	ASYNC	111

int
__aread(ver, fildes, bufp, bufs, ecbp)
int	ver;
int	fildes;
char	*bufp;
uint	bufs;
ecb_t	*ecbp;
{
	register asyncop_t	*aioreqp;	
	register int		ret;


	if ((aioreqp = (asyncop_t *)malloc(sizeof(asyncop_t))) == NULL) {
		errno = EAGAIN;
		return(-1);
	}

	aioreqp->a_syscall = READ;
	aioreqp->a_sysarg[0] = fildes;
	aioreqp->a_sysarg[1] = (int) bufp;
	aioreqp->a_sysarg[2] = (int) bufs;
	aioreqp->a_flags = 0;
	aioreqp->a_ecb = *ecbp;

	ret = _syscall(ASYNC, ver, aioreqp, 1);

	if (ret == 1)				     /* if request is accepted */
		ret = 0;
	else if (ret == 0) {			     /* local error encountered */
		errno = aioreqp->a_error;
		ret = -1;
	}

	/* if global error encountered */
	/* return code and errno should be set already */

	free(aioreqp);
	return(ret);
}
