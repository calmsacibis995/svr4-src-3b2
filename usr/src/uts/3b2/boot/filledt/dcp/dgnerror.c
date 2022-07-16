/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/dgnerror.c	1.2"

/* error message routine for dgmon */

#include <sys/firmware.h>
#include <sys/edt.h>
#include <sys/diagnostic.h>

extern char Dirname[];			/* full pathname of a file */
extern char option[];			/* name of device to be diagnosed */
extern unsigned char *start_ptr;	/* pointer to start of input string */

char check_edt[] = "CHECK EDT.\n\n";
char req_abort[] = "DIAGNOSTIC REQUEST ABORTED.\n\n";
static char req_reenter[] = "RE-ENTER REQUEST.\n\n";
static char req_retry[] = "\nRETRY REQUEST.\n\n";
char d_error[] = "\nDIAGNOSTIC MONITOR ERROR 1-%02d:\n";

void
dgnerror(dgnerrno)
int dgnerrno;
{
int i;

PRINTF(d_error,dgnerrno);

switch (dgnerrno)
	{
	case 0:
		PRINTF("FILE SYSTEM IS INACCESSIBLE.\nCONTROL WILL RETURN TO MAINTENANCE CONTROL PROGRAM.\n\n");
		break;

	case 1:
		PRINTF("UNKNOWN ID CODE 0x%x FOR DEVICE IN SLOT %d.\n",
			EDTP(OPTION)->opt_code,EDTP(OPTION)->opt_slot);
		PRINTF("NO DIAGNOSTIC TESTS RUN FOR THIS SLOT.\n");
		PRINTF(check_edt);
		break;

	case 2:
		PRINTF("CANNOT FIND FILE: %s\n", Dirname);
		PRINTF(req_abort);
		break;

	case 3:
		PRINTF("CANNOT LOAD FILE: %s\n", Dirname);
		PRINTF(req_abort);
		break;

	case 4:
		PRINTF("UNEXPECTED DIAGNOSTIC EXCEPTION\n");
		PRINTF(req_abort);
		break;

	/* number reserved for interrupt - msg printed in handler
	case 5:
		break;
	 */

	case 6:
		PRINTF("NON-EXISTENT UNIT: %s\n\nTHE EQUIPPED UNIT TYPES ARE:\n", option);

		for (i = 0; i < NUM_EDT; i++)
			if (EDTP(i)->opt_num == 0)
				PRINTF("        %s\n",EDTP(i)->dev_name);
		PRINTF(req_retry);
		break;

	case 7:
		PRINTF("INVALID UNIT NUMBER\nFOR %s, THE EQUIPPED UNITS ARE:\n", option);

		for (i = 0; i < NUM_EDT; i++)
			if ( !STRCMP(option,EDTP(i)->dev_name) )
				PRINTF("        %s  %d\n",EDTP(i)->dev_name,EDTP(i)->opt_num);
		PRINTF(req_retry);
		break;

	case 8:
		PRINTF("%s\nUNRECOGNIZABLE DIAGNOSTIC REQUEST\nCHECK REQUEST SYNTAX AND RE-ENTER.\n\n", start_ptr);
		break;

	case 9:
		PRINTF("INVALID REPEAT VALUE\nRE-ENTER REQUEST USING VALUE BETWEEN 1 AND %d.\n\n", MAXRPT);
		break;

	case 10:
		PRINTF("INVALID PHASE(S) REQUESTED.\nCHECK REQUESTED PHASE TABLE AND RETRY.\n\n");
		break;

	case 11:
		PRINTF("REDUNDANT DIAGNOSTIC REQUEST OPTION\n");
		PRINTF(req_reenter);
		break;

	case 12:
		PRINTF("SOAK AND UCL ARE INCOMPATIBLE DIAGNOSTIC OPTIONS.\nRE-ENTER REQUEST, OMITTING ONE.\n\n");
		break;

	case 13:
		PRINTF("UNIT OR UNIT TYPE NEEDED FOR PHASE OPTION REQUEST.\n");
		PRINTF(req_reenter);
		break;

	case 14:
		PRINTF("USE UNIT TYPE ONLY FOR PHASE DISPLAY REQUEST.\n");
		PRINTF(req_reenter);
		break;

	default:
		PRINTF("UNKNOWN ERROR CODE.\n");
	}
}
