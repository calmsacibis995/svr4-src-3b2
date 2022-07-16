/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/dcpcntl.c	1.2"

/*	Control Routine for 3B2 Diagnostic Control Program	*/

/* routine determines nature of its call, system reset vs. demand */
/* for system reset:
 *		EDT completed
 *		dgn called to run NORMAL phases for each slot
 *		DCP returns completion PASS/FAIL code
 */
/* for demand call:
 *		command prompt sent to console
 *		command parsed
 *		command executed
 *		next prompt sent
 *
 * Boot device and file name specified in structure to which argument
 * points also.  See boot.h.	*/

#include <sys/boot.h>
#include <sys/sbd.h>
#include <sys/csr.h>
#include <sys/firmware.h>
#include <sys/edt.h>
#include <sys/iobd.h>
#include <sys/diagnostic.h>
#include <sys/dsd.h>
#include <edt_def.h>

extern dgn();				/* routine to execute DGN commands */
extern parse();				/* routine to parse console commands */
extern void soak ();			/* routine to execute SOAK commands */

extern ph_list();			/* routine for displaying phase tables */
extern void killbdev();			/* routine for stopping floating boot devices */

extern unsigned char cmnd_code;		/* DGN, SOAK, H, or Q command code */
extern unsigned char optno;		/* option (device) number */
extern char option[];			/* option (device) name */

extern unsigned char opt_type;		/* option (device) type diagnostics flag */

extern unsigned long jump_reg[12];	/* storage for SETJMP/LONGJMP */

static void help();
extern long diag_int(),diag_exc();	/* diagnostic interrupt/exception handlers */
extern void dgnerror(), sysreset();

static char cmnd_str[DIRLEN];		/* string for console diagnostic requests */
struct request p_req;			/* diagnostic request flags */
#define P_REQ (&p_req)

long (*fst_int_hand)();			/* storage for previous interrupt handler */
long (*fst_exc_hand)();			/* storage for previous exception handler */
unsigned char exc_flag;			/* exception handler flag */
unsigned char int_flag;			/* interrupt handler flag */

char passed[] = "PASSED\n\n";		/* PASSED string */
char failed[] = "FAILED\n\n";		/* FAILED string */
char not_run[] = "NOT RUN\n\n";		/* NOT RUN string */

