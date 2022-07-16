/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_SYSGDB_H
#define _SYS_SYSGDB_H

#ident	"@(#)head.sys:sys/sysgdb.h	11.2"

/* sysgen data block descriptors */

struct	sgcom
{
	long	sgjid;	/* sysgen job id */
	unsigned char	sdbsize;	/* sysgen data block size in words */
	unsigned char	sgopc;	/* sysgen opcode */
	unsigned char	njrq;	/* number of job request queues */
	unsigned char	njcq;	/* number of job completion queues */
};

/* sysgen common job queue descriptor */

struct	sgjqd
{
	paddr_t	*jqsa;	/* queue start physical address */
	paddr_t	*jqldp;	/* queue load pointer phys addr. */
	paddr_t	*jquldp;	/* queue unload pointer phys. addr. */
	long	jqsize;	/* queue size in words */
	long	jqdsp1;	/* job queue descriptor spare */
};


#endif	/* _SYS_SYSGDB_H */
