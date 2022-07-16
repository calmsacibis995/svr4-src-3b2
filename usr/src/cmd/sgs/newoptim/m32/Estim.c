/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/Estim.c	1.7"

#include	<stdio.h>
#include	"defs.h"
#include	"debug.h"
#include	"ANodeTypes.h"
#include	"OpTabTypes.h"
#include	"OpTabDefs.h"
#include	"RegId.h"
#include	"RegIdDefs.h"
#include	"ANodeDefs.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"RoundModes.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"optab.h"
#include	"Target.h"
#include	<values.h>

#define	SOURCE	2
#define	DESTINATION	3
#define	MAXWEIGHT	(MAXINT - 1000)
						/* ARG_COST is the number */
						/* of cycles it takes to */
						/* load an argument into a */
						/* register.	*/
#define	ARG_COST	9
						/* CALL_COST is the number */
						/* of cycles it takes to */
						/* unload a register and */
						/* load it back.	*/
#define	CALL_COST	18
						/* EXT_COST is the number */
						/* of cycles it takes to */
						/* load an external into a */
						/* register and put it back */
						/* into its external location. */
#define	EXT_COST	18
						/* PAYOFF is the difference */
						/* in cycles between memory */
						/* and register reference. */
#define	PAYOFF		3
						/* POINTER_PAYOFF is the */
						/* difference in cycles */
						/* between using a pointer */
						/* in memory and a pointer */
						/* in a register.	*/
#define	POINTER_PAYOFF	6
						/* WEIGHT is the assumed */
						/* number of times through */
						/* a loop.	*/
#define	WEIGHT		8			/* To optimize multiplies and
				  		 divides, make it a power of 2. */
						/* MAXLDEPTH is the maximum loop*/
						/* depth for weight adjustments.*/
						/* (log MAXINT base WEIGHT).    */
#define MAXLDEPTH	10

struct alist {
	AN_Id a;
	struct alist *next;
	};
static struct alist *head;

	/* Private functions. */
STATIC void putalist();
	void
Estim()				/* Calculate address node estimators.	*/

{extern void Free();		/* free() (3X)	*/
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 extern void addraudit();	/* Audits address nodes; in ANode.c.	*/
 register AN_Id an_id;		/* Address node identifier.	*/
 AN_Id an_id2;			/* Address node identifier of second operand. */
 unsigned long int call_cost;	/* Weight for call overhead.	*/
 int depth;			/* #LOOP depth.	*/
 long int new_estim;
 register unsigned int operand;	/* Operand Counter.	*/
 extern int optmode;		/* Size, Default, or Speed mode.	*/
 register struct alist *p;
 struct alist *pp;
 register int payoff;
 RegId reg_id;			/* Register Id of destinations.	*/
 register TN_Id tn_id;		/* Text node identifier.	*/
 LoopType type;			/* Type of loop node.	*/
 long weight;			/* Weight of address use.	*/

 if(DBdebug(0,XTB_AD))				/* If debugging addresses, */
	addraudit("Entering Estim");		/* audit address nodes.	*/

						/* INITIALIZE.	*/
 for(an_id = GetAdNextNode((AN_Id) NULL);	/* Set all estimators.	*/
		an_id;
		an_id = GetAdNextNode(an_id))
	{if(IsAdArg(an_id))			/* If an argument,	*/
		PutAdEstim(an_id,-ARG_COST);	/* charge cost of moving */
						/* it to a register.	*/
	 else
		switch(GetAdGnaqType(an_id))
			{case ENAQ:
			 case SENAQ:
			 case SNAQ:
						/* If external,statext or statloc */
						/* charge cost of loading & */
						/* unloading it.	*/
				PutAdEstim(an_id,-EXT_COST);
				putalist(an_id);
				endcase;
			 default:
						/* Others.	*/
				PutAdEstim(an_id,0);
				endcase;
			}
	}

 call_cost = 0;					/* No calls encountered. */
 weight = 1;					/* Set initial payoff.	*/
 depth = 0;

						/* CALCULATE ESTIMATES.	*/
 for(tn_id = GetTxNextNode((TN_Id) NULL);	/* Scan all the text nodes. */
		tn_id;
		tn_id = GetTxNextNode(tn_id))
	{if(GetTxOpCodeX(tn_id) == LOOP)	/* Adjust weight for loops. */
		{if(optmode == OSIZE)		/* If optimizing for size, */
			continue;		/* don't weigh loops.	*/
		 type = GetTxLoopType(tn_id);	/* Get loop type.	*/
		 if(type == Header)		/* Increase weight for	*/
						/* loop entry.	*/
			{++depth;
			 if(depth <= MAXLDEPTH)
				weight *= WEIGHT;
			}	
		 else if(type == End)		/* Decrease weight for	*/
						/* loop exit.	*/
			{if(depth <= MAXLDEPTH)
				weight /= WEIGHT;
			 --depth;
			}
		 continue;			/* (LOOP nodes have no	*/
		}				/* interesting operands.*/
	 else if(IsTxAnyCall(tn_id))		/* CALL processing, accumulate */
						/* cost of save/restore around */
						/* calls.	*/
		{if((MAXWEIGHT - call_cost) < (weight * CALL_COST))
			call_cost = MAXWEIGHT;
		 else
			call_cost += (weight * CALL_COST);
		} /* END OF if(IsTxAnyCall(tn_id)) */

	 for(ALLOP(tn_id,operand))
		{an_id = GetTxOperandAd(tn_id,operand);
				/* The next few lines adjust the weight	*/
				/* for this operand if it appears to be	*/
				/* a pointer as evidenced by being the	*/
				/* source of a move to a scratch register. */
		 if((GetTxOpCodeX(tn_id) == G_MOV) &&
				(operand == SOURCE) &&
				IsAdCPUReg((an_id2 = 
					GetTxOperandAd(tn_id,DESTINATION))) &&
				(((reg_id = GetAdRegA(an_id2)) == CREG0) ||
					(reg_id == CREG1) ||
					(reg_id == CREG2)))
			payoff = POINTER_PAYOFF;
		 else
			payoff = PAYOFF;
		 new_estim = GetAdEstim(an_id);
		 if(((MAXWEIGHT - new_estim) / weight) < payoff)
			new_estim = MAXWEIGHT;
		 else
			new_estim += (weight * payoff);
		 PutAdEstim(an_id,new_estim);
		} /* END OF for(ALLOP(tn_id,operand)) */
	} /* END OF for(tn_id = GetTxNextNode((TN_Id) NULL; tn_id; ...) */

					/* ADJUST GLOBALS FOR CALL COST.*/
 if(call_cost != 0)
 	for(p = head; p != NULL; )
		{pp = p->next;
		 new_estim = GetAdEstim(p->a);	/* Get previous estimator. */
		 new_estim -= call_cost;
		 PutAdEstim(p->a,new_estim);
		 Free(p);
		 p = pp;
		} 
 head = NULL;

 return;
}
	void
