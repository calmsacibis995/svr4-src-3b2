/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libc-port:gen/asyncio.c	1.2"

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

extern	int 	errno;

#define	NULL	0
#define	ASYNC	111
#define	READ	3
#define	WRITE	4

int
__asyncio(ver, aiop, aios)
int		ver;
register	aioop_t *aiop;
int		aios;
{

	register asyncop_t *aioreqp;
	register asyncop_t *curp;
	register aioop_t   *caiop;
	register i;
	int	 ret;

	if (aios <= 0) {
		errno = EINVAL;
		return(-1);
	} 
	
	/* if error */
	if ((aioreqp = (asyncop_t *)malloc(aios * sizeof(asyncop_t))) == NULL) {
		errno = EAGAIN;
		return(-1);
	}
	
	caiop = aiop;
	for (i = 0, curp = aioreqp; i < aios; i++, curp++, caiop++) {
		if (caiop->aio_cmd == AIOC_READ)
			curp->a_syscall = READ;
		else if (caiop->aio_cmd == AIOC_WRITE)
			curp->a_syscall = WRITE;
		else {
			curp->a_flags = caiop->aio_flags & AIOF_UFLAGS | AIOF_ERROR;
			curp->a_error = EINVAL;
			continue;
		}

		curp->a_sysarg[0] = caiop->aio_fd;
		curp->a_sysarg[1] = (int) caiop->aio_bufp;
		curp->a_sysarg[2] = caiop->aio_bufs;
		curp->a_flags = caiop->aio_flags & AIOF_UFLAGS;
		curp->a_offset = caiop->aio_offset;
		curp->a_error = 0;
		curp->a_pri = caiop->aio_pri;
		curp->a_ecb = caiop->aio_ecb;
	}

	ret = _syscall(ASYNC, ver, aioreqp, aios);

	for (i = 0, curp = aioreqp; i < aios; i++, curp++, aiop++) {
		if ((curp->a_flags & AIOF_PROCESS) == 0) 
			break;
		else {
			aiop->aio_flags = curp->a_flags;
			aiop->aio_error = curp->a_error;
		}
	}

	free(aioreqp);

	return(ret);
}
