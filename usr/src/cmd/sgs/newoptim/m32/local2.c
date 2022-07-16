/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/local2.c	1.9"

#include	<stdio.h>
#include	"defs.h"
#include	"debug.h"
#include	"ANodeTypes.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"olddefs.h"
#include	"OpTabTypes.h"
#include	"RoundModes.h"
#include	"ALNodeType.h"
#include	"TNodeTypes.h"
#include	"FNodeTypes.h"
#include	"RegId.h"
#include	"RegIdDefs.h"
#include	"ANodeDefs.h"
#include	"FNodeDefs.h"
#include	"OpTabDefs.h"
#include	"TNodeDefs.h"
#include	"optim.h"
#include	"optab.h"
#include	"sgs.h"

#define	DESTINATION	3

	void
pass2(Tfile,ldtab)
FILE *Tfile;			/* Intermediate file for text.	*/
struct ldent ldtab[];		/* The Live-Dead Table for GRA.	*/

{
 extern FN_Id FuncId;		/* FN_Id of most recently seen function. */
 extern void PropSafe();	/* Propagates MMIO safety.	*/
 extern void fatal();
 extern void fprinst();		/* Print instruction.	*/
 extern void init();		/* Initializes portable code improver.	*/
 extern void handle_function_tags();
 register TN_Id lastnode;	/* TN_Id of last node inserted.	*/
 unsigned int op_code;
 extern void sequence2();	/* Pass 2 optimization sequencing.	*/
 extern void print_debug();	/* Pass 2 hook for DebugInfo.c */
 extern void init_line_section(),
	     exit_line_section(); /* Defined in DebugInfo.c: set up
				     and close out .line section 
				     code. */

 init_line_section(); /* Defines ..line.b, emits beginning of .text
			 label */
 while((lastnode = ReadTxNodeBefore((TN_Id) NULL,Tfile)) != NULL) {

 /* Read the text list.*/

    switch(op_code = GetTxOpCodeX(lastnode)) {	/* Process each node. */
   
    case TAIL:			/* End of function.	*/
	DelTxNode(lastnode);	/* Remove TAIL node.	*/
	GetFnPrivate(FuncId);	/* Restore private nodes. */
	PropSafe();		/* Propagate MMIO safety. */
	sequence2(ldtab);	/* Do optimizations.	*/

	print_debug(ldtab, (void *)FuncId);

	printf("\t.text\n");
	for(ALLN(lastnode))	/* Write out text list.	*/
	    fprinst(stdout,-1,lastnode);
	init();
	DelTxNodes((TN_Id) NULL,(TN_Id) NULL);
	if(IsFnCandidate(FuncId))
	    HideAdCandidate();
	else
	    DelAdPrivateNodes();
	endcase;

		/* Written out in pass 1. */
    case PLOWER:
    case PS_DATA:
    case PS_PREVIOUS:
    case PS_SECTION:
    case PS_TEXT:
    case PS_BSS:
    case PS_COMM:
    case PS_FILE:
    case PS_GLOBL:
    case PS_IDENT:
    case PS_LOCAL:
    case PS_SET:
    case PS_TYPE:
    case PS_VERSION:
    case PS_WEAK:
        fatal("pass2: illegal pseudo-op in text list (%u).\n", op_code);
	endcase;

    case ISAVE:			/* Identify function. */
    case SAVE:
	FuncId = GetFnId(GetTxOperandAd(GetTxPrevNode( lastnode),0));
	endcase;

    default:			/* Normal instruction */
	endcase;
    } /* END OF switch(op_code = GetTxOpCodeX(lastnode)) */
 } /* END OF while(lastnode = ReadTxNodeBefore */
 exit_line_section();	/* define ..line.e */
 handle_function_tags();	/* backpatch the inline info */
 return;				/* just exit */
}
	void
PropSafe()			/* Propagate MMIO safety among absolute	*/
				/* address nodes.	*/

