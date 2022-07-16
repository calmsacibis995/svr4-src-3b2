/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/value.c	1.7"

#define	DESTINATION	3
#define	GM_SOURCE	2
#define	MPA_SOURCE	2

#include	<stdio.h>
#include	"defs.h"
#include	"debug.h"
#include	"LoopTypes.h"
#include	"OpTabTypes.h"
#include	"OpTabDefs.h"
#include	"OperndType.h"
#include	"RegId.h"
#include	"RoundModes.h"
#include	"ANodeTypes.h"
#include	"optab.h"
#include	"optim.h"
#include	"ANodeDefs.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"optutil.h"
#include	"TNodeDefs.h"
#include	"ALNodeType.h"
#include	"FNodeTypes.h"
#include	"FNodeDefs.h"

struct vtent			/* The value tracing table.	*/
	{AN_Id vaddr;		/* AN_Id of address node.	*/
	 AN_Id vvalue;		/* "value" of address node.	*/
	 OperandType vtype;	/* "type" of value.	*/
	 unsigned MOVA:1;	/* 1 if G_MOVA; else 0.	*/
	};
static struct vtent vttab[VT_TAB_SIZE];

static unsigned int vtablim;	/* Value trace table limit, computed */
				/*  as a function of GnaqListSize.   */

	/* Private function declarations. */
STATIC void allforget();	/* Forget all value tracing knowledge.	*/
STATIC void do_set();		/* Adjust those operands that are set.	*/
STATIC void do_used();		/* Adjust those operands that are used. */
STATIC void fixup();		/* Replaces name with a better one.	*/
STATIC void fixupA();		/* Replaces name with a better one.	*/
STATIC void forget();		/* Have L-D table forget obsolete knowledge. */


	void
value()				/* This function attempts value tracing	*/
				/* code improvements. It returns if	*/
				/* all went well,and calls fatal() if trouble.*/

{
 extern FN_Id FuncId;		/* Id of current function. */
 extern unsigned int GnaqListSize;	/* Current size of GNAQ list.	*/
 STATIC void allforget();	/* Forget all value tracing knowledge.	*/
 STATIC void do_set();		/* Adjust those operands that are set.	*/
 STATIC void do_used();		/* Adjust those operands that are used. */
 extern void fprinst();		/* Prints an instruction.	*/
 extern void funcprint();	/* Prints a function.	*/
 register TN_Id ip;		/* TN_Id of current instruction.	*/
 extern struct opent optab[];	/* The operation code table.	*/

 if(Xskip(XVTRACE))                             /* Should this optimization */
        return;                                 /* be done? NO. */

 if(IsFnBlackBox(FuncId))			/* If black box in function, */
	return;					/* give a normal return. */

 if(DBdebug(1,XVTRACE))				/* Show function before	*/
	funcprint(stdout,"before value tracing",0);	/* we do anything. */

 vtablim = Min(GnaqListSize, VT_TAB_SIZE);	/* Initialize table limit. */
 allforget();					/* Initialize knowledge. */
						/* SCAN THE INSTRUCTIONS. */
 for(ALLNSKIP(ip))				/* (Skipping profiler code.) */
	{if(DBdebug(3,XVTRACE))
		fprinst(stdout,-2,ip);
	 if(IsTxLabel(ip)			/* If we hit a label or */
#ifndef EXTENDBB
			|| IsTxBr(ip)		/* a branch or 	*/
#endif
			|| IsTxAnyCall(ip))	/* any calls, we must forget. */
		{if(DBdebug(3,XVTRACE))
			(void) fprintf(stdout,"%cwill not be traced\n",
				ComChar);
		 allforget();			/* We hit one.	*/
		 continue;			/* Not part of basic block. */
		}
	 else if(IsTxAux(ip))			/* Ignore auxiliary nodes. */
		{if(DBdebug(3,XVTRACE))
			(void) fprintf(stdout,"%cwill not be traced\n",ComChar);
		 continue;
		}
	 if(!IsTxProtected(ip))			/* If unprotected,        */
	 	do_used(ip);			/*  adjust used operands. */
	 do_set(ip);				/* Adjust set operands. */
	} /* END OF for(ALLNSKIP(ip) */

 if(DBdebug(2,XVTRACE))				/* Show function after	*/
	funcprint(stdout,"after value tracing",0);	/* we do everything. */

 return;					/* Normal return. */
}
	STATIC void
