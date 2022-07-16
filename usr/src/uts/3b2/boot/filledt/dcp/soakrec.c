/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/soakrec.c	1.1"

#include <sys/diagnostic.h>
#include <sys/edt.h>

/* the storage for the soak record is allocated here, it is not */
/* necessary to initialize it, so it is outside bss section     */

struct result result[MAX_IO + 1];
