/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/dgn.c	1.2"

/*	3b2 Diagnostic Control Program    */

#include <sys/types.h>
#include <sys/boot.h>
#include <sys/sbd.h>
#include <sys/csr.h>
#include <sys/iu.h>
#include <sys/sit.h>
#include <sys/firmware.h>
#include <sys/edt.h>
#include <sys/diagnostic.h>
#include <sys/dsd.h>


extern void dgnerror(), sysreset(), rstrcons(), checkpwr();

extern struct edt edt[];        /* equipped device table structure */
extern unsigned long jump_reg[12];	/* storage for SETJMP/LONGJMP */

extern struct request p_req;    /* diagnostic request queue */
#define P_REQ (&p_req)


extern char option[];		/* array containing the option name     */
extern char passed[];		/* PASSED char string			*/
extern char failed[];		/* FAILED char string			*/
extern char not_run[];		/* NOT RUN char string			*/
extern unsigned char optno;	/* char containing the option number   */
extern unsigned char opt_type;	/* flag to diagnose an option type	*/


extern getdgn();		/* function to load regular and auxiliary
				 * diagnostic files */
extern getnum();		/* finds the board number in the edt    */
				/* from the option number and name      */
extern void cleanup();		/* clean up interrupts as possible      */


extern ex_phsz();		/* function which executes the boards   */
				/* phases according to the profile in   */
				/* request                              */

extern ph_check();		/* routine to check phase range */



/*    File Name variables    */

char filename[E_NAMLEN];		/*  name of phase file  */
char Dirname[DIRLEN];			/*  path name of diagnostic directory  */

int bootstartaddr;

/* Unexpected interrupt and exception handlers */
long diag_exc(),diag_int();

dgn()
{

int i;				/* utility variable                */
unsigned long board;            /* offset into edt of board        */
				/* currently being diagnosed       */
unsigned long phflag;		/* flag =1 if last phase failed    */
long retval;			/* return state for tests          */
struct phtab *phptr;            /* pointer to phase table of board */
				/* under diagnosis                 */


SBDWCSR->s_led = 0x01;    /* light the diagnostic LED */
retval = NTR;			/* default return value for diagnostics */

/* Set up the event handlers */
EXC_HAND = diag_exc;    
INT_HAND = diag_int;

/* Set the global board (option) flag */
OPTPTR = (long)(&board);

/* set initial value for EDT entry numbers */
board = 0;


/* Request to diagnose all units i.e., normal request */
if(P_CMDQ->b_type == AUTOBOOT)
	{

        for(board = 0;board < NUM_EDT; board++)   /* for all boards in system */
		{

		/* load regular and auxiliary diagnostic files */

		if ((i = getdgn(board)) == FAIL)
			return(FAIL);

		/* skip entry with unknown ID code */
		else if (i == NTR)
			continue;

		phptr = (struct phtab *)DOWNADDR;

		if ((phflag = ex_phsz(phptr)) == FAIL)
			return(FAIL);

		else if (phflag == PASS)
			retval = PASS;	/* save PASS if tests didn't fail or finish NTR */
		}
	SBDWCSR->c_led = 0x01;		/* turn off LED; diagnostics are ATP or NTR */
	return(retval);
	}


/* Console Request */
/* loop so long as EDT entries remain */

else while (board < NUM_EDT)
	{

	/* Scan for the board and option number in the EDT */
	if(getnum(&board) == FAIL)
		{
		/*
		 * device was not found; return FAIL for single device
	 	 * request or first attempt of device type request
		 */

		if (opt_type == OFF || optno == 0)
			return(FAIL);

		else break;		/*
					 * leave loop at last of
				 	 * device type
					 */
		}

		/*
		 * device was found;
	 	 * print heading for single device request or
	 	 * first unit in device type request
		 */

	if (opt_type == OFF || optno == 0)
		PRINTF("\n    <<<  DIAGNOSTIC MODE  >>>\n");

	/* skip diagnostics on a peripheral board with a console */

	if (FL_CONS->device != EDTP(board)->opt_slot || board == 0)
		{

		/* load regular and auxiliary diagnostic files */

		if ((i = getdgn(board)) == FAIL)
			{
			SBDWCSR->s_led = 0x01;    /* light the diagnostic LED */
			return(FAIL);
			}

		/* skip entry with unknown ID code */
		else if (i == NTR)
			{
			board++;
			continue;
			}

		phptr = (struct phtab *)DOWNADDR;

		/* does phase range intersect with phase table? */

		if (ph_check(phptr) == FAIL)
			{
			/* print error message */
			dgnerror(10);
			break;
			}
		SBDWCSR->s_led = 0x01;		/* light the diagnostic LED */
		phflag = ex_phsz(phptr);	/* execute the diags for the board */
		}

	else	/* set diagnostic return to NTR for peripheral console */
		{
		phflag = NTR;
		}

	/*  start the completion message */
	PRINTF("\n     %s %d (IN SLOT %d) DIAGNOSTICS ",
		EDTP(board)->dev_name,EDTP(board)->opt_num,EDTP(board)->opt_slot);

	if (phflag == FAIL)
		{
		retval = FAIL;
		PRINTF(failed);		/* finish FAIL message */
		if (P_REQ->uncond == OFF)
			return (FAIL);		/* return on FAIL if UCL not set */
		}
	else
		{
		if (phflag == PASS)
			{
			PRINTF(passed);	/* finish PASS message */
			if (retval != FAIL)
				retval = PASS;	/* save PASS as return value if no failures have occurred */
			}
		if (phflag == NTR)
			PRINTF(not_run);	/* finish NTR message */
		}


	if (!STRCMP(option,"\0"))
		board++;	/* diagnose all boards */

	else
		if (opt_type == ON)
			optno++;	/* next option number for option type test */
		else	break;		/* leave loop - option number tested */
	}

if(phflag != FAIL)
	SBDWCSR->c_led = 0x01;		/* put out the diagnostic LED */

/* return a value based on retval */

return( retval );
}