do_set(ip)			/* This function examines the instruction */
				/* specified by its argument and sets the */
				/* vt table entry to contain the new value. */
				/* If the instruction is a G_MOV and the */
				/* new value is the same as the old, the */
				/* source operand is changed to be the same */
				/* as the destination operand so that this */
				/* instruction may be removed by the live- */
				/* dead optimizer if the condition codes are */
				/* dead. */
register TN_Id ip;		/* TN_Id of instruction to be improved. */

{
 AN_Id destname;		/* AN_Id of destination.	*/
 OperandType desttype;		/* Type of destination.		*/
 STATIC void forget();		/* Have L-D table forget obsolete knowledge. */
 unsigned int op_code;		/* Operation code index of an instruction. */
 extern unsigned int praddr();	/* Prints an address.	*/
 extern void prafter();		/* Print nodes after an optimization.	*/
 extern void prbefore();	/* Print nodes before an optimization.	*/
 unsigned int vtx;		/* Value Tracing table index.	*/
 OperandType ntype;		/* "Type" of an operand.	*/
 AN_Id nvalue;			/* "Value" of an operand.	*/
 OperandType otype;		/* "Type" of an operand.	*/
 AN_Id ovalue;			/* "Value" of an operand.	*/

 destname = GetTxOperandAd(ip,DESTINATION);	/* Get name of operand set. */
 desttype = GetTxOperandType(ip,DESTINATION);	/* Get type of operand set. */
 if((!IsAdAddrIndex(destname)) ||		/* If not in VT table, */
		((vtx = GetAdAddrIndex(destname)) >= vtablim))
	{if(DBdebug(3,XV_SET))
		{(void) fprintf(stdout,"%cvalue tracing: ",ComChar);
		 if(praddr(destname,desttype,stdout) < 1)
		 (void) fprintf(stdout, " will not be traced\n");
		}
	 return;
	}

 ovalue = vttab[vtx].vvalue;			/* Get value of operand set */
 nvalue = GetTxOperandAd(ip,GM_SOURCE);
 otype = vttab[vtx].vtype;			/* and its datatype.	*/
 op_code = GetTxOpCodeX(ip);			/* Local (faster) op-code. */
 if(((op_code == G_MOV)				/* If this is a move,	*/
		|| (op_code == G_MMOV))		/* either fixed or floating, */
		&& (GetTxOperandType(ip,GM_SOURCE) ==
			desttype)		/* and no conversions,	*/
		&& (IsAdAddrIndex(nvalue))
		&& (GetAdAddrIndex(nvalue) < vtablim))
	{ntype = GetTxOperandType(ip,GM_SOURCE);
	 if((nvalue == ovalue)			/* same as the old,	*/
			&& (ntype == otype)	/* and datatypes agree,	*/
			&& (ovalue != NULL)	/* and we know the old,	*/
			&& IsAdSafe(nvalue))	/* and source is safe,	*/
		{if((!IsTxProtected(ip))	/* and if not protected, */
				&& legalgen(GetTxOpCodeX(ip),
					GetTxOperandType(ip,0),
					GetAdMode(GetTxOperandAd(ip,0)),
					GetTxOperandType(ip,1),
					GetAdMode(GetTxOperandAd(ip,1)),
					GetTxOperandType(ip,2),
					GetAdMode(destname),
					GetTxOperandType(ip,3),
					GetAdMode(GetTxOperandAd(ip,3))))
			{if(DBdebug(0,XV_SET))
				prbefore(GetTxPrevNode(ip),
					GetTxNextNode(ip),0,"value tracing");
			 PutTxOperandAd(ip,GM_SOURCE,destname);	/* change to move to itself. */
			 if(DBdebug(0,XV_SET))
				prafter(GetTxPrevNode(ip),
					GetTxNextNode(ip),0);
			}
		}
	 else
		{forget(destname);	/* Forget obsolete knowledge. */
		 vttab[vtx].vvalue = nvalue;
		 vttab[vtx].vtype = ntype;
		 vttab[vtx].MOVA = 0;
		 if(DBdebug(3,XV_SET))
			{fprintf(stdout,"%cvalue tracing: vtx=%u ",ComChar,vtx);
			 (void)praddr(destname,desttype,stdout);
			 fprintf(stdout," will contain ");
			 (void)praddr(nvalue,ntype,stdout);
			 putchar(NEWLINE);
			} /* END OF if(DBdebug(3,XV_SET)) */
		} /* END of else [if((nvalue == ovalue) ... */
	} /* END OF if(((GetTxOpCodeX(ip) == G_MOV) ... */
 else if(op_code == G_MOVA)			/* MOVAW's differ a little. */
	{forget(destname);			/* Forget obsolete knowledge. */
	 vttab[vtx].vvalue = nvalue;
	 vttab[vtx].vtype = Tword;
	 vttab[vtx].MOVA = 1;
	 if(DBdebug(3,XV_SET))
		{fprintf(stdout,"%cvalue tracing: vtx = %u ",ComChar,vtx);
		 (void)praddr(destname,desttype,stdout);
		 fprintf(stdout," will contain address of ");
		 (void)praddr(nvalue,Tword,stdout);
		 putchar(NEWLINE);
		} /* END OF if(DBdebug(3,XV_SET)) */
	} /* END OF else if(op_code == G_MOVA) */
 else
	{forget(destname);			/* Forget obsolete info. */
	 vttab[vtx].vvalue = NULL;
	 vttab[vtx].vtype = (OperandType) NULL;
	 if(DBdebug(3,XV_SET))
		{fprintf(stdout,"%cvalue tracing: ",ComChar);
		 (void)praddr(destname,desttype,stdout);
		 fprintf(stdout," will not be traced\n");
		}
	}
 return;					/* Normal return. */
}
	STATIC void