main ()
{
unsigned long i,j;
asm("	SUBW3 %fp,%sp,0(%fp)");		/* save local var area size */
asm("	MOVW &0x2003094,%sp");		/* set up stack pointer */
asm("	ADDW2 0(%fp),%sp");		/* rest. local var area size */
asm("	MOVW &0x2003094,%fp");		/* set up frame pointer */
asm("	MOVW &0x2003094,%ap");		/* set argument pointer */
asm("	MOVW &0x2003094,0xc(%pcbp)");	/* set up stack upper/lower bounds */
asm("	MOVW &0x20037ec,0x10(%pcbp)");

fst_int_hand = INT_HAND;	/* save incoming interrupt handler */
fst_exc_hand = EXC_HAND;	/* save incoming exception handler */
INT_HAND = diag_int;		/* functions to handle spurious interrupts */
EXC_HAND = diag_exc;		/* and exceptions */

/*
 * Code to deal with exceptions and LONGJUMPs before any diagnostic
 * tests may have run.
 * Define return for exceptions and environment LONGJMPs.
 * Respond to exceptions and LONGJMPs, based on AUTO or DEMAND boot.
 */

exc_flag = 0;		/* clear exception flag */
int_flag = 0;		/* clear interrupt flag */

EXCRET;
if (SETJMP(jump_reg) != 0 || exc_flag == 1)
	{
	/* Exception or interrupt occurred */
	if (P_CMDQ->b_type == AUTOBOOT)
		{
		/*
		 * Exception/interrupt hit, possibly before diagnostic tests
		 * during AUTOBOOT.
		 * Reset the system.
		 */
		sysreset();
		}
	else	{
		/*
		 * Exception/interrupt hit before normal SETJMP environment
		 * defined for DEMANDBOOT
		 * Error message already printed; back to boot code.
		 */
		LONGJMP(0);
		}
	}

BRK_INH(ON);			/* set inhibit for break */

	/* set the output control */

	IO = ON;
	if (P_CMDQ->b_type == AUTOBOOT)
		{
		PRINTF("\nDIAGNOSTICS ");	/* start output message */
		IO = OFF;
		}

	/* Reset all peripheral boards, if not console or boot device */
	for (i = 1; i < NUM_EDT; i++)
		if ((FL_CONS->device != EDTP(i)->opt_slot) &&
			((P_CMDQ->b_dev >> 4) != EDTP(i)->opt_slot))
			SL_RESET(i) = 0;

	/*	setup routine for init file system */

	if (setup() != PASS)
		{
		dgnerror(0);	/* print error message */
		sysreset();	/* file sys setup failed; reset system */
		}

/* determine type of boot and memory device from command "queue" */

	if (P_CMDQ->b_type == AUTOBOOT)
		{
		/* set diagnostic control parameters for AUTOBOOT */

		P_REQ->dmnd = OFF;
		P_REQ->uncond = OFF;
		P_REQ->rpt = OFF;
		P_REQ->rept_cnt = 1;
		P_REQ->beg_phsz = 1;
		P_REQ->end_phsz = MAXPHSZ;
		P_REQ->partl = OFF;

		optno = 0;
		opt_type = OFF;
		option[0] = '\0';

		/* Run normal diagnostics for all devices */

 		if ((j = dgn()) == PASS)
			{
			IO = ON;		/* complete diagnostic message */
			PRINTF(passed);
			RUNFLG = REENTRY;
			}
 		else	if (j == NTR)
			{
			IO = ON;		/* complete diagnostic message */
			PRINTF(not_run);
			RUNFLG = REENTRY;
			}
 		else
 			{
			IO = ON;		/* complete diagnostic message */
			PRINTF(failed);
			RUNFLG = FATAL;
 			}

		INT_HAND = fst_int_hand;	/* restore incoming interrupt handler */
		EXC_HAND = fst_exc_hand;	/* restore incoming exception handler */

		killbdev(P_CMDQ->b_dev);	/* shut down floating boot devices */
		LONGJMP(0);
		}
	else
		{
		/* DCP booted by DEMAND */


		PRINTF("\n\n        3B2 DIAGNOSTIC MONITOR\n");

/* establish a return point for exceptions & interrupts from cons. commands */
		EXCRET;				/* define return for exceptions */
		SETJMP(jump_reg);		/* define return for DEMANDBOOT LONGJMPs */
		exc_flag = 0;			/* be sure exception flag is clear */
		int_flag = 0;			/* be sure interrupt flag is clear */

		cmnd_code = H;	/* initial value for while loop */
		while (cmnd_code != Q)
			/* take commands until Q(uit) */ 
			{
			BRK_INH(ON);	/* set inhibit for break */

			while ( GETSTAT());	/* flush UART buffer */

			PRINTF("DGMON > ");	/* send command prompt */

			for (i = 0; i < DIRLEN; i++ )	/* clear command string */
				cmnd_str[i] = '\0';

			GETS(cmnd_str);		/* get string */

			/* skip command case test if parse failed */

			if (parse(cmnd_str) == PASS)
				{
				switch (cmnd_code)
					{
					case DGN:
						dgn();
						break;

					case H:
						help();
						break;

					case SOAK:
						soak();
						break;
					case S:
						DISPEDT();
						break;

					case L:
						ph_list();
						break;

					default:
						break;
					}
				}
			}
		}

/* (re)enter firmware upon departing the Diagnostic Monitor */

	RUNFLG = REENTRY;

	INT_HAND = fst_int_hand;	/* restore incoming interrupt handler */
	EXC_HAND = fst_exc_hand;	/* restore incoming exception handler */

	killbdev(P_CMDQ->b_dev);	/* shut down floating boot devices */
	LONGJMP(0);
}




/* help routine:  provides a list of commands and formats that
 * the DGMON will accept */


static
void
help()
{
char i;
char *msg[10];

BRK_INH(OFF);	/* turn off break inhibit */

/* assign message pointers for table */

msg[0] = "       3B2\n";
msg[1] = "    DIAGNOSTIC\n";
msg[2] = "     COMMANDS   OPTIONS                                 DESCRIPTION\n";
msg[3] = "    ==========  =======                                 ===========\n";
msg[4] = "        DGN     [DEVICE [DEVICE # | REP=? | PH=?-? |    DIAGNOSE DEVICE(S)\n";
msg[5] = "                                    UCL | SOAK ]]\n\n";
msg[6] = "       H(ELP)   (NONE)                                  PRINT HELP MENU\n\n";
msg[7] = "       L(IST)   DEVICE                                  LIST DEVICE PHASE TABLE\n\n";
msg[8] = "       Q(UIT)   (NONE)                                  EXIT DGMON\n\n";
msg[9] = "       S(HOW)   (NONE)                                  SHOW EDT\n\n";

/* print strings character by character, testing for BRK */

for (i = 0; i < 10; i++)
	{
	while (*msg[i] != 0)
		if (PRINTF("%c",*msg[i]++) < 0)
			goto helpdone;

	HWCNTR(1);
	}

helpdone:
BRK_INH(ON);	/* restore break inhibit */
return;
}
