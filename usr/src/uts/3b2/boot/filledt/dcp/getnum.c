/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)boot:boot/filledt/dcp/getnum.c	1.2"

#include <sys/firmware.h>
#include <sys/edt.h>
#include <sys/diagnostic.h>


/*    This function scans for the board and option number in the EDT.
      If the requested board and option are valid, filename is set to the
      diagnostic file name in the EDT.    */


extern struct edt edt[];      /* equipped device table */
extern void dgnerror();

extern char option[];		/* option name from input parser   */
extern char *strcpy();
extern unsigned char optno;	/* option number from input parser */
extern unsigned char opt_type;	/* flag to diagnose option type */
extern char filename[];  	/* diagnostic phase file name  */

getnum(board)
	unsigned long *board;
	{
unsigned char dev_match;		/* device type match flag */

	dev_match = OFF;		/* set to no match */

	/* copy EDT phase file name for or DGN all */

	if (!STRCMP(option,"\0"))
		(void) strcpy(filename,EDTP(*board)->diag_file);

	/* search for match of device ( & number) */

	else

		for((*board) = 0; (*board) < NUM_EDT; (*board)++) {

			/* do device and request names match? */
			if(!STRCMP(option,(EDTP(*board)->dev_name)))
				{
				dev_match=ON;
				/* do device and request numbers match? */
				if ((EDTP(*board)->opt_num) == optno)
						{
						(void) strcpy(filename,(EDTP(*board)->diag_file));
						break;
						}
				}
		}

	/* return FAIL if no device match found in EDT */

	if(*board == NUM_EDT && STRCMP(option,"\0"))
		{
	/* print message for device match failures */

		if (dev_match == OFF) {
			/* non-existent unit; print error message */
			dgnerror(6);
			}

	/* print message for single device number failure */

		else if (!opt_type) {
			/* invalid unit number; print error message */
			dgnerror(7);
			}
		return(FAIL);
		}

	else
			return(PASS);
	}
