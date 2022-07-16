/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:sys/int_error.h	1.3"

/*
 * Error table entry
 */
struct	errortab
	{
        short	msgnum;		/* the message number from error.h */
	short	action;		/* the action to take */
	char	*text;		/* the actual message text */
	};

#define _PANIC_		0x4000		/* Flags:  panic() */
#define _SILENT_	0x2000		/*	   do not print message text */

#define _CODE_		0x0FFF		/* Action codes		*/
#define _RETURN_	1		/*	return, warning	*/
#define _RETURNTRUE_	2		/*	return(TRUE)	*/
#define _RETURNFALSE_	3		/*	return(FALSE)	*/
#define _ERROR_		4		/* print error:		*/
#define _DYNAMIC_	4095		/*	error_action() will determine ultimate action	*/


