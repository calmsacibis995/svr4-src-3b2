/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef	MEMDUP_H
#define	MEMDUP_H
/*==================================================================*/
/*
**
*/
#ident	"@(#)lp:include/memdup.h	1.2"

#ifdef	__STDC__
void	*memdup (void *, int);
#else
void	*memdup ();
#endif
/*==================================================================*/
#endif
