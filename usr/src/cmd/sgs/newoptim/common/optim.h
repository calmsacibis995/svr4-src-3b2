/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/optim.h	1.3"

/*	machine independent include file for code improver */

/* structure of non-branch text reference node */

typedef struct ref {
	AN_Id lab;		/* AN_Id of label referenced. */
	struct ref *nextref;	/* link to next ref */
	} REF;

#define saveop(opn, str, len, op) \
	(void) Saveop((opn),(str),(op))
#define appmisc(str, len)	saveop(0, (str), (len), MISC)
#define appfl(str, len)		saveop(0, (str), (len), FILTER)
#define ALLN(p)	p = GetTxNextNode((TN_Id) NULL); p != NULL; p = GetTxNextNode(p)
#define PRINTF			(void) printf
#define FPRINTF			(void) fprintf
#define SPRINTF			(void) sprintf
#define PUTCHAR(c)		(void) putchar(c)
#define GETSTR(type)		(type *) getspace(sizeof(type))