{register AN_Id NN;		/* Similar address with no Number.	*/
 register AN_Id an_id;
 extern enum CC_Mode ccmode;

 if(ccmode != Transition)
	return;

 for(an_id = GetAdNextNode((AN_Id) NULL);
		an_id;
		an_id = GetAdNextNode(an_id))
	{if(!IsAdAbsolute(an_id))		/*We care only about absolute.*/
		continue;			/* This one wasn't.	*/
	 if((NN = GetAdNoNumber(an_id)) == NULL)	/* Is there one like */
		continue;			/* this with no number? no */
	 if(NN == an_id)			/* If it is us, don't bother. */
		continue;			/* It is us.	*/
	 if(IsAdSafe(NN))			/* If that one is safe,	*/
		PutAdSafe(an_id,TRUE);		/* we are, too.	*/
	}
 return;
}
	void
ldinit()
{
 extern FN_Id FuncId;			/* FN_Id of current function.	*/
 extern int REGS[];			/* Set of GNAQs for l/d analysis. */
 extern int RETREG[];			/* Set of GNAQs live at function exit. */
 AN_Id an_id;
 extern void fatal();
 unsigned int ld;
 extern unsigned int ld_maxbit;		/* Max number of vars for l/d anal */
 extern unsigned int ld_maxword;	/* Words into which ld_maxbits fits*/
 unsigned int rvregs;			/* Number of return registers. */
 unsigned int vector;

					/* Set live/dead bits 	*/
					/* for GNAQ variables.	*/
					/* Note that REGS 	*/
					/* is a misnomer.	*/
 an_id = GetAdPrevGNode((AN_Id) NULL);
 if(an_id == (AN_Id) NULL)			/* If none, register entries */
						/* are missing. */
	fatal("ld_init: No register entries in live/dead table.\n");
 ld_maxbit = GetAdAddrIndex(an_id) + 1;
 if(ld_maxbit > LIVEDEAD)
	ld_maxbit = LIVEDEAD;
 ld_maxword = (ld_maxbit+WDSIZE-1)/WDSIZE;

 for(vector = 0; vector < ld_maxword; vector++)
	{REGS[vector] = ~0;
	 RETREG[vector] = 0;
	}
 if((ld_maxbit % WDSIZE) != 0)
	REGS[ld_maxbit / WDSIZE] = ~ (- (1<<(ld_maxbit % WDSIZE)));
			
 for(vector = (ld_maxbit/WDSIZE) + 1; vector < NVECTORS; vector++)
	{REGS[vector] = 0;
	 RETREG[vector] = 0;
	}

					/* Set return live bits.	*/
 rvregs = GetFnRVRegs(FuncId);
 switch(rvregs)
	{case 0:
		endcase;
	 case 1:
		ld = GetAdAddrIndex(GetAdCPUReg(CREG0));
		RETREG[ld / WDSIZE] |= (1 << (ld % WDSIZE));
		endcase;
	 case 2:
		ld = GetAdAddrIndex(GetAdCPUReg(CREG0));
		RETREG[ld / WDSIZE] |= (1 << (ld % WDSIZE));
		ld = GetAdAddrIndex(GetAdCPUReg(CREG1));
		RETREG[ld / WDSIZE] |= (1 << (ld % WDSIZE));
		endcase;
	} /* END OF switch(rvregs) */

}
	boolean
ExternAlias(tn_id)	/* TRUE if externs aliased.	*/
TN_Id tn_id;		/* Current instruction.*/

