/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/lpNet/debug.c	1.1"

#ifndef	DEBUG
#define	DEBUG
#endif

#include	"debug.h"

int	_nestCount	= 0;
char	*_Unknownp	= "?";
char	*_Nullp		= "Null";
char	*_FnNamep	= "?";
char	*_FnNames [_MAX_NEST_COUNT];
FILE	*_Debugp	= NULL;
