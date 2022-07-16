/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/UseSet.c	1.10"

/****************************************************************/
/*				UseSet.c			*/
/*								*/
/*		This file contains the uses and sets utilities.	*/
/*	All the operations that pertain to the information used	*/
/*	or set by an instruction are meant to be contained	*/
/*	in this file.						*/
/*								*/
/*								*/
/****************************************************************/

#include	<stdio.h>
#include	"defs.h"
#include	"ANodeTypes.h"
#include	"olddefs.h"
#include	"OpTabTypes.h"
#include	"RegId.h"
#include	"ANodeDefs.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"RegIdDefs.h"
#include	"RoundModes.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"ALNodeType.h"
#include	"FNodeTypes.h"
#include	"FNodeDefs.h"
#include	"optab.h"
#include	"Target.h"

	/* private functions */
STATIC AN_Id UsEs();
	void
sets(tn_id,array,words)		/* Returns sets bits for an instruction. */
				/* This function clears array and then	*/
				/* or's the desired bits into it.	*/
TN_Id tn_id;			/* TN_Id of the instruction.	*/
unsigned long int array[];		/* Where to put results.	*/
unsigned int words;		/* Number of result words wanted.	*/

{extern void GetOpSets();	/* Get set bits for this op-code.	*/
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 AN_Id an_id;			/* AN_Id of an operand.	*/
 unsigned int dst;		/* Destination operands.	*/
 struct opent *op;		/* Operation-code table pointer.	*/
 register unsigned int operand;	/* Operand counter for instruction.	*/
 extern struct opent optab[];	/* The operation-code table.	*/
 unsigned int opx;		/* Operation code index.	*/
 register OperandType type;	/* Type of an operand.	*/
 register unsigned int vector;	/* Word counter for array.	*/

 for(vector = 0; vector < words; vector++)	/* Initialize array.	*/
	array[vector] = 0;

 opx = GetTxOpCodeX(tn_id);			/* Get instruction's op-codex.*/
 GetOpSets(opx,array,words);			/* Put in bits determined */
						/* by op-code alone.	*/

 if(IsTxAux(tn_id))				/* If auxiliary node,	*/
	return;					/* that's all there is.	*/

 op = &optab[opx];				/* Pointer to op-code entry. */
 dst = op->odstops;

 for(operand = 0; operand < Max_Ops; operand++)	/* Do each operand.	*/
	{type = GetTxOperandType(tn_id,operand);	/* Get operand's type.*/
	 if(type == Tnone)			/* If Tnone, we are done */
		break;				/* with this instruction. */
	 if(dst & 1)				/* If operand is a destination*/
		{an_id = GetTxOperandAd(tn_id,operand);	/* Get operand's Id.*/
		 if(IsAdAddrIndex(an_id))		/* If address index, */
			{vector = GetAdAddrIndex(an_id);
			 if((vector / (B_P_BYTE*sizeof(unsigned int)))
					< words)
				set_bit(array,vector);
			}
		 if((type == Tdblext) &&	/* If more than one,	*/
				IsAdCPUReg(an_id))
			{an_id =
				GetAdCPUReg(GetNextRegId(GetNextRegId(GetAdRegA(an_id))));
			 if(IsAdAddrIndex(an_id))
				{vector = GetAdAddrIndex(an_id);
				 if(vector / (B_P_BYTE*sizeof(unsigned int))
						< words)
					set_bit(array,vector);
				}
			} /* END OF if((type == Tdblext) ... */
		 if(((type == Tdouble) || (type == Tdblext)) &&
				IsAdCPUReg(an_id))
			{an_id =
				GetAdCPUReg(GetNextRegId(GetAdRegA(an_id)));
			 if(IsAdAddrIndex(an_id))
				{vector = GetAdAddrIndex(an_id);
				 if(vector / (B_P_BYTE*sizeof(unsigned int))
						< words)
					set_bit(array,vector);
				}
			} /* END OF if((type == Tdouble) &&IsAAdCPUReg(an_id))*/
		} /* END OF if(dst & 1) */
	 dst >>= 1;
	} /* END OF for(operand = 0; operand < Max_Ops; operand++) */

 return;
}
	void
uses(tn_id,array,words)		/* Returns uses bits for an instruction. */
				/* This function clears array and then	*/
				/* or's the desired bits into it.	*/
TN_Id tn_id;			/* TN_Id of the instruction.	*/
unsigned long int array[];	/* Where to put results.	*/
unsigned int words;		/* Number of result words wanted.	*/