{extern boolean IsDispAPFP();	/* TRUE if displacement form CAP or CFP. */
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 register AN_Id an_id;		/* Address-Node identifier for loop index. */
 unsigned int operand;		/* Instruction operand counter.	*/

#ifdef W32200
 extern boolean IsTxSets();	/* TRUE if text node sets an address.	*/
 AN_Id an_idA;			/* Identifier of register A of some operand. */
 AN_Id dest;			/* Identifier of a destination operand.	*/
 unsigned int op_code;		/* Operation code index of an instruction. */
 AN_Id opd2;			/* Second operand of some instruction.	*/
 AN_Id opd3;			/* Third operand of some instruction.	*/
 RegId reg_id;			/* Identifier of a register.	*/
 TN_Id tn_idA;			/*Text-Node identifier for another loop index.*/
#endif

 for(ALLOP(tn_id,operand))		/* Scan each operand.	*/
	{an_id = GetTxOperandAd(tn_id,operand);
	 if(IsAdNAQ(an_id))		/* Skip good ones.	*/
		continue;
	 if(IsAdAbsolute(an_id))
		continue;
	 if(IsDispAPFP(an_id))
		continue;
#ifdef W32200
	 if(IsAdPostIncr(an_id) && (GetAdRegA(an_id) == CSP))
		continue;
#endif
	 if(	(IsAdDisp(an_id))	/* If other displacement, */
	    && !IsAdNumber(an_id)
	   )
		continue;

#ifdef W32200
	 if(IsAdIndexRegDisp(an_id) || IsAdIndexRegScaling(an_id))
		{reg_id = GetAdRegA(an_id);	/* Second set.	*/
		 an_idA = GetAdCPUReg(reg_id);	/* Get its an_id. */
		 for(tn_idA = GetTxPrevNode(tn_id);
				tn_idA != before;
				tn_idA = GetTxPrevNode(tn_idA))
			{if(!IsTxSets(tn_idA,an_idA))
				continue;
			 dest = GetTxOperandAd(tn_idA,DESTINATION);
			 if(dest != an_idA)
				break;	/* Set: not destination. */
			 op_code = GetTxOpCodeX(tn_idA);
			 if(	(op_code != G_MOV)
			    &&	(op_code != G_ADD3)
			    &&	(op_code != G_SUB3)
			   )
				break; /*Not destination of mov or add*/
			 opd2 = GetTxOperandAd(tn_idA,SECOND_SRC);
			 opd3 = GetTxOperandAd(tn_idA,THIRD_SRC);
			 if(IsAdImmediate(opd2) && !IsAdNumber(opd2))
				goto OPERAND_OK;
			 if(IsAdImmediate(opd3) && !IsAdNumber(opd3))
				goto OPERAND_OK;
			 break;		/* Neither source immediate. */
			} /* END OF for(tn_idA = tn_id; tn_idA ...) */
		} /* END OF if(IsAdIndexRegDisp(an_id) || ...) */
#endif

	 return(TRUE);

#ifdef W32200
 OPERAND_OK:
		;
#endif

	} /* END OF for(ALLOP(tn_id,operand) */

 return(FALSE);
}
	void
fixnumnreg(newnreg)		/* Change num regs saved/restored.	*/
unsigned int newnreg;

{
 extern AN_Id i0;		/* AN_Id of immediate 0.	*/
 register unsigned int n;	/* Number of registers save. */
 register TN_Id pn;		/* TN_Id for scanning text. */

 for(ALLN(pn))					/* Scan text.	*/
	{switch(GetTxOpCodeX(pn))
		{case ISAVE:
		 case IRET:
						/* For these, put in the */
						/* number of registers	*/
						/* to save.	*/
			PutTxOperandAd(pn,0,GetAdAddToKey(Tword,i0,
				(long int) newnreg));
			endcase;
		 case SAVE:
		 case RESTORE:
						/* For these, put in the */
						/* RegId of the lowest	*/
						/* register to save.	*/
			n = GetRegNo(CFP) - newnreg;
			PutTxOperandAd(pn,0,GetAdCPUReg(GetRegId((int) n)));
			endcase;
		}
	}
 return;
}
	void
chkauto()		 /* Eliminate auto area, if no more autos. */

