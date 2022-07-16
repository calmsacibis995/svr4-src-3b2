/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SYS_DEBUG_H
#define _SYS_DEBUG_H

#ident	"@(#)head.sys:sys/debug.h	11.11"

#define	YES 1
#define	NO  0

#if DEBUG == YES

#if defined(__STDC__)
extern int assfail(char *, char *, int);
#define ASSERT(EX) ((void)((EX) || assfail(#EX, __FILE__, __LINE__)))
#else
extern int assfail();
#define ASSERT(EX) ((void)((EX) || assfail("EX", __FILE__, __LINE__)))
#endif

#else

#define ASSERT(x)

#endif

#ifdef MONITOR
#define MONITOR(id, w1, w2, w3, w4) monitor(id, w1, w2, w3, w4)
#else
#define MONITOR(id, w1, w2, w3, w4)
#endif

#ifdef DEBUG
#define STATIC
#else
#define STATIC static
#endif

#endif	/* _SYS_DEBUG_H */