{extern FN_Id FuncId;		/* Id of current function. */
 extern void GetOpUses();	/* Get uses bits for this op-code.	*/
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 STATIC AN_Id UsEs();		/* Returns AN_Id of address used.	*/
 AN_Id an_id;			/* AN_Id of an operand.	*/
 extern enum CC_Mode ccmode;	/* cc -X? */
 unsigned int dst;
 unsigned int index;		/* Used address index.	*/
 unsigned int ldavail;		/* Number of items in GNAQ list. */
 register unsigned int operand;	/* Operand counter for instruction.	*/
 struct opent *op;		/* Operation-code table pointer.	*/
 unsigned int opcode;		/* Operation-code. */
 extern struct opent optab[];	/* The operation-code table.	*/
 unsigned int src;
 register  OperandType type;	/* Type of an operand.	*/
 AN_Id used_an_id;		/* An AN_Id used by an_id.	*/
 register unsigned int vector;	/* Word counter for array.	*/

 for(vector = 0; vector < words; vector++)	/* Initialize array.	*/
	array[vector] = 0;

 if(IsTxBlackBox(tn_id))   /* If it is a black box, make everything live.  */
	{ldavail = GetAdAddrIndex(GetAdPrevGNode((AN_Id) NULL)) + 1;
	 for(vector = 0; vector < ldavail; vector++)
		if((vector / WDSIZE) < words)
			set_bit(array,vector);
	 return;
	}

 opcode = GetTxOpCodeX(tn_id);
 GetOpUses(opcode,array,words);	/* Put in bits determined */
				/* by op-code alone.	*/

 if(IsTxAux(tn_id))				/* If auxiliary node,	*/
	return;					/* that's all there is.	*/

 op = &optab[opcode];				/* Pointer to op-code entry. */
 src = op->osrcops;
 dst = op->odstops;

 for(operand = 0; operand < Max_Ops; operand++)	/* Do each operand.	*/
	{type = GetTxOperandType(tn_id,operand);	/* Get operand's type.*/
	 if(type == Tnone)			/* If Tnone, we are done */
		break;				/* with this instruction. */
	 an_id = GetTxOperandAd(tn_id,operand);	/* Get operand's AN_Id.	*/
	 if((src & 1) | (dst & 1))		/* If operand is a source */
						/* or destination,	*/
		{index = 0;
		 while((used_an_id = UsEs(an_id,index++)) != NULL) /*Get used adresses.*/
			{if(IsAdAddrIndex(used_an_id))	/* If address index, */
				{vector = GetAdAddrIndex(used_an_id);
				 if((vector / WDSIZE) < words)
					set_bit(array,vector);
				}
			} /* END OF while(used_an_id = UsEs(an_id,index++) */
		} /* END OF if((src & 1) | (dst & 1)) */

	 if(src & 1)				/* If operand is source, */
		{if(IsAdAddrIndex(an_id))		/* If address index, */
			{vector = GetAdAddrIndex(an_id);
			 if((vector / WDSIZE) < words)
				set_bit(array,vector);
			}
		 if((type == Tdblext) &&
				IsAdCPUReg(an_id))
			{used_an_id =
				GetAdCPUReg(GetNextRegId(GetNextRegId(GetAdRegA(an_id))));
			 if(IsAdAddrIndex(used_an_id))
				{vector = GetAdAddrIndex(used_an_id);
				 if((vector / WDSIZE) < words)
					set_bit(array,vector);
				}
			}
		 if(((type == Tdouble) || (type == Tdblext)) &&
				IsAdCPUReg(an_id))
			{used_an_id =
				GetAdCPUReg(GetNextRegId(GetAdRegA(an_id)));
			 if(IsAdAddrIndex(used_an_id))
				{vector = GetAdAddrIndex(used_an_id);
				 if((vector / WDSIZE) < words)
					set_bit(array,vector);
				}
			} /* END OF if((type == Tdouble) &&IsAAdCPUReg(an_id))*/
		} /* END OF if(src & 1) */
	 dst >>= 1;
	 src >>= 1;
	} /* END OF for(operand = 0; operand < Max_Ops; operand++) */

					/* If this func calls setjmp, */
					/* and this instr is a call, */
	if(IsFnSetjmp(FuncId) && IsTxAnyCall(tn_id))
		{for(an_id = GetAdNextGNode((AN_Id) NULL);
				an_id;
				an_id = GetAdNextGNode(an_id))
					/* and in transition mode,    */
					/* make stack variables live. */
			if((ccmode == Transition && IsAdNAQ(an_id) && !IsAdCPUReg(an_id))
					|| IsAdSV(an_id))
				{vector = GetAdAddrIndex(an_id);
				 if((vector / WDSIZE) < words)
					set_bit(array,vector);
				}
		}
 return;
}
	STATIC AN_Id
UsEs(an_id,index)		/* Returns AN_Id of used operand.	*/
				/* Returns NULL if specified index not	*/
				/* used.	*/
AN_Id an_id;			/* AN_Id of using operand.	*/
unsigned int index;		/* Arbitrary used operand index.	*/

{register AN_Id Used;
 switch(index)			/* Examine specified node.	*/
	{case 0:
		if((Used = GetAdUsedId(an_id,0)) != NULL)
			return(Used);
	 /* FALLTHRU */
	 case 1:
		if((Used = GetAdUsedId(an_id,1)) != NULL) 
			return(Used);
	 /* FALLTHRU */
	 case 2:
		if((Used = GetAdUsedId(an_id,0)) != NULL)
			{if((Used = GetAdUsedId(Used,0)) != NULL)
				return(Used);
			}
	 /* FALLTHRU */
	 case 3:
		if((Used = GetAdUsedId(an_id,0)) != NULL)
			{if((Used = GetAdUsedId(Used,1)) != NULL)
				return(Used);
			}
	 /* FALLTHRU */
	 case 4:
		if((Used = GetAdUsedId(an_id,1)) != NULL)
			{if((Used = GetAdUsedId(Used,0)) != NULL)
				return(Used);
			}
	 /* FALLTHRU */
	 case 5:
		if((Used = GetAdUsedId(an_id,1)) != NULL)
			{if((Used = GetAdUsedId(Used,1)) != NULL)
				return(Used);
			}
	}
 return((AN_Id) NULL);		/* No others are possible.	*/
}
	void
