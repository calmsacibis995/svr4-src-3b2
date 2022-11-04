/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/ph_list.c	1.2"

/*	3b2 Phase Table List Program    */

#include <sys/sbd.h>
#include <sys/firmware.h>
#include <sys/edt.h>
#include <sys/diagnostic.h>

extern void dgnerror(), sysreset();

extern struct edt edt[];        /* equipped device table structure */

extern char option[];		/* array containing the option name     */
extern char *strcpy();
extern unsigned char optno;	/* char containing the option number   */
extern unsigned char opt_type;	/* flag to diagnose an option type	*/


extern ld_sbd();		/* function to load regular
				 * diagnostic files */
extern getnum();		/* finds the board number in the edt    */
				/* from the option number and name      */

static char norml[] =    "NORMAL     ";
static char demand[] =   "DEMAND     ";
static char interact[] = "INTERACTIVE";

#define SHRT_PG 15
#define LONG_PG 18

ph_list()
{

unsigned long board;            /* offset into edt of board        */
				/* currently being diagnosed       */
struct phtab *phptr;            /* pointer to phase table of board */
				/* under diagnosis                 */

char phtype[16];
short i,j,k;

/* turn off break inhibit */

BRK_INH(OFF);

/* is board specified in EDT? */

if (!getnum(&board))
	return(FAIL);

IO = ON;

if (setup() != PASS) {
	dgnerror(0);	/* print error message */
	sysreset();	/* file sys setup failed; reset system */
}

/* load regular diagnostic file */

if (ld_sbd() == FAIL)
	{
	return(FAIL);
	}

phptr = (struct phtab *)DOWNADDR;

/* find end of phase table */

for (i=0; i < MAXPHSZ && (phptr +i)->type != END; i++);

/* print out table contents on screen */

for (j = 0; j < i; j++)
	{

		/* print heading for new "pages" if there is too much for one page */

	if ((i - j > LONG_PG - SHRT_PG) && (j % SHRT_PG == 0))
		{
		if (PRINTF("DIAGNOSTIC PHASE TABLE FOR %s\n",option) < 0)
			goto listdone;
		if (PRINTF("\nPHASE #     PHASE TYPE        PHASE DESCRIPTION\n") < 0)
			goto listdone;
		if (PRINTF("========    ==========        =================\n") < 0)
			goto listdone;
		}

	/* detrmine phase type for each entry */

	if ((phptr + j)->type == NORML)
		(void) strcpy(phtype,norml);
	else if ((phptr + j)->type == DEMAND)
		(void) strcpy(phtype,demand);
	else if ((phptr + j)->type == INTERACT)
		(void) strcpy(phtype,interact);

	if (PRINTF("  %2d        %s       %s\n",j+1,phtype,(phptr + j)->title) < 0)
		goto listdone;

		/* print continue message for new "pages" if there is too much for one page */

	if ((i - j -1 > LONG_PG - SHRT_PG) && (j % SHRT_PG == SHRT_PG -1) && (j != 0))
		{
		if (PRINTF("\nENTER ANY KEY TO CONTINUE") < 0)
			goto listdone;
		while (!GETSTAT());

		for (k =0; k < LONG_PG - SHRT_PG; k++)
			if (PRINTF("\n") < 0)
				goto listdone;
		}
	}

PRINTF("\n");

listdone:

BRK_INH(ON);
return(PASS);
}
