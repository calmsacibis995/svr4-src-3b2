/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lp:lib/bsd/global.c	1.2"

#include "msgs.h"

char	*Lhost;		/* Local host name */
char	*Rhost;		/* Remote host name */
char	*Name;		/* Program name */
char	*Printer;	/* Printer name */
char	 Msg[MSGMAX];	/* Message buffer */