GetOpSets(OpIndex,array,words)	/* Get Implied sets information for opcode. */
				/* This function ORs the results into array, */
				/* so the caller must clear it when necessary.*/
unsigned short int OpIndex;	/* optab index of operation code.	*/
unsigned long int array[];	/* Where to put sets bits.	*/
unsigned int words;		/* Number of words wanted.	*/

{extern AN_Id A;		/* AN_Id of A condition code.	*/
 extern AN_Id C;		/* AN_Id of C condition code.	*/
 extern AN_Id N;		/* AN_Id of N condition code.	*/
 extern AN_Id V;		/* AN_Id of V condition code.	*/
 extern AN_Id X;		/* AN_Id of X condition code.	*/
 extern AN_Id Z;		/* AN_Id of Z condition code.	*/
 register AN_Id an_id;		/* AN_Id of address under study.	*/
 register unsigned int index;
 extern m32_target_math math_chip;
 struct opent *op;		/* Pointer to operation-code table entry. */
 extern struct opent optab[];	/* The operation-code table.	*/
 RegId regid;			/* Register identifier.	*/
 register unsigned int tempU;	/* Temporary op-code table entry word.	*/


 op = &optab[OpIndex];				/* Establish pointer.	*/

						/* Condition codes first. */
						/* NOTE: the order of the */
						/* following tests was	*/
						/* determined by a study */
						/* of the statistics of a */
						/* program believed to be */
						/* typical. If it is changed, */
						/* the execution time of this */
						/* optimizer is likely to */
						/* increase.	*/
 tempU = op->osetccs;				/* Condition codes set. */
 if(tempU)					/* If condition codes set, */
	{if(tempU & OCCCPUC)			/* If carry bit set,	*/
		{if(IsAdAddrIndex(C))		/* see if an address index */
			{index = GetAdAddrIndex(C);	/* Get address index. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OCCCPUC) == 0)
			goto DO_SET_REGS;
		}
	 if(tempU & OCCCPUN)			/* If negative bit set, */
		{if(IsAdAddrIndex(N))		/* see if an address index */
			{index = GetAdAddrIndex(N);	/* Get address index. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OCCCPUN) == 0)
			goto DO_SET_REGS;
		}
	 if(tempU & OCCCPUV)			/* If overflow bit set, */
		{if(IsAdAddrIndex(V))		/* see if an address index */
			{index = GetAdAddrIndex(V);	/* Get address index. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OCCCPUV) == 0)
			goto DO_SET_REGS;
		}
	 if(tempU & OCCCPUZ)			/* If zero bit set, */
		{if(IsAdAddrIndex(Z))		/* see if an address index */
			{index = GetAdAddrIndex(Z);	/* Get address index. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OCCCPUZ) == 0)
			goto DO_SET_REGS;
		}
	 if(tempU & OCCMAUASR)			/* If MAUASR bit set,	*/
		{if(IsAdAddrIndex(A))		/* see if an address index */
			{index = GetAdAddrIndex(A);	/* Get address index. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OCCMAUASR) == 0)
			goto DO_SET_REGS;
		}
	 if(tempU & OCCCPUX)		/* If extended-carry-borrow bit set, */
		{if(IsAdAddrIndex(X))		/* see if an address index */
			{index = GetAdAddrIndex(X);	/* Get address index. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OCCCPUX) == 0)
			goto DO_SET_REGS;
		}
	} /* END OF if(tempU) */

DO_SET_REGS:
 tempU = op->osetregs;				/* Now set CPU registers. */
 if(tempU)					/* If any set,	*/
	{if(tempU & OREGPSW)			/* Register PSW?	*/
		{an_id = GetAdCPUReg(CPSW);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OREGPSW) == 0)	/* If nothing left,	*/
			return;			/* we are done.	*/
		}
	 if((tempU & OREGASR) &&		/* Register ASR?
						 * #TYPE DOUBLE creates a G_MMOV op 
						 * whose set bit must be ignored in
						 * FPE mode. 
						 */
			(math_chip == we32106 || math_chip == we32206))
		{an_id = GetAdMAUReg(MASR);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OREGASR) == 0)	/* If nothing left,	*/
			return;			/* we are done.	*/
		}
	 if(tempU & OREGPC)			/* Register PC?	*/
		{an_id = GetAdCPUReg(CPC);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OREGPC) == 0)	/* If nothing left,	*/
			return;			/* we are done.	*/
		}
	 if(tempU & OREGSP)			/* Register SP?	*/
		{an_id = GetAdCPUReg(CSP);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OREGSP) == 0)	/* If nothing left,	*/
			return;			/* we are done.	*/
		}
	 if(tempU & OREGAP)			/* Register AP?	*/
		{an_id = GetAdCPUReg(CAP);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OREGAP) == 0)	/* If nothing left,	*/
			return;			/* we are done.	*/
		}
	 if(tempU & OREG0)			/* Register 0?	*/
		{an_id = GetAdCPUReg(CREG0);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OREG0) == 0)	/* If nothing left,	*/
			return;			/* We are done.	*/
		}
	 if(tempU & OREG1)			/* Register 1?	*/
		{an_id = GetAdCPUReg(CREG1);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OREG1) == 0)	/* If nothing left,	*/
			return;			/* we are done.	*/
		}
	 if(tempU & OREG2)			/* Register 2?	*/
		{an_id = GetAdCPUReg(CREG2);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OREG2) == 0)	/* If none left,	*/
			return;			/* we are done.	*/
		}
	 if(tempU & OREGFP)			/* Register FP?	*/
		{an_id = GetAdCPUReg(CFP);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OREGFP) == 0)	/* If none left,	*/
			return;			/* we are done.	*/
		}
	 if(tempU & OREG3_8)			/* Registers 3_8?	*/
		{for(regid = CREG3; regid != CREG9; regid = GetNextRegId(regid))
			{an_id = GetAdCPUReg(regid);	/* Get register AN_Id.*/
			 if(IsAdAddrIndex(an_id))/* See if has address index. */
				{index = GetAdAddrIndex(an_id);/*If so,get it.*/
				 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
					set_bit(array,index);
				}
			} /* END OF for(regid = CREG3; regid != CREG9; ... */
		 if((tempU &= ~OREG3_8) == 0)	/* If none left,	*/
			return;			/* we are done.	*/
		} /* END OF if(tempU  OREG3_8) */
	 if(tempU & OREGPCBP)			/* Register PCBP?	*/
		{an_id = GetAdCPUReg(CPCBP);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OREGPCBP) == 0)
			return;
		}
	 if(tempU & OREGISP)			/* Register ISP?	*/
		{an_id = GetAdCPUReg(CISP);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OREGISP) == 0)
			return;
		}
	 if(tempU & OREGMFPR)			/* Register MFPR?	*/
		{an_id = GetAdStatCont(MFPR);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);
			}
		 if((tempU &= ~OREGMFPR) == 0)
			return;
		}
	} /* END OF if(tempU) */

 return;
}
	void
