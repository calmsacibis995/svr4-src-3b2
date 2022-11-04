/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_TUNEABLE_H
#define _SYS_TUNEABLE_H

#ident	"@(#)head.sys:sys/tuneable.h	11.5"

typedef struct tune {
	int	t_gpgslo;	/* If freemem < t_getpgslow, then start	*/
				/* to steal pages from processes.	*/
	int	t_gpgshi;	/* Once we start to steal pages, don't	*/
				/* stop until freemem > t_getpgshi.	*/
	int	t_pad[5];	/* Padding for driver compatibility.    */
	int	t_maxumem;	/* The maximum size of a user's virtual	*/
				/* address space in pages.		*/
	int	t_fsflushr;	/* The rate at which fsflush is run in	*/
				/* seconds.				*/
	int	t_minarmem;	/* The minimum available resident (not	*/
				/* swapable) memory to maintain in 	*/
				/* order to avoid deadlock.  In pages.	*/
	int	t_minasmem;	/* The minimum available swapable	*/
				/* memory to maintain in order to avoid	*/
				/* deadlock.  In pages.			*/
} tune_t;

extern tune_t	tune;

/*	The following is the default value for t_gpgsmsk.  It cannot be
 *	defined in /etc/master or /stand/system due to limitations of the
 *	config program.
 */

#define	GETPGSMSK	PG_REF|PG_NDREF

#endif	/* _SYS_TUNEABLE_H */
