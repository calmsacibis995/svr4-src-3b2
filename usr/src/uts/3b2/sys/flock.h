/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_FLOCK_H
#define _SYS_FLOCK_H

#ident	"@(#)head.sys:sys/flock.h	11.9"

#define INOFLCK		1	/* Vnode is locked when reclock() is called. */
#define SETFLCK		2	/* Set a file lock. */
#define SLPFLCK		4	/* Wait if blocked. */

#define IGN_PID		-1	/* ignore epid when cleaning locks	*/

/* file locking structure (connected to vnode) */

#define l_end 		l_len
#define MAXEND  	017777777777

typedef struct filock {
	struct	flock set;	/* contains type, start, and end */
	union	{
		int wakeflg;	/* for locks sleeping on this one */
		struct {
			long sysid;
			pid_t pid;
		} blk;			/* for sleeping locks only */
	}	stat;
#ifdef	u3b
	int	wakesem;
#endif
	struct	filock *prev;
	struct	filock *next;
} filock_t;

/* file and record locking configuration structure */
/* record use total may overflow */
struct flckinfo {
	long recs;	/* number of records configured on system */
	long reccnt;	/* number of records currently in use */
	long recovf;	/* number of times system ran out of record locks. */
	long rectot;	/* number of records used since system boot */
};

extern struct flckinfo	flckinfo;

#if defined(__STDC__)
int	reclock(struct vnode *, struct flock *, int, int, off_t);
#else
int	reclock();
#endif

#endif	/* _SYS_FLOCK_H */
