/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/comutil.c	1.6"

#include	<stdio.h>
#include	"defs.h"
#include	"debug.h"
#include	"LoopTypes.h"
#include	"OpTabTypes.h"
#include	"OperndType.h"
#include	"RoundModes.h"
#include	"ANodeTypes.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"optab.h"

static unsigned char BitCount[256] =	/* Number of 1-bits in a byte.	*/
	{0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
	 1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	 1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	 2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	 1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	 2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	 2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	 3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	 1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	 2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	 2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	 3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	 2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	 3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	 3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
	 4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8};

	unsigned int
CountOneBits(byte)		/* Count 1-bits in a byte.	*/
unsigned char byte;		/* Byte whose bit-count is wanted.	*/

{extern unsigned char BitCount[];	/* Bit-count table.	*/
 unsigned int result;

 result = (unsigned int) BitCount[byte];
 return(result);
}


	AN_Id
GetP(tn_id)			/* Return AN_Id of jump destination.	*/
TN_Id tn_id;			/* TN_Id of instruction whose dest. is wanted.*/

{register int d;
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 register unsigned int operand;	/* Operand counter.	*/
 extern struct opent optab[];		/* The operation code table.	*/

 d = optab[GetTxOpCodeX(tn_id)].odstops;
 for(operand = 0; operand < Max_Ops; operand++)
	{if(d & 1)
		return(GetTxOperandAd(tn_id,operand));
	 d >>= 1;
	}
 return(NULL);
}


	boolean
IsHLP(tn_id)			/* TRUE if a fixed label present.	*/
TN_Id tn_id;

{

 for( ; IsTxLabel(tn_id); tn_id = GetTxNextNode(tn_id))
	if(IsTxHL(tn_id))
		return(TRUE);
 return(FALSE);
}
	void
PutP(tn_id,an_id)		/* Put destination in jump node.	*/
TN_Id tn_id;			/* TN_Id of jump node.	*/
AN_Id an_id;			/* AN_Id of jump destination.	*/

{extern char *GetExtTypeId();	/* Gets external form of operand type.	*/
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 register int d;
 unsigned short int opcode;
 register unsigned int operand;	/* Operand counter.	*/
 extern struct opent optab[];	/* The operation code table.	*/
 extern unsigned int praddr();	/* Prints an address node.	*/

 d = optab[opcode = GetTxOpCodeX(tn_id)].odstops;
 for(operand = 0; operand < Max_Ops; operand++)
	{if(d & 1)
		{PutTxOperandAd(tn_id,operand,an_id);
		 PutTxOperandType(tn_id,operand,
			(OperandType) optab[opcode].otype[operand]);
		 if(DBdebug(3,XPCI))
			{printf("PutP: %s %s ",
				optab[opcode].oname,
				GetExtTypeId((OperandType) optab[opcode].otype[operand]));
			 (void)praddr(an_id,
				(OperandType) optab[opcode].otype[operand],
				stdout);
			 putchar(NEWLINE);
			}
		}
	 d >>= 1;
	}
 return;
}
	void
newlab(label,prefix,labelsize)
char *label;			/* Where to put label.	*/
char *prefix;			/* A prefix string for the label. */
unsigned int labelsize;		/* Maximum size for label.	*/

{extern void fatal();		/* Handles fatal errors; in common.	*/
 static unsigned int number;	/* Number part of new label.	*/
 /*extern int sprintf();	** Prints to strings; in C(3) library.	*/

 if(sprintf(label,"%s%u",prefix,number++) >= labelsize)	/* Generate new label.*/
	fatal("newlab: new label too large: %s.\n",label);
 return;
}
