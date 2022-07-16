/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/fillcntl.c	1.3"

/*	Control Routine for 3B2 EDT Completion Program	*/

/* routine determines nature of its call, system reset vs. demand */
/* for system reset:
 *		EDT completed
 *		Console located, if possible
 *		EDTFILL returns completion PASS/FAIL code
 */
/* for demand call:
 *		EDT completed
 *		Console located, if possible
 *		EDTFILL returns completion PASS/FAIL message
 *
 * Boot device specified in structure to which argument
 * points also.  See boot.h.	*/

#include <sys/types.h>
#include <sys/boot.h>
#include <sys/sbd.h>
#include <sys/csr.h>
#include <sys/firmware.h>
#include <sys/iu.h>
#include <sys/sit.h>
#include <sys/edt.h>
#include <sys/iobd.h>
#include <sys/diagnostic.h>
#include <sys/dsd.h>
#include <edt_def.h>

extern edt_fill();			/* routine for completing EDT */
extern xbusgen();			/* routine for completing XEDT */

extern killbdev();			/* routine for stopping floating boot devices */
extern cleanup();			/* clear interrupts where possible */
extern char option[];			/* option (device) name */
extern unsigned long jump_reg[];	/* storage for SETJMP/LONGJMP */
extern long fill_int(),fill_exc();	/* diagnostic interrupt/exception handlers */
unsigned char exc_flag;			/* flag for exception handler */
unsigned char int_flag;			/* flag for interrupt handler */
char failed[] = "\nEDT FILL FAILED\n";

long (*fst_int_hand)();			/* storage for previous interrupt handler */
long (*fst_exc_hand)();			/* storage for previous exception handler */

main ()
{
unsigned long i;
long retval;			/* return value flag */

fst_int_hand = INT_HAND;	/* save incoming interrupt handler */
fst_exc_hand = EXC_HAND;	/* save incoming exception handler */
INT_HAND = fill_int;		/* functions to handle spurious interrupts */
EXC_HAND = fill_exc;		/* and exceptions */
RUNFLG = FATAL;			/* set run flag to to default "return" value */
retval = FAIL;			/* set return flag to default - FAIL */

/*
 * Code to deal with exceptions and LONGJUMPs.
 * Define return for exceptions and environment LONGJMPs.
 * Respond to exceptions and LONGJMPs, based on AUTO or DEMAND boot.
 */

exc_flag = 0;
int_flag = 0;

EXCRET;
if (SETJMP(jump_reg) != 0 || exc_flag == 1)
	{
	/* Exception or interrupt occurred */
	if (P_CMDQ->b_type == AUTOBOOT)
		{
		/*
		 * Exception/interrupt hit during AUTOBOOT.
		 * Reset the system.
		 */
		PRINTF(failed);
		sysreset();
		}
	else	{
		/*
		 * Exception/interrupt during DEMANDBOOT
		 * Error message already printed; back to boot code.
		 */
		softlanding();
		}
	}

BRK_INH(ON);			/* set break inhibit */

	/* set the output control */
	IO = ON;
	if (P_CMDQ->b_type != AUTOBOOT)
		{
		PRINTF("\nBEGIN FILLING EDT\n");	/* start output message */
		}
	else	{
		IO = OFF;
		}

	/* Reset all peripheral boards, if not console or boot device */
	for (i = 1; i < NUM_EDT; i++)
		if ((FL_CONS->cons_found != ON || FL_CONS->device != EDTP(i)->opt_slot) &&
			((P_CMDQ->b_dev >> 4) != EDTP(i)->opt_slot))
			SL_RESET(i) = 0;


	/*	setup routine for init file system */
	if (setup() != PASS)
		{
		fillerror(0);	/* print error messsage */
		sysreset();	/* file sys setup failed; reset system */
		}

	/* Complete EDT */
	if (edt_fill() == PASS)
		{
		/* EDT completed O.K.; set retval to "PASS" */
		retval = PASS;
		xbusgen();
		}

	/*
	 * This test on FLOAT is used to suppress the floating console search for
	 * all cases until both the 3B2 firmware and software support floating
	 * console.  This test and the contained code must be removed at that time.
	 */
#ifndef FLOAT				/* FLOAT test */
	FL_CONS->cons_found = OFF;	/* FLOAT test */
#endif					/* FLOAT test */

	/* locate console terminal if not already known */
	/* RUNFLG not affected by console location results */

	if (FL_CONS->cons_found != ON && loc_cons() != PASS)
		{
		/* send message in case terminal present w/o DTR */
		PRINTF("COULD NOT FIND CONSOLE\n");
		}

	IO = ON;
	if (retval == PASS)
		{
		if (P_CMDQ->b_type != AUTOBOOT)
			PRINTF("\nEDT SUCCESSFULLY COMPLETED\n");
		}

	else	{
		if (P_CMDQ->b_type != AUTOBOOT)
			/* EDT incomplete or in error; print error message */
			fillerror(1);
		else
			{
			/* print failure message */
			PRINTF(failed);
			}
		}

	if (P_CMDQ->b_type != AUTOBOOT)
		{
		PRINTF("\nCONSOLE VALUES:\n");
		PRINTF("slot = %d, port = %d\n",FL_CONS->device,FL_CONS->port);
		PRINTF("cflags = 0x%x\n",FL_CONS->cflags);
		}

/* back to SBD PROM boot code */
softlanding();
}

/*
 * any landing that you can walk away from is a good one -- set RUNFLG
 * to REENTRY since no exceptions or interrupts have interfered
 */

softlanding()
{
RUNFLG = REENTRY;
INT_HAND = fst_int_hand;	/* restore incoming interrupt handler */
EXC_HAND = fst_exc_hand;	/* restore incoming exception handler */

killbdev(P_CMDQ->b_dev);	/* Shut down floating boot devices */

LONGJMP(0);	/* return value passed with RUNFLG */
}


long fill_exc()
{
exc_flag = 1;			/* set exception flag */
cleanup();			/* clean up possible interrupt sources */
rstrcons();			/* restore peripheral console, if necessary */
rstrbdev(P_CMDQ->b_dev);	/* restore floating boot devices */

if (P_CMDQ->b_type != AUTOBOOT)
	{
	IO=ON;
	/* print error message */
	fillerror(4);

#ifdef EBUG
	/* print PC and PSW for non-auto boot runs */

	PRINTF("PC = 0x%x\n",OLDPC);
	PRINTF("PSW = 0x%x\n",OLDPSW);
#endif

	LONGJMP(jump_reg);
	}

PRINTF(failed);		/* print AUTOBOOT failure message */
sysreset();
}


long fill_int()
{
extern char f_error[];
char i = 5;
int_flag = 1;

checkpwr();	/* test for soft power: an interrupt, possibly from soft power timer, has been encountered */


cleanup();			/* clean up possible interrupt sources */
rstrcons();			/* restore peripheral console, if necessary */
rstrbdev(P_CMDQ->b_dev);	/* restore floating boot devices */

if (P_CMDQ->b_type != AUTOBOOT)
	{
	IO=ON;
	/* print error message */
	PRINTF(f_error, i, "%s", "UNEXPECTED INTERRUPT\n");

#ifdef EBUG
	/* print PC and PSW for non-auto boot runs */

	PRINTF("PC = 0x%x\n",OLDPC,
		"PSW = 0x%x\n",OLDPSW);
#endif

	LONGJMP(jump_reg);
	}

PRINTF(failed);		/* print AUTOBOOT failure message */
sysreset();
}
