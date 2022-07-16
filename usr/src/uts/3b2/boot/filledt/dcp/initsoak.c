/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/initsoak.c	1.2"

#include <sys/firmware.h>
#include <sys/diagnostic.h>

extern struct result result[];	/* results of the soak */

/* Routine to zero all the soak result queues */
void
initsoak()
{
unsigned long board;
long ph;

for(board=0; board < NUM_EDT ; board++){
	result[board].times=0;

	for(ph=0; ph < MAXPHSZ; ph++)
		result[board].phases[ph]=0;
}
}
