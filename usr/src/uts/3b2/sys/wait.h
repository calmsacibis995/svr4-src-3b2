/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include "sys/types.h"
#include "sys/siginfo.h"
#include "sys/procset.h"

#ident	"@(#)head.sys:sys/wait.h	1.10"

/*
 * arguments to wait functions
 */

#define WEXITED		0001	/* wait for processes that have exited	*/
#define WTRAPPED	0002	/* wait for processes stopped while tracing */
#define WSTOPPED	0004	/* wait for processes stopped by signals */
#define WCONTINUED	0010	/* wait for processes continued */

#define WUNTRACED	WSTOPPED /* for POSIX */

#define WNOHANG		0100	/* non blocking form of wait	*/
#define WNOWAIT		0200	/* non destructive form of wait */

#define WOPTMASK	(WEXITED|WTRAPPED|WSTOPPED|WCONTINUED|WNOHANG|WNOWAIT)

/*
 * macros for stat return from wait functions
 */

#define WSTOPFLG		0177
#define WCONTFLG		0177777
#define WCOREFLG		0200
#define WSIGMASK		0177

#define WLOBYTE(stat)		((int)((stat)&0377))
#define WHIBYTE(stat)		((int)(((stat)>>8)&0377))
#define WWORD(stat)		((int)((stat))&0177777)

#define WIFEXITED(stat)		(WLOBYTE(stat)==0)
#define WIFSIGNALED(stat)	(WLOBYTE(stat)>0&&WHIBYTE(stat)==0)
#define WIFSTOPPED(stat)	(WLOBYTE(stat)==WSTOPFLG&&WHIBYTE(stat)!=0)
#define WIFCONTINUED(stat)	(WWORD(stat)==WCONTFLG)

#define WEXITSTATUS(stat)	WHIBYTE(stat)
#define WTERMSIG(stat)		(WLOBYTE(stat)&WSIGMASK)
#define WSTOPSIG(stat)		WHIBYTE(stat)
#define WCOREDUMP(stat)		((stat)&WCOREFLG)



#if !defined(_KERNEL)
#if defined(__STDC__)

extern pid_t wait(int *);
extern pid_t waitpid(pid_t, int *, int);
extern int waitid(idtype_t, id_t, siginfo_t *, int);

#else

extern pid_t wait();
extern pid_t waitpid();
extern int waitid();

#endif	/* __STDC__ */
#endif	/* _KERNEL */

#endif	/* _SYS_WAIT_H */
