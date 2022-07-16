/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/soak.c	1.2"

#include <sys/types.h>
#include <sys/boot.h>
#include <sys/firmware.h>
#include <sys/edt.h>
#include <sys/diagnostic.h>
#include <sys/sbd.h>
#include <sys/csr.h>
#include <sys/dsd.h>

extern ex_phsz();       /* routine to execute a phase given info */
			/* in request queue */

extern unsigned char optno;	/* the option number into the edt */
extern char option[];		/* the option name from the edt   */
extern unsigned char opt_type;	/* flag to diagnose option type */
extern unsigned long soak_rept;	/* repeat count for soak command */

extern struct result result[];	/* queue for diagnostic failure recordings */

extern ph_check();		/* routine to check range table range */
extern void dumpsoak();		/* routine to dump the failure information */

extern void initsoak();		/* routine to initialize the result queues */
extern long diag_int(),diag_exc();	/* diagnostic interrupt/exception handlers */
extern void dgnerror();

extern struct request p_req;	/* diagnostic request flags */
#define P_REQ (&p_req)


extern getdgn();		/* function to load regular and auxiliary
				 * diagnostic files */
extern getnum();		/* routine to get slot number and phase
				 * file name information from ETD */

void
soak()
	{
struct phtab *phptr;
unsigned long board,loboard,hiboard;     /* board number in edt and 2 dummy   */
				/* variables for loop limits         */
char kill;     			/* flag saying a char was typed at   */
				/* keyboard, thus ending the session */

unsigned long save_beg;		/* variable to save 1st phase value  */
unsigned long save_end;		/* variable to save last phase value */
long times;			/* loop variable */
int i;				/* utility variable */


OPTPTR = (long)(&board);	/* set global board device pointer   */
INT_HAND = diag_int; 		/* set up for spurious interrupts or */
EXC_HAND = diag_exc;		/* exceptions */

SBDWCSR->s_led = 0x01;   /* light Diagnostic LED */

initsoak();			/* clear out the result queues       */

save_beg = P_REQ->beg_phsz; 	/* save beginning phase number */
save_end = P_REQ->end_phsz;	/* save ending phase number */

/* was soak the only keyword specified? if so soak all. */
if(!STRCMP(option,"\0"))
	{
	loboard=0;
	hiboard=NUM_EDT;
	}
else	{
	/* search for device(s) in EDT */
	/* getnum starts searching at "board" = 0 for devices
	 * and puts saves the found value in board */
	if(!getnum(&board))
		return;
	
	/* it does exist so set up range of boards for soak */
	else	{
		if (opt_type)
			{
			loboard = board;	/* start with first of type found */
			hiboard = NUM_EDT;	/* check all boards to end */
			}

		else
			{
			loboard = board;	/* single device soak */
			hiboard = board + 1;
			}
		}
	}

	kill = 0;		/* flag which says kill request */

times = 0;	/* clear counter for while loop */
while(!kill)
	{

	/* for the number of boards to be soaked */
	for(board = loboard; (board < hiboard) & !kill; board++)
		{
		/* skip any peripheral board that serves as the console */
		if (FL_CONS->device == EDTP(board)->opt_slot && FL_CONS->cons_found && board > 0)
			continue;

		if (opt_type && STRCMP(option,EDTP(board)->dev_name))
			/* EMPTY */ ;
	/* skip boards if no option match during option type soak */

		else	{

	/* options match for option type request OR all boards requested
	 * OR a single board requested (found by getnum()) */


		/* load regular and auxiliary diagnostic files */

		if ((i = getdgn(board)) == FAIL)
			return;

		/* skip entries with unknown ID codes */
		else if (i == NTR)
			continue;

		phptr = (struct phtab *)DOWNADDR;

	/* check for phase requests that are mutually exlusive
	 * with phase table
	 */

	/* save beginning phase number */
 		P_REQ->beg_phsz = save_beg; 
 	/* restore initial ending phase number */
		P_REQ->end_phsz = save_end;

		if ( !ph_check(phptr) )
			{
			IO = ON;
			/* invalid phase request;  print error message */
			dgnerror(10);
			IO = OFF;
			kill = 1;
			break;
			}

		result[board].times++;

		/* Set up for single phase execution, starting with save_beg. */

		/* P_REQ->beg_phsz = save_beg; */
		P_REQ->end_phsz = P_REQ->beg_phsz;

	/* call single phases until END phase or last requested (save_end) phase */
		while(((phptr + ((P_REQ->beg_phsz)-1))->type != END)
			&& P_REQ->beg_phsz <= save_end)
			{
			/* skip the interactive phases */
			if((phptr + ((P_REQ->beg_phsz)-1))->type == INTERACT)
				/* EMPTY */ ;
				
			/* increment the failing phase counter if */
			/* FAIL phase result       */
			else if(!ex_phsz(phptr))
				(result[board].phases[P_REQ->beg_phsz-1])++;

			/*  Check for Console Abort Request  */
			kill = GETSTAT();
			if(kill != 0)
				break;

			else
			/* increment phase index */
				{
				(P_REQ->end_phsz)++;
				P_REQ->beg_phsz=P_REQ->end_phsz;
				}
			}
		}
	}
	/* end of slot sweep loop */

	times++;		/* increment while loop counter */
	/* If specified, has repeat range for soak been exceeded? */
	if (soak_rept > 0 && soak_rept <= times)
		{
		kill = 1;	/* repeat limit reached; set flag to leave while loop */
		}
   }
IO = ON;				/* re-enable IO  */
dumpsoak(loboard,hiboard);		/* dump the failure information */
SBDWCSR->c_led = 0x01;		/* turn off LED */
	}
