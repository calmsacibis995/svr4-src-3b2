/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/execphsz.c	1.2"

#include <sys/types.h>
#include <sys/csr.h>
#include <sys/sbd.h>
#include <sys/iu.h>
#include <sys/sit.h>
#include <sys/edt.h>
#include <sys/firmware.h>
#include <sys/iobd.h>
#include <sys/diagnostic.h>
#include <sys/dsd.h>
#include <sys/boot.h>
#include <edt_def.h>

/* Function to take the diagnostics whose phase table is pointed to 
* by ph_ptr and execute them according to the information in request
* default flags give diagnostics for unit with no output or input
* ucl flag will keep going on failure
* rpt flag will specify a repeat count
* dmnd flag must be set to enable input and output.
* partl flag must be set to attempt to run demand phases.
* 
* IO is enabled by the following options on the command line,
*      ucl, partl
* 
* IO is not affected (left off unless above is specified)
*	by rpt 
*/

extern void rstrcons(), checkpwr(), sysreset();
extern struct vectors vectors;  	/* vector table from firmware.h */
extern long diag_int(), diag_exc(); 	/* the diagnostic exception handlers */
extern fw_pump(); 			/* pump routine for peripheral console */
extern struct request p_req;		/* diagnostic request flags */
extern unsigned char exc_flag;		/* exception flag */
extern unsigned char cmnd_code;		/* command code from parse routine */
extern char failed[];			/* FAILED string */

#define P_REQ (&p_req)

ex_phsz(ph_ptr)
struct phtab *ph_ptr;
{

long rep,ph,old_IO,temp,state;

/*
 * Code to deal with exceptions of any diagnostic phases that have dangling
 * returns for excptions.
 * Define return for exceptions.
 * Respond to exceptions and LONGJMPs, based on AUTO or DEMAND boot.
 */

exc_flag = 0;
EXCRET;
if (exc_flag == 1)
	{
	/* Exception occurred */
	if (P_CMDQ->b_type == AUTOBOOT)
		{
		/* Exception hit during AUTOBOOT.   Reset the system. */
		PRINTF(failed);
		sysreset();
		}
	else	{
		/*
		 * Exception hit during DEMANDBOOT testing.
		 * Error message already printed; back to prompt code
		 * using direct call of DGMON's exception handler function.
		 */
		diag_exc();
		}
	}
exc_flag = 1;	/*
		 * Set flag before running each phase.
		 * Preceeding test will catch use of EXCRET even when
		 * defined handler does not directly use flag.
		 */

state = NTR;     /* variable describing the final state of the diag request */
old_IO = IO;	/* save incoming IO value */
if(!P_REQ->dmnd)    /* turn off the IO if this is a non-demand request */
	IO = OFF;

asm("	INSFW	&3,&13,&0x0,%psw");	/* (re)set IPL */
BRK_INH(ON);				/* set inhibit for break */

/*
 * Peripheral console board diagnosed only on auto boot and must remain
 * available for error messages until it is actually diagnosed.
 * Reset it now.
 */
if (P_CMDQ->b_type == AUTOBOOT && FL_CONS->device == EDTP(OPTION)->opt_slot
	&& OPTION != 0)
	SL_RESET(OPTION) = 0;

/*
 * The peripheral boot device must remain available for disk access
 * until it is actually diagnosed.
 * Reset it now.
 */
if ((P_CMDQ->b_dev >> 4) == EDTP(OPTION)->opt_slot && OPTION != 0)
	SL_RESET(OPTION) = 0;

/* for the number of times to repeat */

for(rep = 1; rep <= P_REQ->rept_cnt; rep++)
	{

      /* for the correct number of phases */
      for (ph = P_REQ->beg_phsz; (ph <= P_REQ->end_phsz) && ((ph_ptr+ph-1)->type != END); ph++)
		{
	
/* skip DEMAND and INTERACTIVE phases if partial diagnostic flag not set */

		if((((ph_ptr+ph-1)->type == DEMAND) | ((ph_ptr+ph-1)->type == INTERACT))
			& (!P_REQ->partl))
			/* EMPTY */
			;
		else
			{
			PRINTF("\n");

			if ((temp = ((ph_ptr+ph-1)->phase)(P_REQ->dmnd)) == FAIL)
				{
				state = FAIL;

				if(!(P_REQ->uncond))
					goto breakout;
				}

			/* set state flag to PASS if no failures occurred */
			else	if (state != FAIL && temp == PASS)
					state = PASS;

			/*
			 * If this is not a soak, purge console's UART of any "type ahead" characters
			 * that would be taken as input for a following interactive phase.
			 * Do nothing for soaks, keyboard input is a "stop" signal to the soak routine.
			 */

			if (cmnd_code != SOAK)
				{
				while (GETSTAT());
				GETSTAT();
				}
			}
	      /* re-establish dgmon interrupt/exception routines */
	      INT_HAND = diag_int;
	      EXC_HAND = diag_exc;

		/* clear soft power inhibit if left set by a phase; check for soft power request */
		SPWR_INH = OFF;
		checkpwr();

		BRK_INH(ON);	/* set inhibit for break */

		/* (re)set IPL */
		asm("	INSFW	&3,&13,&0x0,%psw");
		}
	}
breakout: IO = old_IO;

	/* reset peripheral boards at end of diagnostic */

	if ( !(OPTION) )
		SL_RESET(OPTION) = 0;

	/* re-establish dgmon interrupt/exception routines */

      INT_HAND = diag_int;
      EXC_HAND = diag_exc;

	/* clear soft power inhibit if left set by a phase; check for soft power request */
	SPWR_INH = OFF;
	checkpwr();

	BRK_INH(ON);	/* set inhibit for break */

	/* (re)set IPL */
	asm("	INSFW	&3,&13,&0x0,%psw");

	/* if a peripheral board has the console terminal, it will be diagnosed
	 * only in the AUTOBOOT sequence.
	 * The console interface through it must be restored after its tests
	 * are done.
	 */

	rstrcons();
	
	/* Restore integral floppy or peripheral boot devices.  */

	if (rstrbdev(P_CMDQ->b_dev) == FAIL)
		state = FAIL;	/* return FAIL if peripheral boot device not restored */

return(state);
}
