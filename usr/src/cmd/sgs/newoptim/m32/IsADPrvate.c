/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/IsADPrvate.c	1.3"

#include	"defs.h"
#include	"RegId.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"


	boolean
IsADPrivate(an_id)		/* TRUE if an_id is private. */
AN_Id an_id;			/* AN_Id of address node to be tested. */

{extern void fatal();		/* Handles fatal errors; in common.	*/
 AN_Mode mode;			/* Mode of address node. */
 RegId reg_idA;			/* Register identifier of address node. */
 RegId reg_idB;			/* Register identifier of address node. */

 reg_idA = REG_NONE;
 reg_idB = REG_NONE;
 switch((mode = GetAdMode(an_id)))		/* Some modes are private. */
	{case Absolute:
	 case AbsDef:
	 case CPUReg:
	 case StatCont:
	 case Immediate:
	 case MAUReg:
	 case Raw:
		endcase;
	 case IndexRegDisp:
	 case IndexRegScaling:
		reg_idB = GetAdRegB(an_id);	/* Examine register B. */
		if(reg_idB == CAP || reg_idB == CFP)
			return(TRUE);
		/* FALLTHRU */
	 case Disp:
	 case DispDef:
	 case PostDecr:
	 case PostIncr:
	 case PreDecr:
	 case PreIncr:
		reg_idA = GetAdRegA(an_id);	/* Examine register A. */
		if(reg_idA == CFP || reg_idA == CAP)
			return(TRUE);
		endcase;		/* Not offset from "good" register. */
	 default:
		fatal("IsADPrivate: unknown address mode (0x%8.8x).\n",mode);
		endcase;
	}
 return(FALSE);
}
