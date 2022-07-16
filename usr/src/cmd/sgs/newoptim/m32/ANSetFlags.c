/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/ANSetFlags.c	1.5"

/************************************************************************/
/*				AN_SetFlags.c				*/
/*									*/
/*		This file contains a machine-dependent function that	*/
/*	initializes the status of the gnaq, FP, Safe, and TIQ fields	*/
/*	of an address node.						*/
/*									*/
/************************************************************************/

#include	"defs.h"
#include	"RegId.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"

	void
AN_SetFlags(an_id)		/* Set some address flags. */
register AN_Id an_id;		/* AN_Id of node to set. */

{extern enum CC_Mode ccmode;	/* cc -X? */
 extern void fatal();		/* Handles fatal errors; in debug.c. */
 AN_Mode mode;			/* Mode of the address mode. */
 register RegId regidA;		/* Register identifier of some address nodes. */
 register RegId regidB;		/* Register identifier of some address nodes. */

 PutAdLabel(an_id,FALSE);			/* Not a label unless we */
						/* find out otherwise.	*/
 PutAdSafe(an_id,TRUE);				/* Is safe from volatility,
						 * i.e. NOT volatile, unless
						 * otherwise determined.
						 */
 PutAdRO(an_id,FALSE);				/* Not a readonly. */
 PutAdCandidate(an_id,FALSE);			/* Not for candidate use only. */

 switch((mode = GetAdMode(an_id)))		/* Flags depend on mode. */
	{case Immediate:			/* Immediate. */
		PutAdGnaqType(an_id,NAQ);	/* It is a NAQ. */
		PutAdTIQ(an_id,TRUE);		/* It is a TIQ. */
		PutAdFP(an_id,MAYBE);		/* Don't know if FP. */
		endcase;
	 case Absolute:				/* Absolute. */
		PutAdGnaqType(an_id,Other);	/* It is a Other. */
		PutAdTIQ(an_id,FALSE);		/* It is not a TIQ. */
		if(ccmode == Transition)	/* In transition mode,  */
			PutAdSafe(an_id,FALSE);	/* default is not Safe. */
		PutAdFP(an_id,MAYBE);		/* Don't know if FP. */
		endcase;
	 case AbsDef:				/* Absolute deferred.. */
		PutAdGnaqType(an_id,Other);	/* It is a Other. */
		PutAdTIQ(an_id,FALSE);		/* It is not a TIQ. */
		if(ccmode == Transition)	/* In transition mode,  */
			PutAdSafe(an_id,FALSE);	/* default is not Safe. */
		PutAdFP(an_id,MAYBE);		/* Don't know if FP. */
		endcase;
	 case CPUReg:				/* CPU Register. */
		PutAdGnaqType(an_id,NAQ);	/* It is a NAQ. */
		PutAdTIQ(an_id,TRUE);		/* It is a TIQ. */
		PutAdFP(an_id,MAYBE);		/* Don't know if FP. */
		endcase;
	 case Disp:				/* Displacement. */
		PutAdTIQ(an_id,TRUE);		/* It is a TIQ. */
		regidA = GetAdRegA(an_id);	/* Is it Safe? */
		if(		(regidA != CAP) &&
				(regidA != CFP) &&
				(regidA != CSP) &&
				(ccmode == Transition))
			PutAdSafe(an_id,FALSE);	/* It is not Safe. */
		PutAdFP(an_id,MAYBE);		/* Don't know if FP.	*/
		if((regidA == CAP) || (regidA == CFP))	/* SVs are those */
			PutAdGnaqType(an_id,SV);	/* off AP or FP. */
		else
			PutAdGnaqType(an_id,Other);	/* Not SV. */
		endcase;
	 case DispDef:				/* Displacement deferred. */
		PutAdGnaqType(an_id,Other);	/* It is a Other. */
		PutAdTIQ(an_id,FALSE);		/* It is not a TIQ. */
		if(ccmode == Transition)	/* In transition mode,  */
			PutAdSafe(an_id,FALSE);	/* default is not Safe. */
		PutAdFP(an_id,MAYBE);		/* Don't know if FP. */
		endcase;
	 case PreDecr:				/* Auto pre-decrement. */
		PutAdGnaqType(an_id,Other);	/* It is a Other. */
		PutAdTIQ(an_id,TRUE);		/* It is a TIQ. */
		regidA = GetAdRegA(an_id);	/* Is it safe? */
		if(		(regidA != CAP) &&
				(regidA != CFP) &&
				(regidA != CSP) &&
				(ccmode == Transition))
			PutAdSafe(an_id,FALSE);	/* It is not safe. */
		PutAdFP(an_id,MAYBE);		/* Don't know if FP. */
		endcase;
	 case PreIncr:				/* Auto pre-increment. */
		PutAdGnaqType(an_id,Other);	/* It is a Other. */
		PutAdTIQ(an_id,TRUE);		/* It is a TIQ. */
		regidA = GetAdRegA(an_id);	/* Is it safe? */
		if(		(regidA != CAP) &&
				(regidA != CFP) &&
				(regidA != CSP) &&
				(ccmode == Transition))
			PutAdSafe(an_id,FALSE);	/* It is not Safe. */
		PutAdFP(an_id,MAYBE);		/* Don't know if FP. */
		endcase;
	 case PostDecr:				/* Auto post-decrement. */
		PutAdGnaqType(an_id,Other);	/* It is a Other. */
		PutAdTIQ(an_id,TRUE);		/* It is a TIQ. */
		regidA = GetAdRegA(an_id);	/* Is it safe? */
		if(		(regidA != CAP) &&
				(regidA != CFP) &&
				(regidA != CSP) &&
				(ccmode == Transition))
			PutAdSafe(an_id,FALSE);	/* It is not Safe. */
		PutAdFP(an_id,MAYBE);		/* Don't know if FP. */
		endcase;
	 case PostIncr:				/* Auto post-increment. */
		PutAdGnaqType(an_id,Other);	/* It is a Other. */
		PutAdTIQ(an_id,TRUE);		/* It is a TIQ. */
		regidA = GetAdRegA(an_id);	/* Is it safe? */
		if(		(regidA != CAP) &&
				(regidA != CFP) &&
				(regidA != CSP) &&
				(ccmode == Transition))
			PutAdSafe(an_id,FALSE);	/* It is not Safe. */
		PutAdFP(an_id,MAYBE);		/* Don't know if FP. */
		endcase;
	 case IndexRegDisp:		/* Indexed register with displacement.*/
		PutAdGnaqType(an_id,Other);	/* It is a Other. */
		PutAdTIQ(an_id,TRUE);		/* It is a TIQ. */
		regidA = GetAdRegA(an_id);	/* Is it safe? */
		regidB = GetAdRegB(an_id);	/* Is it safe? */
		if(		(regidA != CAP) &&
				(regidA != CFP) &&
				(regidA != CSP) &&
				(regidB != CAP) &&
				(regidB != CFP) &&
				(regidB != CSP) &&
				(ccmode == Transition))
			PutAdSafe(an_id,FALSE);	/* It is not safe. */
		PutAdFP(an_id,MAYBE);		/* Don't know if FP. */
		endcase;
	 case IndexRegScaling:		/* Indexed register with scaling. */
		PutAdGnaqType(an_id,Other);	/* It is a Other. */
		PutAdTIQ(an_id,TRUE);		/* It is a TIQ. */
		regidA = GetAdRegA(an_id);	/* Is it safe? */
		regidB = GetAdRegB(an_id);	/* Is it safe? */
		if(		(regidA != CAP) &&
				(regidA != CFP) &&
				(regidA != CSP) &&
				(regidB != CAP) &&
				(regidB != CFP) &&
				(regidB != CSP) &&
				(ccmode == Transition))
			PutAdSafe(an_id,FALSE);	/* It is not safe. */
		PutAdFP(an_id,MAYBE);		/* Don't know if FP. */
		endcase;
	 case MAUReg:				/* MAU register. */
		PutAdGnaqType(an_id,NAQ);	/* It is a NAQ. */
		PutAdTIQ(an_id,TRUE);		/* It is a TIQ. */
		PutAdFP(an_id,YES);		/* It is FP. */
		endcase;
	 case StatCont:				/* Status or  Control bit. */
		PutAdGnaqType(an_id,NAQ);	/* It is NAQ. */
		PutAdTIQ(an_id,FALSE);		/* It is not a TIQ. */
		PutAdFP(an_id,NO);		/* Surely not FP. */
		endcase;
	 case Raw:				/* Raw string. */
		PutAdGnaqType(an_id,Other);	/* It is a Other. */
		PutAdTIQ(an_id,FALSE);		/* It is not a TIQ. */
		PutAdSafe(an_id,FALSE);		/* It is not Safe. */
		PutAdFP(an_id,MAYBE);		/* Don't know if FP. */
		endcase;
	 default:				/* Unknown. */
		fatal("AN_SetFlags: illegal mode (%d).\n",mode);
		endcase;
	}
 return;
}