GetOpUses(OpIndex,array,words)	/* Get Implied uses information for opcode. */
				/* This function ORs the results into array, */
				/* so the caller must clear it when necessary.*/
unsigned short int OpIndex;	/* optab index of operation code.	*/
unsigned long int array[];	/* Where to put uses bits.	*/
unsigned int words;		/* Number of words wanted.	*/

{extern AN_Id A;		/* AN_Id of A condition code.	*/
 extern AN_Id C;		/* AN_Id of C condition code.	*/
 extern AN_Id N;		/* AN_Id of N condition code.	*/
 extern AN_Id V;		/* AN_Id of V condition code.	*/
 extern AN_Id X;		/* AN_Id of X condition code.	*/
 extern AN_Id Z;		/* AN_Id of Z condition code.	*/
 register AN_Id an_id;		/* AN_Id of address under study.	*/
 register unsigned int index;
 struct opent *op;		/* Pointer to op-code table entry. */
 extern struct opent optab[];	/* The operation-code table.	*/
 RegId regid;			/* Register identifier.	*/
 register unsigned int tempU;	/* Temporary op-code table entry word.	*/

 op = &optab[OpIndex];				/* Establish pointer.	*/

						/* Condition codes first. */
						/* NOTE: the order of the */
						/* following tests was	*/
						/* determined by a study */
						/* of the statistics of a */
						/* program believed to be */
						/* typical. If it is changed, */
						/* the execution time of this */
						/* optimizer is likely to */
						/* increase.	*/
 tempU = op->ouseccs;				/* Condition codes used. */
 if(tempU)					/* If condition codes used, */
	{if(tempU & OCCCPUZ)			/* If zero bit used, */
		{if(IsAdAddrIndex(Z))		/* see if an address index */
			{index = GetAdAddrIndex(Z);	/* Get address index. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OCCCPUZ) == 0)
			goto DO_USED_REGS;
		}
	 if(tempU & OCCCPUN)			/* If negative bit used, */
		{if(IsAdAddrIndex(N))		/* see if an address index */
			{index = GetAdAddrIndex(N);	/* Get address index. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OCCCPUN) == 0)
			goto DO_USED_REGS;
		}
	 if(tempU & OCCCPUC)			/* If carry bit used,	*/
		{if(IsAdAddrIndex(C))		/* see if an address index */
			{index = GetAdAddrIndex(C);	/* Get address index. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OCCCPUC) == 0)
			goto DO_USED_REGS;
		}
	 if(tempU & OCCCPUV)			/* If overflow bit used, */
		{if(IsAdAddrIndex(V))		/* see if an address index */
			{index = GetAdAddrIndex(V);	/* Get address index. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OCCCPUV) == 0)
			goto DO_USED_REGS;
		}
	 if(tempU & OCCCPUX)		/* If extended-carry-borrow bit used, */
		{if(IsAdAddrIndex(X))		/* see if an address index */
			{index = GetAdAddrIndex(X);	/* Get address index. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OCCCPUX) == 0)
			goto DO_USED_REGS;
		}
	 if(tempU & OCCMAUASR)			/* If MAUASR bit used,	*/
		{if(IsAdAddrIndex(A))		/* see if an address index */
			{index = GetAdAddrIndex(A);/* Get address index. */
			 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
				set_bit(array,index);	/* Or in new bit. */
			}
		 if((tempU &= ~OCCMAUASR) == 0)
			goto DO_USED_REGS;
		}
	} /* END OF if(tempU) */

DO_USED_REGS:
 tempU = op->ouseregs;				/* Now used CPU registers. */
 if(tempU)					/* If any used,	*/
	{if(tempU & OREGSP)			/* Register SP?	*/
		{an_id = GetAdCPUReg(CSP);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / WDSIZE) < words)
				set_bit(array,index);
			}
		 if((tempU &= ~OREGSP) == 0)
			return;
		}
	 if(tempU & OREGAP)			/* Register AP?	*/
		{an_id = GetAdCPUReg(CAP);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / WDSIZE) < words)
				set_bit(array,index);
			}
		 if((tempU &= ~OREGAP) == 0)
			return;
		}
	 if(tempU & OREG2)			/* Register 2?	*/
		{an_id = GetAdCPUReg(CREG2);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / WDSIZE) < words)
				set_bit(array,index);
			}
		 if((tempU &= ~OREG2) == 0)
			return;
		}
	 if(tempU & OREGFP)			/* Register FP?	*/
		{an_id = GetAdCPUReg(CFP);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / WDSIZE) < words)
				set_bit(array,index);
			}
		 if((tempU &= ~OREGFP) == 0)
			return;
		}
	 if(tempU & OREG3_8)			/* Registers 3_8?	*/
		{for(regid = CREG3; regid != CREG9; regid = GetNextRegId(regid))
			{an_id = GetAdCPUReg(regid);	/* Get register AN_Id.*/
			 if(IsAdAddrIndex(an_id))/* See if has address index. */
				{index = GetAdAddrIndex(an_id);/*If so,get it.*/
				 if((index / WDSIZE) < words)
					set_bit(array,index);
				}
			} /* END OF for(regid = CREG3; regid != CREG9; ... */
		 if((tempU &= ~OREG3_8) == 0)
			return;
		} /* END OF if(tempU  OREG3_8) */

