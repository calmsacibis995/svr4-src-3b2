/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/ALNodeType.h	1.1"

struct Alias			/* Structure of a alias node.	*/
	{AN_Id name;		/* Name being aliased.	*/
	 AN_GnaqType naqtype;	/* NAQ type saved for restore.	*/
	 struct Alias *next;	/* Next one in the chain. */
	};

typedef struct Alias *ALN_Id;