do_used(ip)			/* This function examines those operands that */
				/* are used by the instruction specified by */
				/* its argument and replaces those that are */
				/* NAQs by the "first" one found in the */
				/* value tracing table. This gives preverence */
				/* to registers and tries to make other */
				/* instances dead. */
register TN_Id ip;		/* TN_Id of instruction.	*/

{
 register AN_Id Lvalue;		/* "Value" of an operand.	*/
 extern unsigned int Max_Ops;	/* Maximum number of instruction operands. */
 AN_Id ad[MAX_OPS];
 STATIC void fixup();		/* Replaces name with a better one.	*/
 STATIC void fixupA();		/* Replaces name with a better one.	*/
 unsigned int op_code;		/* Operation code index of an instruction. */
 extern unsigned int praddr();	/* Prints an address.	*/
 extern void prafter();		/* Print nodes after an optimization.	*/
 extern void prbefore();	/* Print nodes before an optimization.	*/
 register unsigned int vt;	/* Value Tracing table index counter.	*/
 unsigned int vtx;		/* Value Tracing table index.	*/
 AN_Id name;			/* Name of an operand.	*/
 unsigned int operand;		/* Operand counter for instructions.	*/
 OperandType type;		/* "Type" of an operand.	*/

 op_code = GetTxOpCodeX(ip);			/* Local (quickie) op-code. */
 if(op_code == G_PUSHA)				/* Leave these alone.	*/
	{if(DBdebug(3,XV_USED))
		{(void) fprintf(stdout,"%cvalue tracing: ",ComChar);
		 (void) fprintf(stdout,"instruction will not be traced\n");
		}
	 return;
	}

 if(op_code == G_MOVA)
	{fixupA(ip);
	 return;
	}

 for(operand = 0; operand < Max_Ops; operand++)	/* Get AN_Id's of operands. */
	ad[operand] = GetTxOperandAd(ip,operand);

						/* Process only those used. */
 for(operand = GetOpFirstOp(op_code); operand < Max_Ops - 1; operand++)
	{name = GetTxOperandAd(ip,operand);	/* Get name of this operand.*/
	 if((!IsAdAddrIndex(name)) ||		/* If not in vt table,	*/
			((vtx = GetAdAddrIndex(name)) >= vtablim))
		{fixup(ip,operand,name);	/* Transform if possible. */
		 continue;
		}
	 Lvalue = vttab[vtx].vvalue;		/* Get operand value */
	 type = vttab[vtx].vtype;		/* and type. */
	 if(DBdebug(3,XV_USED))
		{fprintf(stdout,"%cvalue tracing: do_used: ",ComChar);
		 (void)praddr(name,GetTxOperandType(ip,operand),stdout);
		 fprintf(stdout," contains ");
		 if(Lvalue){
			if(vttab[vtx].MOVA)
				fprintf(stdout," address of ");
			(void)praddr(Lvalue,type,stdout);
			}
		 else
			fprintf(stdout,"UNKNOWN");
		 putchar(NEWLINE);
		}
	 if(Lvalue == NULL)			/* If we don't know value. */
		continue;			/* let it alone. */
#ifdef	PROP_CONST
	 if(IsAdImmediate(Lvalue))		/* If value is constant, */
		{				/* use it instead. */
		 ad[operand] = Lvalue;
		 if(legalgen(GetTxOpCodeX(ip),
				GetTxOperandType(ip,0),
				GetAdMode(ad[0]),
				GetTxOperandType(ip,1),
				GetAdMode(ad[1]),
				GetTxOperandType(ip,2),
				GetAdMode(ad[2]),
				GetTxOperandType(ip,3),
				GetAdMode(ad[3])))
			{if(DBdebug(0,XV_USED))
				prbefore(GetTxPrevNode(ip),
					GetTxNextNode(ip),0,"value tracing");
			 PutTxOperandAd(ip,operand,Lvalue);
			 if(DBdebug(0,XV_USED))
				prafter(GetTxPrevNode(ip),
					GetTxNextNode(ip),0);
			}
		 else
			ad[operand] = GetTxOperandAd(ip,operand);
		 continue;
		}
#endif /*PROP_CONST*/

	 for(vt = 0; vt < vtx; vt++)	/* Find "first" thing with that value. */
		{if(DBdebug(4,XV_USED)){
			fprintf(stdout,
				"%cvalue tracing: do_used: vt=%u",ComChar,vt);
			if(vttab[vt].vvalue != NULL){
				fprintf(stdout,",v=");
				(void)praddr(vttab[vt].vaddr,vttab[vt].vtype,stdout);
				fprintf(stdout,",vv=");
				(void)praddr(vttab[vt].vvalue,vttab[vt].vtype,stdout);
				putchar(NEWLINE);
				}
			else
				fprintf(stdout,",v=?,vv=0\n");
		 	}
		 if(Lvalue == vttab[vt].vaddr)    /* Found address in table before
						   *  any match, so this value must
						   *  be a cheaper NAQ, so use it
						   */
			 ad[operand] = Lvalue;

		 else if((Lvalue == vttab[vt].vvalue)	/* Value match */
				&& (type == vttab[vt].vtype))
			 ad[operand] = vttab[vt].vaddr;

		 else
			continue;

		 if(DBdebug(4,XV_USED))
			fprintf(stdout,"%cvalue tracing: do_used: match, vt=%u\n",
					ComChar,vt);
		 if(legalgen(GetTxOpCodeX(ip),
				GetTxOperandType(ip,0),
				GetAdMode(ad[0]),
				GetTxOperandType(ip,1),
				GetAdMode(ad[1]),
				GetTxOperandType(ip,2),
				GetAdMode(ad[2]),
				GetTxOperandType(ip,3),
				GetAdMode(ad[3])))
			{if(DBdebug(0,XV_USED))
				prbefore(GetTxPrevNode(ip),GetTxNextNode(ip),0,
					       "value tracing");
			 PutTxOperandAd(ip,operand,ad[operand]);
			 if(DBdebug(0,XV_USED))
				prafter(GetTxPrevNode(ip),GetTxNextNode(ip),0);
			 break;
			}
		 else
			ad[operand] = GetTxOperandAd(ip,operand);
		} /* for(vt = 0; vt < vtx; ++vt) */

	if((vt == vtx)		/* No match, but value is cheaper. */
			&& IsAdAddrIndex(Lvalue) 
			&& GetAdAddrIndex(Lvalue) < vt) 
		{		/* Note that if Lvalue is a NAQ, it cannot be a
				 * value of a MOVA, and furthermore, it must already
				 * be in vttab by now.
				 */
		 ad[operand] = Lvalue;
		 if(legalgen(GetTxOpCodeX(ip),
				GetTxOperandType(ip,0),
				GetAdMode(ad[0]),
				GetTxOperandType(ip,1),
				GetAdMode(ad[1]),
				GetTxOperandType(ip,2),
				GetAdMode(ad[2]),
				GetTxOperandType(ip,3),
				GetAdMode(ad[3])))
			{if(DBdebug(0,XV_USED))
				prbefore(GetTxPrevNode(ip),GetTxNextNode(ip),0,
				        "value tracing");
			 PutTxOperandAd(ip,operand,ad[operand]);
			 if(DBdebug(0,XV_USED))
				prafter(GetTxPrevNode(ip),GetTxNextNode(ip),0);
			}
		 else
			ad[operand] = GetTxOperandAd(ip,operand);
		}
	} /* for(operand= GetOpFirstOp(op_code); ...) */
 return;					/* Normal return. */
}
	STATIC void
