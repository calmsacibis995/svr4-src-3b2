/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:sys/hrtsys.c	1.1"

#ifdef __STDC__
	#pragma weak hrtcntl = _hrtcntl
	#pragma weak hrtalarm = _hrtalarm
	#pragma weak hrtsleep = _hrtsleep
	#pragma weak hrtcancel = _hrtcancel
#endif
#include	"synonyms.h"
#include	"sys/types.h"
#include	"sys/evecb.h"
#include	"sys/hrtcntl.h"

#define	HRTSYS		109

#define	HRTCNTL		0
#define HRTALARM	1
#define HRTSLEEP	2
#define	HRTCANCEL	3

hrtcntl(cmd, clk, intp, hrtp)
int cmd, clk;
interval_t *intp;
hrtime_t *hrtp;
{
	return(syscall(HRTSYS, HRTCNTL, cmd, clk, intp, hrtp));
}

hrtalarm(cmdp, cmds)
hrtcmd_t *cmdp;
int cmds;
{
	return(syscall(HRTSYS, HRTALARM, cmdp, cmds));
}

hrtsleep(cmdp)
hrtcmd_t *cmdp;
{
	return(syscall(HRTSYS, HRTSLEEP, cmdp));
}

hrtcancel(eidp, eids)
const long *eidp;
int eids;
{
	return(syscall(HRTSYS, HRTCANCEL, eidp, eids));
}
