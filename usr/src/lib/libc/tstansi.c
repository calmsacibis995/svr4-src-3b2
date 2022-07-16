/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc:tstansi.c	1.1"

/* test if we have an ANSI compiler */
#if !defined(__STDC__)
	/* include a file that does not exist so that
	 * a non-zero value is returned from the compiler */
#include "TESTANSI"
#endif