#ifdef W32200
	 if(tempU & OREG16_23)			/* Registers 16_23?	*/
		{if(cpu_chip == we32200)
			{for(regid = CREG16;
					regid != CREG24;
					regid = GetNextRegId(regid))
				{an_id = GetAdCPUReg(regid);	/* Get reg.*/
				 if(IsAdAddrIndex(an_id))
					{index = GetAdAddrIndex(an_id);
					 if((index /
						(B_P_BYTE*sizeof(unsigned int)))
						< words)
						set_bit(array,index);
					}
				} /* END OF for(regid = CREG16; ... */
			} /* END OF if(cpu_chip == we32200) */
		 if((tempU &= ~OREG16_23) == 0)	/* If none left,	*/
			return;			/* we are done.	*/
		} /* END OF if(tempU & OREG16_23) */
#endif

	 if(tempU & OREGENAQS)
		{OrAdEnaqs(array,words);	/* OR in the ENAQ bits.	*/
		 if((tempU &= ~OREGENAQS) == 0)	/* If none left,	*/
			return;			/* we are done.	*/
		} /* END OF if(tempU & OREGENAQS) */
	 if(tempU & OREGSENAQS)
		{OrAdSenaqs(array,words);	/* OR in the SENAQ bits. */
		 if((tempU &= ~OREGSENAQS) == 0)/* If none left, */
			return;			/* we are done.	*/
		} /* END OF if(tempU & OREGSENAQS) */
	 if(tempU & OREGSNAQS)
		{OrAdSnaqs(array,words);	/* OR in the SNAQ bits. */
		 if((tempU &= ~OREGSNAQS) == 0)	/* If none left, */
			return;			/* we are done.	*/
		} /* END OF if(tempU & OREGSNAQS) */

#ifdef W32200
	 if(tempU & OREG24_31)			/* Registers 24_31?	*/
		{for(regid = CREG24;
				regid != REG_NONE;
				regid = GetNextRegId(regid))
			{an_id = GetAdCPUReg(regid);	/* Get register AN_Id.*/
			 if(IsAdAddrIndex(an_id))/* See if has address index. */
				{index = GetAdAddrIndex(an_id);/*If so,get it.*/
				 if((index / (B_P_BYTE*sizeof(unsigned int))) < words)
					set_bit(array,index);
				}
			} /* END OF for(regid = CREG24; regid != REG_NONE; ... */
		 if((tempU &= ~OREG24_31) == 0)	/* If none left,	*/
			return;			/* we are done.	*/
		} /* END OF if(tempU & OREG24_31) */