SetRegEstim(modes)		/* Set "register" estimates.	*/
char *modes;			/* 'C' for condition codes, and */
				/* 'M' for math registers.	*/
{extern AN_Id A;		/* AN_Id of A Condition Code.	*/
 extern AN_Id C;		/* AN_Id of C Condition Code.	*/
 extern AN_Id N;		/* AN_Id of N Condition Code.	*/
 extern AN_Id V;		/* AN_Id of V Condition Code.	*/
 extern AN_Id Z;		/* AN_Id of Z Condition Code.	*/
 boolean CCregs;		/* TRUE if get estims for CCs.	*/
 boolean MAUregs;		/* TRUE if get estims for MAU regs.*/
 register AN_Id an_id;
 register long int counter;
 RegId regid;			/* Register identifier.	*/
#ifdef W32200
 extern AN_Id X;		/* AN_Id of X Condition Code.	*/
#endif

 CCregs = MAUregs = FALSE;
 if(modes != NULL){
	for(;*modes != EOS; ++modes)
		switch(*modes){
		case 'C':	CCregs = TRUE; endcase;
		case 'M':	MAUregs = TRUE; endcase;
		}
	}
			/*	WE MUST THINK MORE ABOUT THIS.	*/
 counter = MAXLONG;

 if(CCregs)					/* Condition Codes */
	{PutAdEstim(C,counter--);		/* We want it to go into LD table.*/
	 PutAdEstim(V,counter--);		/* We want it to go into LD table.*/
	 PutAdEstim(Z,counter--);		/* We want it to go into LD table.*/
	 PutAdEstim(N,counter--);		/* We want it to go into LD table.*/
	 if((math_chip == we32106) || (math_chip == we32206))
		PutAdEstim(A,counter--);	/* We want it to go into LD table.*/
#ifdef W32200
	 if(cpu_chip == we32200)		/* Only the WE32200 has an x-bit. */
		PutAdEstim(X,counter--);	/* We want it to go into LD table.*/
#endif
	}

 if(MAUregs)						/* MAU registers. */
	{if((math_chip == we32106) 
			|| (math_chip == we32206))	/* If math chip, */
		{for(regid = MREG0; regid != REG_NONE; regid = GetNextRegId(regid))
			{an_id = GetAdMAUReg(regid);
			 PutAdEstim(an_id,counter--);	/* Want it in LD table. */
			}
		} 
	}

						/* CPU registers. */
 for(regid = CREG0; regid != REG_NONE; regid = GetNextRegId(regid))
	{
	 switch(regid){				/* Skip certain registers. */
	 case CPSW:
	 case CPC:
	 case CFP:
	 case CAP:
	 case CSP:
	 case CPCBP:
	 case CISP:
		(void) GetAdCPUReg(regid);	/* Make address node.	*/
		continue;
	 }
	 an_id = GetAdCPUReg(regid);
	 PutAdEstim(an_id,counter--);		/* Want it in LD table. */
	}

 return;
}
	STATIC void
putalist(an_id)
AN_Id an_id;
{
 struct alist *p;
 extern char *Malloc();
 extern void fatal();

 p = (struct alist *)Malloc(sizeof(struct alist));
 if(p == NULL)
	fatal("putalist: out of space\n");
 p->next = head;
 p->a = an_id;
 head = p;
}