{extern FN_Id FuncId;		/* FN_Id of current function.	*/
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 unsigned short int OpCodeX;	/* Operation Code Index. */
 AN_Id an_id;
 extern void fatal();
 AN_Mode mode;
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 register unsigned int operand;
 extern TN_Id skipprof();	/* Skips profiling code, if any.	*/
 register TN_Id tn_id;

 if(GetFnLocSz(FuncId) != 0)
	{if(IsFnBlackBox(FuncId))
		return;		/* Might be reference in black box. */
	 for(ALLN(tn_id))				/* Scan text. */
		{OpCodeX = GetTxOpCodeX(tn_id);
		 if((OpCodeX == SAVE) || (OpCodeX == RESTORE))
			continue;	/* i.e., ignore %fp for these instr */
		 if(IsOpPseudo(OpCodeX))		/* Ignore */
			continue;			/* pseudo-ops.	*/
		 if(IsOpAux(OpCodeX))
			continue;
		 for(operand = 0; operand < Max_Ops; operand++)
			{if(GetTxOperandType(tn_id,operand) == Tnone)
				break;
			 an_id = GetTxOperandAd(tn_id,operand);
			 switch(mode = GetAdMode(an_id))
				{case Immediate:
				 case Absolute:
				 case AbsDef:
					endcase;
				 case CPUReg:
				 case Disp:
				 case DispDef:
				 case PreDecr:
				 case PreIncr:
				 case PostDecr:
				 case PostIncr:
					if(GetAdRegA(an_id) == CFP)
						return;	/* Found ref to %fp. */
					endcase;
				 case IndexRegDisp:
				 case IndexRegScaling:
					if(GetAdRegA(an_id) == CFP)
						return;	/* Found ref to %fp. */
					if(GetAdRegB(an_id) == CFP)
						return;	/* Found ref to %fp. */
					endcase;
				 case MAUReg:
				 case StatCont:
					endcase;
				 case Undefined:
				 default:
					fatal("chkauto: unknown mode (%d).\n",
						mode);
					endcase;
				} /* END OF switch(mode = GetAdMode(an_id) */
			} /* END OF for(operand = 0; operand < MAXOPS; 
				operand++) */
		} /* END OF for(ALLN(tn_id)) */
	} /* END OF if(GetFnLocSz(FuncId) != 0) */
 PutFnLocSz(FuncId,0);
 tn_id = skipprof(GetTxNextNode((TN_Id) NULL));		/* Skip label, SAVE and
						   profiling code */

						/* If first non-profiling */
						/* instruction adds to the */
						/* stack-pointer, */
 an_id = GetTxOperandAd(tn_id,DESTINATION);
 if((GetTxOpCodeX(tn_id) == G_ADD3) && 
		IsAdCPUReg(an_id) &&
		(GetAdRegA(an_id) == CSP))
	{DelTxNode(tn_id);			/* remove auto area. */
	 ndisc += 1;				/* Update number discarded. */
	}
 return;
}
	void
wrapup()	/* Print unprocessed text and update statistics file */

{
 extern void fprinst();		/* Prints an instruction.	*/
 extern boolean identflag;	/* Output ident info?		*/
 register TN_Id  tn_id;

 printf("\t.text\n");				/* Make .text line. */
 if(GetTxNextNode((TN_Id) NULL) != NULL)	/* Any nodes in text list? */
	for(ALLN(tn_id))			/* If so, write them out. */
		fprinst(stdout,-1,tn_id);
 if(identflag)					/* Output ident info.	*/
	printf("\t.ident\t\"optim: %s\"\n",SGU_REL);

}
	void
dstats()	 	/* Print stats on machine dependent optimizations */
{extern unsigned int LICMrem;	/* Number of instructions removed from loops.*/
 extern unsigned int MaxGNAQNodes;	/* Maximum GNAQ list size.	*/
 /*extern int fprintf();		** Prints to stream; in C(3) library.	*/

 (void) fprintf(stderr,"%8u instructions removed from loop(s).\n",
		LICMrem);
 (void) fprintf(stderr,"%8u maximum nodes in GNAQ list.\n",MaxGNAQNodes);
 return;
}
