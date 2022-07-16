/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/rer.c	1.4"

#include	<stdio.h>
#include	"defs.h"
#include	"ANodeTypes.h"
#include	"OpTabTypes.h"
#include	"RegId.h"
#include	"ANodeDefs.h"
#include	"olddefs.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"RoundModes.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"optab.h"
#include	"optim.h"

	/* Private function declarations. */
STATIC boolean chklabel();

	void
rer()			/* Check for unused returns.	*/
			/* NOTE: only removes IS25 returns. */

{
 register TN_Id pn, p, r, rr;
 TN_Id pret;
 STATIC boolean chklabel();

 pret = NULL;
 for(ALLN(pn))					/* Examine all text-nodes. */
	{if(GetTxOpCodeX(pn) != IRET)		/* If not IS25 return,	*/
		continue;			/* skip it.	*/
	 for(p = GetTxPrevNode(pn); p != (TN_Id) NULL; p = GetTxPrevNode(p))
		{if(IsTxUncBr(p))
			{for( r = GetTxNextNode(p); r != GetTxNextNode(pn); )
				{rr = r;
				 r = GetTxNextNode(r);
				 if(IsTxLabel(rr))	/* Save the labels. */
					{if(IsTxHL(rr))		/* Hard? */
						(void) MoveTxNodeBefore(rr,p);
					 else			/* Soft. */
						(void) MoveTxNodeBefore(rr,
							pret);
					}
				 else		/* Not a label.	*/
					DelTxNode(rr);
				}
			 break;
			}
		 if( !IsTxLabel( p ) ||
				(IsTxHL(p) && chklabel(GetTxOperandAd(p,0))) ||
				(!IsTxHL(p) && (pret == NULL)))
			{pret = pn;
			 break;
			}
		} /*  END OF for(p = GetTxPrevNode(pn); p != NULL; ... ) */
	} /* END OF for(ALLN(pn)) */
 return;
}
	STATIC boolean
chklabel(an_id)			/* Check whether label is used in function. */
AN_Id an_id;			/* AN_Id of label to be checked.	*/

{extern unsigned int Max_Ops;	/* Maximum number of operands.	*/
 register unsigned int operand;	/* Operand counter.	*/
 register TN_Id pn;

 for(ALLN(pn))				/* Examine all text-nodes in function.*/
	{if(IsTxAux(pn))		/* Skip the auxiliary nodes.	*/
		continue;
	 for(operand = 0; operand < Max_Ops; operand++)
		{if(GetTxOperandType(pn,operand) == Tnone)
			break;		/* (No more operands in instruction.) */
		 if(IsAdUses(GetTxOperandAd(pn,operand),an_id))
			return(TRUE);
		} /* END OF for(operand = 0; operand < Max_Ops; operand++) */
	} /* END OF for(ALLN(pn)) */
 return(FALSE);
}