#endif

	 if(tempU & OREG0)			/* Register 0?	*/
		{an_id = GetAdCPUReg(CREG0);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / WDSIZE) < words)
				set_bit(array,index);
			}
		 if((tempU &= ~OREG0) == 0)
			return;
		}
	 if(tempU & OREG1)			/* Register 1?	*/
		{an_id = GetAdCPUReg(CREG1);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / WDSIZE) < words)
				set_bit(array,index);
			}
		 if((tempU &= ~OREG1) == 0)
			return;
		}
	 if(tempU & OREGPSW)			/* Register PSW?	*/
		{an_id = GetAdCPUReg(CPSW);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / WDSIZE) < words)
				set_bit(array,index);
			}
		 if((tempU &= ~OREGPSW) == 0)
			return;
		}
	 if(tempU & OREGASR)			/* Register ASR?	*/
		{an_id = GetAdMAUReg(MASR);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / WDSIZE) < words)
				set_bit(array,index);
			}
		 if((tempU &= ~OREGASR) == 0)
			return;
		}
	 if(tempU & OREGPCBP)			/* Register PCBP?	*/
		{an_id = GetAdCPUReg(CPCBP);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / WDSIZE) < words)
				set_bit(array,index);
			}
		 if((tempU &= ~OREGPCBP) == 0)
			return;
		}
	 if(tempU & OREGISP)			/* Register ISP?	*/
		{an_id = GetAdCPUReg(CISP);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / WDSIZE) < words)
				set_bit(array,index);
			}
		 if((tempU &= ~OREGISP) == 0)
			return;
		}
	 if(tempU & OREGPC)			/* Register PC?	*/
		{an_id = GetAdCPUReg(CISP);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / WDSIZE) < words)
				set_bit(array,index);
			}
		 if((tempU &= ~OREGPC) == 0)
			return;
		}
	 if(tempU & OREGMFPR)			/* Register MFPR?	*/
		{an_id = GetAdStatCont(MFPR);	/* Get AN_Id of register. */
		 if(IsAdAddrIndex(an_id))	/* See if has address index. */
			{index = GetAdAddrIndex(an_id);/* If so, get it. */
			 if((index / WDSIZE) < words)
				set_bit(array,index);
			}
		 if((tempU &= ~OREGMFPR) == 0)
			return;
		}
	} /* END OF if(tempU) */

 return;
}


	boolean
IsTxSets(tn_id,an_id)		/* TRUE if instruction in textnode sets */
				/* the address. */
register TN_Id tn_id;		/* TN_Id of the instruction.	*/
AN_Id an_id;			/* AN_Id of address. */

{extern boolean IsOpSets();	/* TRUE if opcode sets the address. */
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 unsigned int dst;		/* Destination operands.	*/
 struct opent *op;		/* Operation-code table pointer.	*/
 register unsigned int operand;	/* Operand counter for instruction.	*/
 extern struct opent optab[];	/* The operation-code table.	*/
 register OperandType type;	/* Type of an operand.	*/

 if(IsOpSets(GetTxOpCodeX(tn_id),an_id))	/* Does opcode set it? */
	return(TRUE);				

 if(IsTxAux(tn_id))				/* If auxiliary node,	*/
	return(FALSE);				/* that's all there is.	*/

 op = &optab[GetTxOpCodeX(tn_id)];		/* Pointer to op-code entry. */
 dst = op->odstops;

 for(operand = 0; operand < Max_Ops; operand++)	/* Do each operand.	*/
	{type = GetTxOperandType(tn_id,operand);	/* Get operand's type.*/
	 if(type == Tnone)			/* If Tnone, we are done */
		break;				/* with this instruction. */
	 if(dst & 1)				/* If operand is a destination*/
		{if(an_id == GetTxOperandAd(tn_id,operand)) 
			return(TRUE);		/* Found a match. */
		 if((type == Tdblext) &&	/* If more than one,	*/
				IsAdCPUReg(an_id))
			if(an_id == GetAdCPUReg(GetNextRegId(GetNextRegId(
					GetAdRegA(an_id)))))
				return(TRUE);	/* Found a match. */
		 if(((type == Tdouble) || (type == Tdblext)) &&
				IsAdCPUReg(an_id))
			if(an_id == GetAdCPUReg(GetNextRegId(
					GetAdRegA(an_id))))
				return(TRUE);
		} /* END OF if(dst & 1) */
	 dst >>= 1;
	} /* END OF for(operand = 0; operand < Max_Ops; operand++) */

 return(FALSE);
}

#ifdef LINT
	boolean
IsTxUses(tn_id,an_id)		/* TRUE if instruction uses address */
TN_Id tn_id;			/* TN_Id of the instruction.	*/
AN_Id an_id;			/* AN_Id of address. */

