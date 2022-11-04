/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/values-Xc.c	1.2"

#include <math.h>

/* variables which differ depending on the
 * compilation mode
 *
 * Strict ANSI mode
 * This file is linked into the a.out immediately following
 * the startup routine if the -Xc compilation mode is selected
 */

 const enum version _lib_version = strict_ansi;
