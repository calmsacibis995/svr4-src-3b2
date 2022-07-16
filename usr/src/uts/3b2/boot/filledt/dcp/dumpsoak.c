/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/dumpsoak.c	1.2"

#include <sys/firmware.h>
#include <sys/edt.h>
#include <sys/diagnostic.h>

extern struct result result[];	/* results of the soak */

/* Routine to print all the soak result queues */

void
dumpsoak(loboard,hiboard)
unsigned long loboard,hiboard;      /* variables describing the begin and end of */
			   /* the soak request.  i.e. one brd or all    */
{
unsigned long board;   /* temp variable for finding the board */
long ph;      /* temp variable for finding the phase */
long brd_rslt;	/* ATP/STF flag for results of each board */

BRK_INH(OFF);	/* clear break inhibit & enable X-on/X-off */

for(board=loboard; board < hiboard; board++)
	{
	if (result[board].times == 0)
		continue;	/* skip report if device not soaked */

	if (PRINTF("\n\n%s BOARD IN SLOT %d:  COMPLETE RUNS = %ld\n",
		EDTP(board)->dev_name,EDTP(board)->opt_slot,result[board].times) < 0)
		goto dumpdone;

	brd_rslt = PASS;	/* initial value for ATP soak */
	if (PRINTF("PHASE FAILURES:\n") < 0)
		goto dumpdone;

	/* loop through soak result structure, listing failures */

	for(ph=0; ph < MAXPHSZ; ph++)
		{
		/* Did any phases fail? */

		if(result[board].phases[ph] != 0)
			{
			if (PRINTF("    %d  FAILED %ld TIMES\n",
				(ph+1),result[board].phases[ph]) < 0)
				goto dumpdone;
			brd_rslt = FAIL;
			}

		/* print "none" under heading if board is ATP */

		}
	if (brd_rslt == PASS)
		if (PRINTF("    NONE.\n") < 0)
			goto dumpdone;
	}
if (PRINTF("\n") < 0)
	goto dumpdone;

dumpdone:

BRK_INH(ON);	/* re-set break inhibit & turn off X-on/X-off */
}