{extern FN_Id FuncId;		/* Id of current Function. */
 extern boolean IsOpUses();	/* TRUE is opcode uses address.*/
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 STATIC AN_Id UsEs();		/* Returns AN_Id of address used.	*/
 extern enum CC_Mode ccmode;	/* cc -X? */
 unsigned int dst;		/* Destination word from opcode table. */
 unsigned int index;		/* Used address index. */
 register unsigned int operand;	/* Operand counter for instruction.	*/
 struct opent *op;		/* Operation-code table pointer.	*/
 unsigned int opcode;		/* Operation-code index. */
 extern struct opent optab[];	/* The operation-code table.	*/
 unsigned int src;		/* Source word from opcode table. */
 AN_Id srch_an_id;		/* Search address node id. */
 OperandType type;		/* Type of an operand.	*/
 AN_Id used_an_id;		/* An AN_Id used by an_id.	*/

 opcode = GetTxOpCodeX(tn_id);
 if(IsOpUses(opcode,an_id))	/* Does opcode use addr? */
	return(TRUE);

 if(IsTxAux(tn_id))				/* If auxiliary node,	*/
	return(TRUE);				/* that's all there is.	*/

 if(IsTxBlackBox(tn_id))			/* Black box uses everything.*/
	return(TRUE);

 op = &optab[opcode];				/* Pointer to op-code entry. */
 src = op->osrcops;
 dst = op->odstops;

 for(operand = 0; operand < Max_Ops; operand++)	/* Do each operand.	*/
	{type = GetTxOperandType(tn_id,operand); /* Get operand's type.*/
	 if(type == Tnone)			/* If Tnone, we are done */
		break;				/* with this instruction. */
	 srch_an_id = GetTxOperandAd(tn_id,operand); 
						/* Get operand's AN_Id.	*/
	 if((src & 1) | (dst & 1))		/* If operand is a source */
						/* or destination,	*/
		{index = 0;
		 while((used_an_id = UsEs(srch_an_id,index++)) != NULL) 
						/*Get used adresses.*/
			if(used_an_id == an_id)
				return(TRUE);	/* Found match. */
		} /* END OF if((src & 1) | (dst & 1)) */

	 if(src & 1)				/* If operand is source, */
		{if(srch_an_id == an_id)
			return(TRUE);		/* Found match. */
		 if((type == Tdblext) &&
				IsAdCPUReg(an_id))
			{used_an_id = GetAdCPUReg(GetNextRegId(GetNextRegId(
					GetAdRegA(an_id))));
			 if(used_an_id == an_id)
				return(TRUE);	/* Found match. */
			}
		 if(((type == Tdouble) || (type == Tdblext)) &&
				IsAdCPUReg(an_id))
			{used_an_id =
				GetAdCPUReg(GetNextRegId(GetAdRegA(an_id)));
			 if(used_an_id == an_id)
				return(TRUE);	/* Found a match. */
			} /* END OF if((type == Tdouble) &&IsAAdCPUReg(an_id))*/
		} /* END OF if(src & 1) */
	 dst >>= 1;
	 src >>= 1;
	} /* END OF for(operand = 0; operand < Max_Ops; operand++) */

	if(IsFnSetjmp(FuncId) && IsTxAnyCall(tn_id))
		{
					/* If this func calls setjmp, */
					/* and in transition mode,    */
					/* then uses stack variables. */
		 if((ccmode == Transition && IsAdNAQ(an_id) && !IsAdCPUReg(an_id))
				|| IsAdSV(an_id))
			return(TRUE);
		}

 return(FALSE);
}
#endif
	boolean
IsOpSets(OpIndex,an_id)		/* TRUE if the indicated opcode sets the */
				/* indicated address. */
unsigned short int OpIndex;	/* Opcode index of operation code. */
AN_Id an_id;			/* Address node id of address. */

{register unsigned int tempU;	/* Temporary opcode table entry word. */
 struct opent *op;		/* Pointer to opcode table entry. */

 op = &optab[OpIndex];
 tempU = op->osetccs;		/* Check for condition codes set. */
 if(tempU && IsAdStatCont(an_id))
	{switch(GetAdRegA(an_id))
		{case CCODE_A: if(tempU & OCCMAUASR) return(TRUE); endcase;
		 case CCODE_C: if(tempU & OCCCPUC) return(TRUE); endcase;
		 case CCODE_N: if(tempU & OCCCPUN) return(TRUE); endcase;
		 case CCODE_V: if(tempU & OCCCPUC) return(TRUE); endcase;
		 case CCODE_X: if(tempU & OCCCPUX) return(TRUE); endcase;
		 case CCODE_Z: if(tempU & OCCCPUZ) return(TRUE); endcase;
		} /* End of switch(GetAdRegA(an_id)) */
	 return(FALSE);
	}

 tempU = op->osetregs;		/* Check for registers set. */
 if(tempU && IsAdCPUReg(an_id))
	{switch(GetAdRegA(an_id))
		{case CREG0:	if(tempU & OREG0) return(TRUE); endcase;
		 case CREG1:	if(tempU & OREG1) return(TRUE); endcase;
		 case CREG2:	if(tempU & OREG2) return(TRUE); endcase;
		 case CREG3:
		 case CREG4:
		 case CREG5:
		 case CREG6:
		 case CREG7:
		 case CREG8:	if(tempU & OREG3_8) return(TRUE); endcase;
		 case CFP:	if(tempU & OREGFP) return(TRUE); endcase;
		 case CAP:	if(tempU & OREGAP) return(TRUE); endcase;
		 case CPSW:	if(tempU & OREGPSW) return(TRUE); endcase;
		 case CSP:	if(tempU & OREGSP) return(TRUE); endcase;
		 case CPCBP:	if(tempU & OREGPCBP) return(TRUE); endcase;
		 case CISP:	if(tempU & OREGISP) return(TRUE); endcase;
		 case CPC:	if(tempU & OREGPC) return(TRUE); endcase;
		 case MFPR:	if(tempU & OREGMFPR) return(TRUE); endcase;
		} /* End of switch(GetAdRegA(an_id)) */
	}
 return(FALSE);
}
	boolean
IsOpUses(OpIndex,an_id)		/* TRUE if the indicated opcode uses the */
				/* indicated address. */
unsigned short int OpIndex;	/* Opcode index of operation code. */
AN_Id an_id;			/* Address node id of address. */

