/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/ph_check.c	1.1"

/*
 * routine to compare requested phase values to the phase
 * table contents
 * 	-range requests that are mutually exclusive with the
 *	phase table will cause a FAIL return
 *
 *	-if beg_last or end_last flags are set, the beg_phsz and
 *	end_phsz members of the diagnostic request queue will
 *	be set to the last phase value before a PASS return
 */

#include <sys/firmware.h>
#include <sys/diagnostic.h>

unsigned char beg_last;		/* flag to begin diagnostics with
				 * last phase in table */

unsigned char end_last;		/* flag to end diagnostics with
				 * last phase in table */

extern unsigned char ph_set;	/* flag for phase option */
extern struct request p_req;	/* flag for diagnostics requests */
#define P_REQ (&p_req)

ph_check(phptr) 
struct phtab *phptr;	/* pointer to phase table */
{
unsigned char i;

	/* Scan phase table for last phase,
	 * check for out of range requests and
	 * save last phase number, if last_phsz set.
	 */

for ( i = 1; i <= MAXPHSZ; i++)
	if ((phptr+i-1)->type == END ) {

	/* is last phase requested as starting phase? */

		if (beg_last)
			P_REQ->beg_phsz = i-1;

	/* is beginning phase outside range of phase table? */

		if ( i <= P_REQ->beg_phsz)
			return(FAIL);

	/* is last phase requested as ending phase? */

		if (end_last)
			P_REQ->end_phsz = i-1;

	/* is ending phase outside range of phase table? */

		if (ph_set == ON && i <= P_REQ->end_phsz)
			return(FAIL);

	/* check possible phase range errors */
		if (P_REQ->beg_phsz > P_REQ->end_phsz)
			return (FAIL);

	/* leave the loop  - there's nothing more to do */

		break;
	}
return(PASS);
}
