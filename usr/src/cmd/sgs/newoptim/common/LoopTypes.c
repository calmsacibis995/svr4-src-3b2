/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/LoopTypes.c	1.3"

#include	<string.h>
#include	"defs.h"
#include	"LoopTypes.h"

static char *ExtLoopType[] = {"BEG","HDR","INCR","COND","END",""};
static LoopType IntLoopType[] = {Begin,Header,Increment,Condition,End,Missing};

	char *
GetExtLoopType(internal)	/* Return pointer to external loop type. */
LoopType internal;		/* Internal form of loop type. */

{extern char *ExtLoopType[];	/* External form of loop types. */
 extern LoopType IntLoopType[];	/* Internal form of loop types. */
 extern void fatal();		/* Handles fatal errors; in common. */
 register unsigned int typeno;	/* Sequential search index. */

 for(typeno = 0; IntLoopType[typeno] != Missing; typeno++)
	{if(internal == IntLoopType[typeno])
		return(ExtLoopType[typeno]);
	}

 fatal("GetExtLoopType: invalid loop type (0x%8.8x).\n",internal);
 /*NOTREACHED*/
}


	LoopType
GetIntLoopType(external)	/* Return internal loop type. */
char *external;			/* Pointer to external form; */
				/* need not be NULL-terminated. */

{extern char *ExtLoopType[];	/* External form of loop types. */
 extern LoopType IntLoopType[];	/* Internal form of loop types. */
 extern void fatal();		/* Handles fatal errors; in common. */
 /*extern int strlen();		** String length; in C(3) library. */
 /*extern int strncmp();	** Counted string compare; in C(3) library. */
 register unsigned int typeno;	/* Sequential search index. */

 for(typeno = 0; *ExtLoopType[typeno] != EOS; typeno++)
	{if(!strncmp(external,ExtLoopType[typeno],strlen(ExtLoopType[typeno])))
		return(IntLoopType[typeno]);	/* (We found it.) */
	}

 fatal("GetIntLoopType: invalid loop type: '%s`.\n",external);
 /*NOTREACHED*/
}