forget(Lvalue)			/* This subroutine causes the Value Tracing */
				/* table to forget its obsolete knowledge. */
				/* This occurs whenever a value is changed */
				/* and requires us to forget what knew the */
				/* old knowledge. */
register AN_Id Lvalue;		/* Value that is changing.	*/

{
 register unsigned int limit;
 extern unsigned int praddr();	/* Prints an address.	*/
 register unsigned int vt;	/* Value Tracing table index counter.	*/

 limit = vtablim;
 for(vt = 0; vt < limit; vt++)			/* Scan the table. */
	{if(vttab[vt].vvalue == NULL)		/* If we know nothing, */
		continue;			/* skip it. */
	 if(IsAdUses(vttab[vt].vvalue,Lvalue))	/* If we know about this, */
		{if(DBdebug(3,XVTRACE))
			{(void) fprintf(stdout,
				"%cvalue tracing: forgetting: ",ComChar);
			 (void) praddr(vttab[vt].vaddr,vttab[vt].vtype,stdout);
			 putchar(NEWLINE);
			}
		 vttab[vt].vvalue = NULL;	/* forget it. */
		}
	}

 return;
}


	STATIC void
allforget()		/* Forget tracing knowledge.	*/

{register AN_Id an_id;
 register unsigned int limit;
 register unsigned int vt;	/* Value  Tracing table index. */

 limit = vtablim;
 for(vt = 0, an_id = GetAdNextGNode((AN_Id)NULL); 
	an_id != (AN_Id)NULL && vt < limit; 
		an_id = GetAdNextGNode(an_id), vt++)
	{vttab[vt].vaddr = an_id;
	 vttab[vt].vvalue = NULL;
	}

 if(DBdebug(3,XVTRACE))
	(void) fprintf(stdout,"%cvalue tracing: forgetting everything\n",
		ComChar);
 return;
}
	STATIC void
