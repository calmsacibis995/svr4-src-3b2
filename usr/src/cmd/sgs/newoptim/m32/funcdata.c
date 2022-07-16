/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/funcdata.c	1.3"

/************************************************************************/
/*				funcdata.c				*/
/*									*/
/*		This file contains routines to compute data on		*/
/*	functions.  							*/
/*									*/
/************************************************************************/

#include	<stdio.h>
#include	"defs.h"
#include	"OpTabTypes.h"
#include	"OpTabDefs.h"
#include	"RegId.h"
#include	"RegIdDefs.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"RoundModes.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"ALNodeType.h"
#include	"FNodeTypes.h"
#include	"FNodeDefs.h"
#include	"optab.h"
#include	"optim.h"
#include	"optutil.h"

	void
funcdata()				/* Compute data on whole functions. */

{extern FN_Id FuncId;			/* Id of current function .*/
 AN_Id an_id;				/* Id of name of called function. */
 FN_Id fn_id;				/* Id of called function. */
 register TN_Id tn_id;			/* Id for scanning text nodes. */
 unsigned short int OpCode;		/* Operation code index for instr. */
 unsigned long int instructions;	/* Instruction counter. */
 AN_Id sjan_id;				/* Id of setjmp address node. */

 PutFnDefined(FuncId,TRUE);		/* Mark function as defined. */

 sjan_id = GetAdAbsolute(Tbyte,"setjmp");
 instructions = 0;
					/* Scan nodes of function. */
 for(ALLNSKIP(tn_id))
	{OpCode = GetTxOpCodeX(tn_id);

					/* Check for black boxes. */
	 if(IsTxBlackBox(tn_id)) 
		PutFnBlackBox(FuncId,TRUE);

	 if(IsOpAux(OpCode) || IsOpPseudo(OpCode))
		continue;

	 				/* Count instructions in this func. */
	 instructions++;
					/* Count calls to other functions, */
					/* and look for calls to setjmp. */
 	 if(OpCode == CALL || OpCode == ICALL)
		{an_id = GetTxOperandAd(tn_id,1);
		 if(IsAdAbsolute(an_id))
		 	{fn_id = GetFnId(an_id);
		 	 PutFnCalls(fn_id,GetFnCalls(fn_id) + 1);
		 	 if(an_id == sjan_id)
				PutFnSetjmp(FuncId,TRUE);
			}
		}

	}
 PutFnInstructions(FuncId,instructions);
}
