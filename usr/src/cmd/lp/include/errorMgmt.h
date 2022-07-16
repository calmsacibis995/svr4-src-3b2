/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef	ERROR_MGMT_H
#define	ERROR_MGMT_H
/*==================================================================*/
/*
*/
#ident	"@(#)lp:include/errorMgmt.h	1.4"

#include	<errno.h>

/*----------------------------------------------------------*/
/*
*/
typedef	enum
{

	Fatal,
	NonFatal

}  errorClass;

typedef	enum
{

	Unix,
	TLI,
	XdrEncode,
	XdrDecode,
	Internal

}  errorType;
	

/*----------------------------------------------------------*/
/*
**	Interface definition.
*/
#ifdef	__STDC__

extern	void	TrapError (errorClass, errorType, char *, char *);

#else

extern	void	TrapError ();

#endif


/*----------------------------------------------------------*/
/*
*/
extern	int	errno;

/*==================================================================*/
#endif