fixup(ip,operand,name)		/* Process an operand used in an instr      */
				/* that does not fall within the GNAQ list. */
				/* Currently, rm in n(rm) is replaced with  */
				/* a better choice.                         */
register TN_Id ip;
unsigned int operand;
register AN_Id name;

{OperandType type;		/* Type  of an operand.	*/
 register unsigned int vt;
 unsigned int vtx;		/* Value Tracing table index. */
 AN_Id an_id;
 char *expression;
 extern void prafter();		/* Print nodes after an optimization.	*/
 extern void prbefore();	/* Print nodes before an optimization.	*/
 AN_Id target;
 AN_Id Lvalue;

 if(!IsAdDisp(name))				/* Handle only register disp.*/
	 return;				/* Something else: give up. */
 else if(IsTxOperandVol(ip,operand))		/* Is it volatile? */
	return;					/* Yes, leave it alone. */
 target = GetAdUsedId(name,0);			/* All the way to end. */
 if((!IsAdAddrIndex(target)) ||			/* Might we know its value? */
		((vtx = GetAdAddrIndex(target)) >= vtablim))
	return;					/* NO. */
 Lvalue = vttab[vtx].vvalue;			/* Current value. */
 if(Lvalue == NULL)				/* Do we know the value? */
	return;					/* No. */
 for(vt = 0; vt < vtx; vt++)		/* See if we find earlier one.*/
	{if(Lvalue != vttab[vt].vvalue)
		continue;			/* Not this one. */
	 if(vttab[vt].MOVA == 1)		/* Values of MOVA's differ. */
		continue;
	 if(!IsAdCPUReg(vttab[vt].vaddr))	/* Must be CPU register. */
		continue;			/* Isn't. */
						/* Replace the register number*/
						/* with a better one */
	 if(DBdebug(0,XV_USED))
		prbefore(GetTxPrevNode(ip),
			GetTxNextNode(ip),0,"value tracing: fixup");
	 type = GetTxOperandType(ip,operand);
	 expression = GetAdExpression(type,name);
	 an_id = GetAdDisp(type,expression,GetAdRegA(vttab[vt].vaddr));
	 PutTxOperandAd(ip,operand,an_id);
	 if(DBdebug(0,XV_USED))
		prafter(GetTxPrevNode(ip),GetTxNextNode(ip),0);
	}
}
	STATIC void
