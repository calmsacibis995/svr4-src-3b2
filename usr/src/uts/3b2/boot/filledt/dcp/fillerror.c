/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/fillerror.c	1.1"


/* error message routine for filledt */

#include <sys/firmware.h>
#include <sys/edt.h>
#include <sys/diagnostic.h>

extern unsigned char Dirname[];		/* full pathname of a file */


char check_edt[] = "CHECK EDT.\n\n";
char tbl_cont[] = "EQUIPPED DEVICE TABLE COMPLETION WILL CONTINUE.\n";
char f_error[] = "\nEDT COMPLETION ERROR 1-%02d:\n";

fillerror(fillerrno)
int fillerrno;
{
int i;

PRINTF(f_error, fillerrno);

switch (fillerrno)
	{
	case 0:
		PRINTF("FILE SYSTEM IS INACCESSIBLE.\n");
		PRINTF("CONTROL WILL RETURN TO MAINTENANCE CONTROL PROGRAM.\n\n");
		break;

	case 1:
		PRINTF("ERROR OCURRED DURING SYSTEM CONFIGURATION.  ");
		PRINTF("CONSOLE LOCATION PROCEEDING.\n");
		PRINTF(check_edt);
		break;

	case 2:
		PRINTF("CANNOT FIND FILE: %s\n",Dirname);
		break;

	case 3:
		PRINTF("CANNOT LOAD FILE: %s\n",Dirname);
		break;

	case 4:
		PRINTF("UNEXPECTED EXCEPTION\n");
		break;

	/* interrupt message printed in interrupt handler to save stack space 
	case 5:
		PRINTF("UNEXPECTED INTERRUPT\n");
		break;
	 */

	case 6:
		PRINTF("SYSGEN FAILED FOR %s IN SLOT %d\n",
			EDTP(OPTION)->dev_name,EDTP(OPTION)->opt_slot);
		PRINTF(tbl_cont);
		PRINTF(check_edt);
		break;

	case 7:
		PRINTF("DSD FAILED FOR %s IN SLOT %d\n",
			EDTP(OPTION)->dev_name,EDTP(OPTION)->opt_slot);
		PRINTF(tbl_cont);
		PRINTF(check_edt);
		break;

	case 8:
		i = IO;
		IO = ON;
		PRINTF("UNKNOWN ID CODE 0x%x FOR DEVICE IN SLOT %d\n",
			EDTP(OPTION)->opt_code,EDTP(OPTION)->opt_slot);
		PRINTF(tbl_cont);
		PRINTF(check_edt);
		IO = i;
		break;

	case 9:
		i = IO;
		IO = ON;
		PRINTF("UNKNOWN SUBDEVICE ID CODE FOR %s IN SLOT %d\n",
			EDTP(OPTION)->dev_name,EDTP(OPTION)->opt_slot);
		PRINTF(tbl_cont);
		PRINTF(check_edt);
		IO = i;
		break;

	case 10:
		i = IO;
		IO = ON;
		PRINTF("EDT EXCEEDS ALLOCATED SPACE ");
		PRINTF("AND CANNOT BE COMPLETED.\n\n");
		PRINTF("REDUCE SYSTEM CONFIGURATION\n\n");
		IO = i;
		break;

	case 11:
		i = IO;
		IO = ON;
		PRINTF("SOFTWARE APPLICATION FILE ERROR - ENTRY FOR SLOT %d\nDOES NOT MATCH EDT DEVICE NAME, %s.\n",
			EDTP(OPTION)->opt_slot,EDTP(OPTION)->dev_name);
		PRINTF(tbl_cont);
		PRINTF(check_edt);
		IO = i;
		break;

	case 12:
		i = IO;
		IO = ON;
		PRINTF("SOFTWARE APPLICATION FILE ERROR - EDT HAS NO DEVICE IN SLOT %d.\n",OPTION);
		PRINTF(tbl_cont);
		PRINTF(check_edt);
		IO = i;
		break;

	case 13:
		i = IO;
		IO = ON;
		PRINTF("SOFTWARE APPLICATION FILE ERROR - INCOMPLETE ENTRY FOR SLOT %d.\n",OPTION);
		PRINTF(tbl_cont);
		PRINTF(check_edt);
		IO = i;
		break;

	default:
		PRINTF("UNKNOWN ERROR CODE.\n");
	}
}
