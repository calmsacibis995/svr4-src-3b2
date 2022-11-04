/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_VMSYSTM_H
#define _SYS_VMSYSTM_H

#ident	"@(#)head.sys:sys/vmsystm.h	1.4"

/*
 * Miscellaneous virtual memory subsystem variables and structures.
 */

#ifdef _KERNEL
int	freemem;		/* remaining blocks of free memory */
int	avefree;		/* moving average of remaining free blocks */
int	avefree30;		/* 30 sec (avefree is 5 sec) moving average */
int	deficit;		/* estimate of needs of new swapped in procs */
int	nscan;			/* number of scans in last second */
int	multprog;		/* current multiprogramming degree */
int	desscan;		/* desired pages scanned per second */

/* writable copies of tunables */
int	maxpgio;		/* max paging i/o per sec before start swaps */
int	maxslp;			/* max sleep time before very swappable */
int	lotsfree;		/* max free before clock freezes */
int	minfree;		/* minimum free pages before swapping begins */
int	desfree;		/* no of pages to try to keep free via daemon */
int	saferss;		/* no pages not to steal; decays with slptime */
#endif

/*
 * Fork/vfork accounting.
 */
struct	forkstat
{
	int	cntfork;
	int	cntvfork;
	int	sizfork;
	int	sizvfork;
};
#ifdef _KERNEL
struct	forkstat forkstat;

#if defined(__STDC__)
extern void vmtotal(void);
extern int valid_va_range(addr_t *, u_int *, u_int, int);
extern int valid_usr_range(addr_t, size_t);
extern int useracc(caddr_t, uint, int);
extern int page_deladd(int, int, rval_t *);
extern void map_addr(addr_t *, u_int, off_t, int);
#else
extern void vmtotal(void);
extern void vmtotal();
extern int valid_va_range();
extern int valid_usr_range();
extern int useracc();
extern int page_deladd();
extern void map_addr();
#endif	/* __STDC__ */

#endif	/* _KERNEL */

#endif	/* _SYS_VMSYSTM_H */