{register unsigned int tempU;	/* Temporary opcode table entry word. */
 struct opent *op;		/* Pointer to opcode table entry. */

 op = &optab[OpIndex];
 tempU = op->ouseccs;		/* Check for condition codes set. */
 if(tempU && IsAdStatCont(an_id))
	{switch(GetAdRegA(an_id))
		{case CCODE_A: if(tempU & OCCMAUASR) return(TRUE); endcase;
		 case CCODE_C: if(tempU & OCCCPUC) return(TRUE); endcase;
		 case CCODE_N: if(tempU & OCCCPUN) return(TRUE); endcase;
		 case CCODE_V: if(tempU & OCCCPUC) return(TRUE); endcase;
		 case CCODE_X: if(tempU & OCCCPUX) return(TRUE); endcase;
		 case CCODE_Z: if(tempU & OCCCPUZ) return(TRUE); endcase;
		} /* End of switch(GetAdRegA(an_id)) */
	 return(FALSE);
	}

 tempU = op->ouseregs;		/* Check for registers used. */

#ifdef W32200
 if((tempU & (OREG0|OREG1|OREG2|OREG3_8|OREG16_23|OREG24_31|
		OREGFP|OREGAP|OREGPSW|OREGSP|OREGPCBP|OREGISP|OREGPC|OREGMFPR))
		 && IsAdCPUReg(an_id))
#else
 if((tempU & (OREG0|OREG1|OREG2|OREG3_8|
		OREGFP|OREGAP|OREGPSW|OREGSP|OREGPCBP|OREGISP|OREGPC|OREGMFPR))
		 && IsAdCPUReg(an_id))
#endif
	{switch(GetAdRegA(an_id))
		{case CREG0:	if(tempU & OREG0) return(TRUE); endcase;
		 case CREG1:	if(tempU & OREG1) return(TRUE); endcase;
		 case CREG2:	if(tempU & OREG2) return(TRUE); endcase;
		 case CREG3:
		 case CREG4:
		 case CREG5:
		 case CREG6:
		 case CREG7:
		 case CREG8:	if(tempU & OREG3_8) return(TRUE); endcase;
#ifdef W32200
		 case CREG16:
		 case CREG17:
		 case CREG18:
		 case CREG19:
		 case CREG20:
		 case CREG21:
		 case CREG22:
		 case CREG23:
			if(tempU & OREG16_23)
				return(TRUE);
			endcase;
		 case CREG24:
		 case CREG25:
		 case CREG26:
		 case CREG27:
		 case CREG28:
		 case CREG29:
		 case CREG30:
		 case CREG31:
			if(tempU & OREG24_31)
				return(TRUE);
			endcase;
#endif
		 case CFP:	if(tempU & OREGFP) return(TRUE); endcase;
		 case CAP:	if(tempU & OREGAP) return(TRUE); endcase;
		 case CPSW:	if(tempU & OREGPSW) return(TRUE); endcase;
		 case CSP:	if(tempU & OREGSP) return(TRUE); endcase;
		 case CPCBP:	if(tempU & OREGPCBP) return(TRUE); endcase;
		 case CISP:	if(tempU & OREGISP) return(TRUE); endcase;
		 case CPC:	if(tempU & OREGPC) return(TRUE); endcase;
		 case MFPR:	if(tempU & OREGMFPR) return(TRUE); endcase;
		} /* End of switch(GetAdRegA(an_id)) */
	}
 if(tempU & OREGENAQS)
	{if(IsAdENAQ(an_id))
		return(TRUE);
	}
 if(tempU & OREGSENAQS)
	{if(IsAdSENAQ(an_id))
		return(TRUE);
	}
 if(tempU & OREGSNAQS)
	{if(IsAdSNAQ(an_id))
		return(TRUE);
	}
 return(FALSE);
}
	boolean
setslivecc(tn_id)		/* TRUE if it sets live condition codes. */
TN_Id tn_id;

{extern AN_Id A;		/* AN_Id of A Condition Code.	*/
 extern AN_Id C;		/* AN_Id of C Condition Code.	*/
 extern AN_Id N;		/* AN_Id of N Condition Code.	*/
 extern AN_Id V;		/* AV_Id of V Condition Code.	*/
 extern AN_Id X;		/* AX_Id of X Condition Code.	*/
 extern AN_Id Z;		/* AZ_Id of Z Condition Code.	*/
 extern m32_target_cpu cpu_chip;		/* Type of CPU chip.	*/
 extern m32_target_math math_chip;		/* Type of MATH chip.	*/
 extern void sets();		/* Determines what is set by instruction. */
 register unsigned int mask;	/* Mask for condition code bits.	*/
 unsigned long int Live;	/* Space for Live bits.	*/
 unsigned long int Sets;	/* Space for sets bits.	*/

	/* N.B.: HERE WE ASSUME THAT ALL THE CONDITION CODES WILL GO */
	/*	INTO THE SAME (THE FIRST) WORD.	*/

 mask = 0;
 if((math_chip == we32106) || (math_chip == we32206))
	if(IsAdAddrIndex(A))
		mask |= (1 << GetAdAddrIndex(A));
 if(IsAdAddrIndex(C))
 	mask |= (1 << GetAdAddrIndex(C));
 if(IsAdAddrIndex(N))
 	mask |= (1 << GetAdAddrIndex(N));
 if(IsAdAddrIndex(V))
 	mask |= (1 << GetAdAddrIndex(V));
 if(cpu_chip == we32200)
	if(IsAdAddrIndex(X))
	 	mask |= (1 << GetAdAddrIndex(X));
 if(IsAdAddrIndex(Z))
 	mask |= (1 << GetAdAddrIndex(Z));

 GetTxLive(tn_id,&Live,1);			/*Get first word of live bits.*/
 mask &= Live;					/* Live condition codes. */
 if(mask == 0)					/* (Save time.) */
	return(FALSE);

 sets(tn_id,&Sets,1);				/*Get first word of sets bits.*/
 mask &= Sets;					/* Live CCs that are set. */

 if(mask != 0)
	return(TRUE);
 return(FALSE);
}
