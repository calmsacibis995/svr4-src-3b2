/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoconfig:cunix/int_error.c	1.5"

#include <sys/types.h>
#include <stdio.h>
#include <sys/localtypes.h>
#include <sys/int_error.h>
#include <sys/error.h>


/*
 * Error(msgno, args ...)
 *
 * Print an error message, and determine the recovery action (if any) to
 * be taken.  The actions are itemized in errortab[] located in errortable.c
 */

extern struct errortab errortab[];

static int error_action();

/*VARARGS1*/
 int
error(msgnum, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
	register int msgnum;
	int arg1;
	int arg2;
	int arg3;
	int arg4;
	int arg5;
	int arg6;
	int arg7;
	int arg8;
	int arg9;
	int arg10;
	{

	register struct errortab *ep;
	register int msg;
	register int code;
	register char *text;

	for (ep=errortab; msg = ep->msgnum; ++ep)
		{
		if (msg == msgnum)
			{
			/*
			 * determine recovery action
			 */
			code = ep->action;

			if ((text=ep->text) == NULL)
				text = "<NOTEXT>";

			if (code & _PANIC_)
				/*
				 * no deposit, no return
				 */
				panic(text);

			if (! (code & _SILENT_))
				/*
				 * either printf() or perror() will write the message
				 */
					{
					if (code & _RETURN_)
						fprintf(stderr,"warning: ");
					else
						fprintf(stderr,"error: ");

					fprintf(stderr,text, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
					fprintf(stderr,"\n");
					}

			/*
			 * take the recovery action
			 */
			switch (code & _CODE_)
				{
			/*
			 * the caller will do the recovery
			 */
			case _RETURN_:
			case _ERROR_:
			/*
			 * return FALSE
			 */
			case _RETURNFALSE_:
				return(FALSE);
			/*
			 * return TRUE
			 */
			case _RETURNTRUE_:
				return(TRUE);
			/*
			 * better fix errortab[] ...
			 */
			default:
				panic("Illegal error action\n");
				}
			}
		}

	panic("Unknown error number");
	/*NOTREACHED*/
	}


/*
 * panic, print message and exit 
 */
panic(text)
char *text;
{
	fprintf(stderr,"error: %s\n",text);
	exit(1);
}