fixupA(ip)			/* Process a MOVA instruction.                */
				/* If the address of the source has already   */
				/* been taken, then change the MOVA to a MOV. */
register TN_Id ip;		/* Node to work on.	*/

{OperandType Ntype;		/* Type of new source.	*/
 AN_Id Nvalue;			/* AN_Id of new source.	*/
 AN_Id name;			/* AN_Id of source operand.	*/
 extern void prafter();		/* Print nodes after an optimization.	*/
 extern void prbefore();	/* Print nodes before an optimization.	*/
 register unsigned int vt;	/* Value tracing table index.	*/
 register unsigned int vtx;	/* Value tracing table index.	*/

 name = GetTxOperandAd(ip,MPA_SOURCE);		/* Name of source.	*/
 if((!IsAdAddrIndex(name)) ||
		((vtx = GetAdAddrIndex(name)) >= vtablim))
	return;

 for(vt = 0; vt < vtx; vt++)		/* Look for earlier one. */
	{if(name != vttab[vt].vvalue)		/* Value match?	*/
		continue;
	 if(vttab[vt].MOVA != 1)
		continue;			/* No.	*/
	 Nvalue = vttab[vt].vaddr;		/*This address has same value.*/
	 Ntype = vttab[vt].vtype;
	 if(DBdebug(0,XV_USED))
		prbefore(GetTxPrevNode(ip),
			GetTxNextNode(ip),0,"value tracing: fixupA");
	 PutTxOperandAd(ip,MPA_SOURCE,Nvalue);
	 PutTxOperandType(ip,MPA_SOURCE,Ntype);
	 PutTxOpCodeX(ip,G_MOV);
	 if(DBdebug(0,XV_USED))
		prafter(GetTxPrevNode(ip),GetTxNextNode(ip),0);
	} /* END OF for(vt = 0; vt < vtx; vt++) */
}