extern unsigned char exc_flag;	/* flag for exception handler */

long
diag_exc()
{
/* assign DGMON interrupt handler to cover any left-over interrupts */
INT_HAND = diag_int;

exc_flag = 1;			/* set flag to record exception */
cleanup();			/* clean up possible interrupts */
rstrcons();			/* restore peripheral console, if necessary */
(void) rstrbdev(P_CMDQ->b_dev);	/* restore floating boot device, if necessary */

if (P_CMDQ->b_type != AUTOBOOT)
	{
	IO=ON;
	/* print error message */
	dgnerror(4);

#ifdef EBUG
	/* print PC and PSW for non-auto boot runs */

	PRINTF("PC = 0x%x\n", OLDPC);
	PRINTF("PSW = 0x%x\n",OLDPSW);
#endif

	LONGJMP(jump_reg);	/* return to prompt loop */
	}

PRINTF(failed);		/* complete AUTOBOOT failure message */

sysreset();
}


extern unsigned char int_flag;	/* flag for interrupt handler */

long
diag_int()
{
extern char req_abort[];
extern char d_error[];
char i = 5;

/* assign DGMON exception handler to cover parity errors that generate both ints. & exc */
EXC_HAND = diag_exc;

checkpwr();	/* test for soft power: an interrupt, possibly from soft power timer, has been encountered */
int_flag = 1;	/* set flag to record interrupt */

asm("	INSFW &3,&13,&0xF,%psw");

cleanup();			/* clean up possible interrupts */
rstrcons();			/* restore peripheral console, if necessary */
(void) rstrbdev(P_CMDQ->b_dev);	/* restore floating boot device, if necessary */

if (P_CMDQ->b_type != AUTOBOOT)
	{
	IO=ON;
	/* print error message */
	PRINTF(d_error, i);
	PRINTF("UNEXPECTED DIAGNOSTIC INTERRUPT\n");
	PRINTF(req_abort);

#ifdef EBUG
	/* print PC and PSW for non-auto boot runs */

	PRINTF("PC = 0x%x\n",OLDPC);
	PRINTF("PSW = 0x%x\n",OLDPSW);
#endif

	LONGJMP(jump_reg);	/* return to prompt loop */
	}

PRINTF(failed);
sysreset();
}
