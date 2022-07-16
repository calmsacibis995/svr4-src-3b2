/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/cse.c	1.4"

#include	<stdio.h>
#include	<string.h>
#include	"defs.h"
#include	"debug.h"
#include	"LoopTypes.h"
#include	"OpTabTypes.h"
#include	"OpTabDefs.h"
#include	"OperndType.h"
#include	"RegId.h"
#include	"RegIdDefs.h"
#include	"RoundModes.h"
#include	"ANodeTypes.h"
#include	"optab.h"
#include	"ANodeDefs.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"optutil.h"
#include	"TNodeDefs.h"
#include	"ALNodeType.h"
#include	"FNodeTypes.h"
#include	"FNodeDefs.h"

#define	NTEMP		96		/* MUST be <= 96 (LIVEDEAD) */
					/* MUST be <= 256 because the */
					/* 'tempid' field of AN_Id is */
					/* an unsigned char.          */
#define	NTWORDS		((NTEMP+WDSIZE-1)/WDSIZE)
#define	NREGS		9		/* %r0-%r2, %r8-%r3 */
#define	SYM_NARGS	(MAX_OPS - 1)
#define ATABSZ		NTEMP
#define STABSZ		NTEMP

#define SaveSet(t,v,N)	PutTxDead(t,v,N)
#define GetSet(t,v,N)	GetTxDead(t,v,N)

#define	MarkTxNode(t)	((t)->mark = 1)
#define	IsTxMarked(t)	((t)->mark == 1)
#define UnmarkTxNode(t)	((t)->mark = 0)

			/* symtab flags */
#define	DELETE		01
#define	IsDeleted(s)	(((s)->flag & DELETE) != 0)
#define	Delete(s)	((s)->flag |= DELETE)

			/* flags for ralloc() */
#define	L_R0	01
#define	L_R1	02
#define	L_R2	04
#define	L_R8	010
#define	L_R7	020
#define	L_R6	040
#define	L_R5	0100
#define	L_R4	0200
#define	L_R3	0400
			/* cost for save & restore an additional register */
#define	SR_DELTA	9

typedef struct operand {
	AN_Id a;
	OperandType t;
	} symoper_t;

typedef struct symtab {
	AN_Id lhs;
	unsigned short op;
	unsigned char flag;
	unsigned char nargs;
	symoper_t arg[SYM_NARGS];
	} SymTab;

typedef struct assigntab {
	AN_Id lhs;
	AN_Id rhs;
	} AssignTab;

typedef struct reg {
	boolean avail;		/* TRUE if available for allocation */
	boolean ref;		/* TRUE if reg has been referenced in the block */
	unsigned int ld;	/* GNAQ index */
	unsigned int lastset;	/* Id of temp created at last set of reg */
	AN_Id an_id;		/* AN_Id of reg */
	unsigned short mbit;	/* Mask bit for allocation */
	} Reg;

typedef struct tmptab {
	AN_Id from;		/* original register */
	AN_Id to;		/* final assigned register */
	unsigned short sz;	/* 1 for single word, 2 = double, 3 = triple */
	unsigned short nallow;	/* TV's not allowed by allocation constraints */
	struct reg *lastset;	/* the lastset of a TV */
	} TempTab;

typedef struct modrec {
	TN_Id t;		/* tn_id of where a mod has occurred */
	unsigned int i;		/* i-th operand modified */
	AN_Id a;		/* old an_id */
	struct modrec *next;	/* next mod record */
	} ModRec;

static ModRec *mhead = NULL;
static SymTab stab[STABSZ];
static SymTab *stabend = &stab[0];
static AssignTab atab[ATABSZ];
static AssignTab *atabend = &atab[0];
static TempTab temptab[NTEMP];
static Reg reg[NREGS];
static Reg *regtop;		/* top of the available regs		*/
static Reg *tracetop;		/* top of the traced vars		*/
static unsigned int nsvregs;
static unsigned int nexttemp = 0;

	/* Private functions. */
STATIC boolean aliasable();	/* Is a nonTV aliasable?		*/
STATIC AssignTab *alookup();	/* Looks up a entry in assigntab	*/
STATIC boolean commutative();	/* Is an operator commutative?		*/
STATIC void cse_init();		/* Does initialization.			*/
STATIC boolean cse_opt();	/* Second pass: remove CSE.		*/
STATIC boolean cse_temp();	/* First pass: temp conversion.		*/
STATIC void exclude();		/* Excludes reg from conflicting temps 	*/
STATIC Reg *findreg();		/* Find a reg pointer given its an_id  	*/
STATIC void fix_lastset();	/* Fix the lastset attributes		*/
STATIC void flushsym();		/* Flushes symtab			*/
STATIC void free_old();		/* Frees mod records			*/
STATIC void freereg();		/* Frees an allocated reg 		*/
STATIC void get_outTV();	/* Determine the set of live TVs on block exit */
STATIC AN_Id getassign();	/* Get an assigntab entry 		*/
STATIC void getld();		/* Third pass: compute l/d vector. 	*/
STATIC AN_Id getnew();		/* Get a new reg for temp		*/
STATIC AN_Id getreg();		/* Get a new reg from the available pool*/
STATIC AN_Id getsvreg();	/* Get a saved reg			*/
STATIC int ibenefit();		/* Computes instruction removal benefit */
STATIC void initTV();		/* Initialize the set of traced variables */
STATIC boolean istraced();	/* TRUE if an_id is a TV		*/
STATIC void makeassign();	/* Make an entry in assigntab		*/
STATIC void makesym();		/* Insert a value tuple into symtab	*/
STATIC AN_Id maketemp();	/* Creat a temp				*/
STATIC void mod_use_r();	/* Modify an operand to use a TV	*/
STATIC boolean mod_use_t1();	/* Modify an operand to use a temp (cse_temp)*/
STATIC void mod_use_t2();	/* Modify an operand to use another temp (cse_opt)*/
STATIC char *mtos();		/* Converts reg mask to string		*/
STATIC boolean ralloc();	/* Fourth pass: register allocation. 	*/
STATIC boolean rassign();	/* Register assignment			*/
STATIC boolean regallow();	/* Is reg allowed by mask		*/
STATIC void restore_old();	/* Undo all mods, restore to old	*/	
STATIC void rewrite();		/* Fifth pass: rewrite. 		*/
STATIC unsigned int ridtom();	/* Converts RegId to reg mask		*/
STATIC void save_old();		/* Make a mod record			*/
STATIC boolean set_constraints();/* setup allocation constraints		*/
STATIC void set_tempdata();	/* Sets initial values for temps	*/
STATIC SymTab *symlookup();	/* Lookup a symtab emtry		*/
STATIC void symprint();		/* Prints a symtab entry (for debugging)*/
STATIC void tempsets();		/* Temp use info			*/
STATIC void tempuses();		/* Temp set info			*/
	void
cse()				/* This function attempts common subexpression	*/
				/* elimination improvements.	*/

{
 extern FN_Id FuncId;		/* Id of current function. */
 TN_Id begin;			/* Beginning of block pointer.	*/
 TN_Id end;			/* End of block pointer.	*/
 extern void fixnumnreg();
 extern void funcprint();	/* Prints a function.	*/
 register TN_Id ip;		/* TN_Id of current instruction.	*/
 int ninstr;
 unsigned int osvregs;

 if(Xskip(XCSE))				/* Should this optimization */
        return;                                 /*  be done? NO. */

 if(IsFnBlackBox(FuncId))			/* If black box in function, */
	return;					/*  give a normal return. */

 if(DBdebug(4,XCSE))				/* Show function before	*/
	funcprint(stdout,"before CSE",0);	/*  we do anything. */

 nsvregs = osvregs = GetFnNumReg(FuncId);
 initTV();
 cse_init();
 begin = end = NULL;
 ninstr = 0;
						/* SCAN THE INSTRUCTIONS. */
 for(ALLNSKIP(ip))
	{if(IsTxLabel(ip)			/* If we hit a label or */
			|| IsTxBr(ip)		/* any branch (alas!), or	*/
			|| IsTxAnyCall(ip))	/* any calls, we've hit the end */
						/* of the basic block.	*/
		{if(begin != NULL)
			{if(cse_opt(begin,end,ninstr))
				rewrite(begin,end);
			 else
				restore_old();
			 cse_init();
			 ninstr = 0;
			 begin = NULL;
			}
		 continue;			/* Not part of basic block. */
		}
	 else if(IsTxAux(ip))			/* Ignore auxiliary nodes	*/
		continue;
	 else					/* Real instrs of the block */
		{if(begin == NULL)		/* First real instr of the block */
			begin = end = ip;
		 else
			end = ip;		/* March along end pointer. */
		 if(cse_temp(ip))		/* temp conversion succeeded */
			++ninstr;
		 else				/* temp conversion failed. */
						/* restore to original and */
						/* continue with the next  */
						/* instr as the first of block */
			{restore_old();
			 cse_init();
			 begin = NULL;
			 ninstr = 0;
			}
		}
	} /* END OF for(ALLNSKIP(ip) */

 if(begin != NULL)				/* Do the last block */
	if(cse_opt(begin, end, ninstr))
		rewrite(begin,end);
	else
		restore_old();

 if(nsvregs != osvregs)
	{fixnumnreg(nsvregs);
	 PutFnNumReg(FuncId,nsvregs);
	}

 if(DBdebug(4,XCSE))				/* Show function after	*/
	funcprint(stdout,"after CSE",0);	/* we do everything. */

 return;					/* Normal return. */
}
	STATIC void
initTV()		/* initialize the set of Traced Variables */
			/* we're tracing scratch CPU registers for now  */
			/* this routine is called once per function */
{
 RegId rid;
 Reg *rp;
 extern void fatal();

 /* initial condition:
  *
  * 	reg[]:	|--------|
  * 		| unused |
  *		| saved  |
  *		|  regs  |
  *		|--------|  <- (regtop, tracetop)
  *		| traced |
  *		|  vars  |
  *		|--------|
  */
 regtop = tracetop = &reg[2];
 rid = CREG0;
 for(rp = &reg[0]; rp <= tracetop; ++rp)
	{rp->an_id = GetAdCPUReg(rid);
	 rp->ld = GetAdAddrIndex(rp->an_id);
	 rp->mbit = ridtom(rid);
	 rid = GetNextRegId(rid);
	}
 if(nsvregs > 6)
	fatal("CSE: initTV: invalid saved register number\n");
 switch(nsvregs)
	{case 6:	rid = CREG2; endcase;
	 case 5:	rid = CREG3; endcase;
	 case 4:	rid = CREG4; endcase;
	 case 3:	rid = CREG5; endcase;
	 case 2:	rid = CREG6; endcase;
	 case 1:	rid = CREG7; endcase;
	 case 0:	rid = CREG8; endcase;
	}
 for(rp = tracetop+1; rid != CREG2; ++rp)
	{rp->an_id = GetAdCPUReg(rid);
	 rp->mbit = ridtom(rid);
	 rid = GetPrevRegId(rid);
	}
}

	STATIC boolean
istraced(an_id)			/* TRUE if a Traced Variable	*/
AN_Id an_id;
{
 if(IsAdCPUReg(an_id))
	switch(GetAdRegA(an_id))
		{case CREG0:
		 case CREG1:
		 case CREG2:
				return TRUE;
		}
 return FALSE;
}


	STATIC void
cse_init()		/* this init routine is called once per basic block */
{
 Reg *rp;

 nexttemp = 0;
 atabend = &atab[0];
 stabend = &stab[0];
 for(rp = &reg[0]; rp <= regtop ; ++rp)
	{rp->avail = TRUE;
	 rp->ref = FALSE;
	}
}
	STATIC boolean
cse_temp(tn_id)		/* convert all TVs into temps */
TN_Id tn_id;

{unsigned short o;
 unsigned int op;
 extern struct opent optab[];
 unsigned src;
 unsigned dst;
 AN_Id an_id;
 OperandType type;
 AN_Id new;
 extern void prafter();
 extern void prbefore();

 if(DBdebug(3,XCSE))
	{prbefore(GetTxPrevNode(tn_id),GetTxNextNode(tn_id),0,
		"CSE: temp conversion");
	}

 o = GetTxOpCodeX(tn_id);
 if(IsOpDstSrc(o))	/* can't deal with instructions that */
			/* don't have pure src or dest operands */
	return(FALSE);
 src = optab[o].osrcops;
 dst = optab[o].odstops;
 for(op = 0; op < Max_Ops; op++)
 {
	an_id = GetTxOperandAd(tn_id,op);
	type = GetTxOperandType(tn_id,op);
	if((src & 1) & (dst & 1))	/* can't deal with operands */
					/* that are both src and dest */
		return(FALSE);
	if(src & 1)
		{if(istraced(an_id))
			{if((new = getassign(an_id)) == NULL)
			 	/* this one is live on entry ! */
				{new = maketemp(an_id,type);
				 if(new == NULL)
					return(FALSE);
				 makeassign(an_id,new);
				}
			 save_old(tn_id,op,an_id);
			 PutTxOperandAd(tn_id,op,new);
			}
		 else 
			{if(!mod_use_t1(tn_id,op))
				return(FALSE);
			}
		}
	if(dst & 1)
		{if(istraced(an_id))
			{new = maketemp(an_id,type);
			 if(new == NULL)
				return(FALSE);
			 makeassign(an_id,new);
			 save_old(tn_id,op,an_id);
			 PutTxOperandAd(tn_id,op,new);
			}
		 else 
			{if(!mod_use_t1(tn_id,op))
				return(FALSE);
			}
		}
	src >>= 1;
	dst >>= 1;
 }

 if(DBdebug(3,XCSE))
	prafter(GetTxPrevNode(tn_id),GetTxNextNode(tn_id),0);

 return(TRUE);
}
	STATIC boolean
cse_opt(begin,end,ninstr)	/* find and mark all redundant computations */
TN_Id begin;
TN_Id end;
int ninstr;
{
 boolean IsVolatile;		/* Is an operand volatile? 		*/
 unsigned int Ntemp;		/* number of words for temp bits 	*/
 TN_Id afterend;		/* the instruction after last one 	*/
 AN_Id an_id;			/* AN_Id of an operand			*/
 int benefit;			/* accounts for benefits of optimization*/
 int cost;			/* accounts for cost of optimization 	*/
 unsigned int dst;		/* bits identifying destination operands*/
 unsigned short i;		/* i-th operand of an instruction 	*/
 AN_Id new;			/* used for operand replacement		*/
 unsigned short op;		/* opcode of an instruction 		*/
 unsigned long outTV[NVECTORS];	/* vector for TVs live on block exit 	*/
 register TN_Id p;		/* traverses all text nodes 		*/
 SymTab *sp;
 unsigned int src;		/* bits identifying source operands 	*/
 AN_Id tempdest;		/* destination that's a temp		*/
 unsigned int ti;
 unsigned int tj;
 extern struct opent optab[];	/* opcode table (in optab.c)		*/
 extern void prafter();
 extern void prbefore();

 if(nexttemp == 0)		/* no temps allocated, not interesting */
	return FALSE;

 if(ninstr < 2)			/* block too small */
	return FALSE;

 /* reset assign table since temp conversion used it.
  */
 atabend = &atab[0];

 /* get the set of TVs live on block exit
  */
 get_outTV(end,outTV);

 /* determine the lastset attribute of temps
  */
 fix_lastset(outTV);

 afterend = GetTxNextNode(end);
 if(DBdebug(3,XCSE))
	prbefore(GetTxPrevNode(begin),afterend,0,"CSE: before instruction removal");

 benefit = 0;
 for(p = begin; p != afterend ; p = GetTxNextNode(p))
	{op = GetTxOpCodeX(p);
	 if(IsOpAux(op))
		continue;
	 src = optab[op].osrcops;
	 dst = optab[op].odstops;
	 tempdest = NULL;
	 IsVolatile = FALSE;
	 for(i = 0; i < Max_Ops; i++)
		{an_id = GetTxOperandAd(p,i);
		 if(src & 1)
			{if(!IsAdSafe(an_id) || IsTxOperandVol(p,i))
				IsVolatile = TRUE;
			 if((new = getassign(an_id)) != NULL)
				{save_old(p,i,an_id);	/* for recovery */
				 PutTxOperandAd(p,i,new);
				}
			 else
				mod_use_t2(p,i);
			}
		 if(dst & 1)
			{if(!IsAdSafe(an_id) || IsTxOperandVol(p,i))
				IsVolatile = TRUE;
			 if(IsAdTemp(an_id))
				tempdest = an_id;
			 else
				{flushsym(an_id);
				 mod_use_t2(p,i);
				}
			}
		 src >>= 1;
		 dst >>= 1;
		}
	 if(tempdest != NULL)
	 	{if((sp = symlookup(p)) != NULL)
			{if(setslivecc(p))	/* if this instr sets a live CC */
				continue;	/* then don't remove it */
			 /* preserve the lastset attribute of temps */
			 ti = GetAdTempIndex(tempdest);
			 tj = GetAdTempIndex(sp->lhs);
			 if(temptab[ti].lastset != NULL) /* %ti is a lastset of %ri */
				{if(temptab[tj].lastset != NULL)
					{/* %tj is a lastset of %rj =>    */
					 /*    can't remove this instr    */
					 /* change it to a G_MOV instead? */
					 /* but can we back out of it?    */
					 /* movemod(p,sp->lhs,tempdest);  */
					 continue;
					}
				 else	/* %tj inherits the lastset of %ti */
					{temptab[tj].lastset = temptab[ti].lastset;
					 temptab[ti].lastset = NULL;
					}
				}
			 makeassign(tempdest,sp->lhs);
			 MarkTxNode(p);
			 benefit += ibenefit(p);
			}
		 else if(!IsVolatile)
			makesym(p);
		}
	}

 if(DBdebug(3,XCSE))
	{printf("%c after instruction removal\n",ComChar);
	 prafter(GetTxPrevNode(begin),afterend,0);
	 if(DBdebug(4,XCSE))
		{AssignTab *ap;
		 extern unsigned int praddr();

		 printf("%cCSE: assign table after instruction removal:\n",ComChar);
		 for(ap = &atab[0]; ap < atabend; ap++)
			{printf("%c ",ComChar);
			 praddr(ap->lhs,Tword,stdout);
			 printf(" assigned ");
			 praddr(ap->rhs,Tword,stdout);
			 putchar(NEWLINE);
			}
		}
	 putchar(NEWLINE);
	}

 if(benefit > 0)		/* there are benefits derived from instr removal */
	{Ntemp = (nexttemp+WDSIZE-1)/WDSIZE;
	 getld(begin,end,Ntemp);
	 cost = 0;
	 if(ralloc(begin,end,Ntemp,outTV,&cost)	/* TV assignment succeeded */
		&& (benefit > cost)		/* and there is perf gain */
	   )
		return(TRUE);
	}
 return(FALSE);
}
	STATIC void
getld(begin,end,Ntemp)
TN_Id begin;
TN_Id end;
unsigned int Ntemp;
{
 unsigned long Set[NTWORDS];
 unsigned long Use[NTWORDS];
 int i;
 unsigned long live[NTWORDS];
 TN_Id p;

 /* compute the live info for temps. */
 /* note that we're overwriting the live info for the GNAQs. */
 for(i=0; i < Ntemp; ++i)
	live[i] = 0;
 begin = GetTxPrevNode(begin);
 for(p = end; p != begin; p = GetTxPrevNode(p))
	{if(IsTxMarked(p))
		continue;
	 PutTxLive(p,live,Ntemp);
	 tempuses(p,Use,Ntemp);
	 tempsets(p,Set,Ntemp);
	 for(i=0; i < Ntemp; ++i)
		{live[i] &= ~Set[i];
		 live[i] |= Use[i];
		}
	 SaveSet(p,Set,Ntemp);	/* save for allocation */
	}

 if(DBdebug(3,XCSE))
	{extern int fprinstx();
	 extern unsigned int praddr();
	 unsigned j;

	 end = GetTxNextNode(end);
	 begin = GetTxNextNode(begin);
	 printf("\n%c after live/dead analysis\n",ComChar);
	 for(p = begin; p != end; p = GetTxNextNode(p))
		{fprinstx(stdout,-2,p);
		 if(IsTxMarked(p))
			{printf(" skipped\n");
			 continue;
			}
		 GetTxLive(p,live,Ntemp);
		 printf(" live:");
		 for(j=0; j < nexttemp; ++j)
			if(get_bit(live,j) != 0)
				{putchar(SPACE);
				 praddr(GetAdTemp(j),Tword,stdout);
				}
		 putchar(NEWLINE);
		}
	}
}
	STATIC boolean
ralloc(begin,end,Ntemp,outTV,costp)	/* allocate a TV to a temp 	*/

TN_Id begin;			/* first instr of the block		*/
TN_Id end;			/* last instr of the block		*/
unsigned int Ntemp;		/* number of words to hold temp bits 	*/
unsigned long outTV[];		/* set of TVs live on exit		*/
int *costp;			/* cost updated by rassign()		*/
{
 TN_Id afterend;		/* instructions past the last one	*/
 TN_Id tn_id;

 /* try to set allocation constraints
  */
 if(!set_constraints(begin,end,Ntemp,outTV))
	return(FALSE);

 /* now try to allocate temps to registers
  */
 afterend = GetTxNextNode(end);
 for(tn_id = begin; tn_id != afterend; tn_id = GetTxNextNode(tn_id))
	{if(IsTxMarked(tn_id))
		continue;
	 if(!rassign(tn_id,Ntemp,costp))	/* if allocation fails, */
		{				/* give up. */
		 if(DBdebug(3,XCSE))
			printf("%c register assignment failed\n",ComChar);
		 return(FALSE);
		}
	}

 if(DBdebug(3,XCSE))
	{unsigned j;

	 printf("%c register assignment table after allocation :\n",ComChar);
	 for(j = 0; j < nexttemp; ++j)
		{extern unsigned int praddr();

		 if(temptab[j].to != NULL)
			{printf("%c ",ComChar);
			 praddr(GetAdTemp(j),Tword,stdout);
			 printf(" is assigned ");
			 praddr(temptab[j].to,Tword,stdout);
			 printf(", old = ");
			 praddr(temptab[j].from,Tword,stdout);
			 printf(", nallow = %s\n",mtos(temptab[j].nallow));
			}
		}
	 putchar(NEWLINE);
	}

 return(TRUE);
}
	STATIC void
rewrite(begin,end)		/* rewrite the instrs after TV allocation */

TN_Id begin;			/* first instr of block		*/
TN_Id end;			/* last instr of block		*/
{
 TN_Id afterend;
 AN_Id an_id;
 unsigned short i;
 unsigned short j;
 unsigned short op;
 unsigned int src;		/* bits identifying source operands*/
 unsigned int dst;		/* bits identifying dest operands */
 int ndel;			/* number of instructions deleted */
 extern struct opent optab[];	/* opcode table			*/
 TN_Id p;
 TN_Id pp;
 extern void fatal();
 extern void prafter();
 extern void prbefore();

 afterend = GetTxNextNode(end);

 if(DBdebug(0,XCSE))
	prbefore(GetTxPrevNode(begin),afterend,0,"CSE: before rewrite");

 free_old();	/* throw away mod records */

 ndel = 0;		
 for(p = begin; p != afterend ; p = GetTxNextNode(p))
	{op = GetTxOpCodeX(p);
	 if(IsOpAux(op))
		continue;
	 if(IsTxMarked(p))
		{pp = GetTxPrevNode(p);
		 DelTxNode(p);
		 p = pp;
		 ndel++;
		 continue;
		}
	 src = optab[op].osrcops;
	 dst = optab[op].odstops;
	 for(i = 0; i < Max_Ops; ++i)
		{if(GetTxOperandType(p,i) == Tnone)
			break;
		 if((src & 1) | (dst & 1))
		 	{an_id = GetTxOperandAd(p,i);
		 	 if(IsAdTemp(an_id))
				{j = GetAdTempIndex(an_id);
				 if(temptab[j].to == NULL)
					fatal("CSE: rewrite: unassigned temp\n");
				 PutTxOperandAd(p,i,temptab[j].to);
				}
			 else
				mod_use_r(p,i);
			}
		 src >>= 1;
		 dst >>= 1;
		}
	}

 if(DBdebug(0,XCSE))
	{if(ndel > 0)
		printf("%c %d instructions deleted\n",ComChar,ndel);
	 prafter(GetTxPrevNode(begin),afterend,0);
	}

}
	STATIC boolean
mod_use_t1(tn_id,op)			/* modify an operand to use a temp, */
					/* called in phase 1 (cse_temp).    */
					/* returns FALSE when out of temps. */
TN_Id tn_id;
unsigned op;

{AN_Id an_id;
 char *expr;
 AN_Mode m;
 AN_Id new;
 AN_Id old;
 unsigned int ti;
 OperandType type;
 extern void fatal();

 old = an_id = GetTxOperandAd(tn_id,op);
 switch(m = GetAdMode(an_id))
	{case DispDef:
		if((an_id = GetAdUsedId(an_id,0)) == NULL)
			fatal("CSE: mod_use_t: bad DispDef address\n");
		/* FALLTHRU */
	 case Disp:
		if((an_id = GetAdUsedId(an_id,0)) == NULL)
			fatal("CSE: mod_use_t1: bad Disp address\n");
		if(!istraced(an_id))
			return(TRUE);
		if((new = getassign(an_id)) == NULL)
			/* this one live on entry */
			{new = maketemp(an_id,Tword);
			 if(new == NULL)
				return(FALSE);	/* out of temps */
			 makeassign(an_id,new);
			}
		else if(!IsAdTemp(new))
			fatal("CSE: mod_use_t1: getassign() returned nonTemp\n");
		ti = GetAdTempIndex(new);
		type = GetTxOperandType(tn_id,op);
		expr = GetAdExpression(type,old);
		if(m == Disp)
			new = GetAdDispTemp(type,expr,ti);
		else
			new = GetAdDispDefTemp(expr,ti);
		endcase;
	 default:
		return(TRUE);
	}
 save_old(tn_id,op,old);
 PutTxOperandAd(tn_id,op,new);
 return(TRUE);
}
	STATIC void
mod_use_t2(tn_id,op)			/* modify an operand to use a different   */
					/* temp, called during phase 2 (cse_opt). */
TN_Id tn_id;
unsigned op;

{AN_Id an_id;
 char *expr;
 AN_Mode m;
 AN_Id new;
 AN_Id old;
 unsigned int ti;
 OperandType type;
 extern void fatal();

 old = an_id = GetTxOperandAd(tn_id,op);
 switch(m = GetAdMode(an_id))
	{case DispDef:
		if((an_id = GetAdUsedId(an_id,0)) == NULL)
			fatal("CSE: mod_use_t2: bad DispDef address\n");
		/* FALLTHRU */
	 case Disp:
		if((an_id = GetAdUsedId(an_id,0)) == NULL)
			fatal("CSE: mod_use_t2: bad Disp address\n");
		if(!IsAdTemp(an_id))
			return;
		if((new = getassign(an_id)) == NULL)
			return;
		else if(!IsAdTemp(new))
			fatal("CSE: mod_use_t: getassign() returned nonTemp\n");
		ti = GetAdTempIndex(new);
		type = GetTxOperandType(tn_id,op);
		expr = GetAdExpression(type,old);
		if(m == Disp)
			new = GetAdDispTemp(type,expr,ti);
		else
			new = GetAdDispDefTemp(expr,ti);
		endcase;
	 default:
		return;
	}
 save_old(tn_id,op,old);
 PutTxOperandAd(tn_id,op,new);
}
	STATIC boolean
rassign(tn_id,Ntemp,costp)		/* assign a TV to a temp */
TN_Id tn_id;
unsigned Ntemp;
int *costp;				/* where to update cost		*/
{
 unsigned long set[NTWORDS];		/* set of temps set by this instr  */
 unsigned long use[NTWORDS];		/* set of temps used by this instr  */
 unsigned long live[NTWORDS];		/* set of temps live after this instr */
 AN_Id new;
 unsigned i;
 Reg *rp;
 extern void fatal();

 tempuses(tn_id,use,Ntemp);
 GetSet(tn_id,set,Ntemp);
 GetTxLive(tn_id,live,Ntemp);

 /* Here, we make use of the following fact:
  *	if an instruction uses & sets temps, e.g.
  *		...tj... -> ti
  * 	then j <= i must always be true.
  * This assumption helps to optimize register usage.
  */
 for(i = 0; i < nexttemp; ++i)
	{if(get_bit(use,i) != 0)
		{if(temptab[i].to == NULL)
			fatal("CSE: rassign: unassigned live-on-entry temp\n");
		 if(get_bit(live,i) == 0)
			freereg(temptab[i].to);
		}
	 if(get_bit(set,i) != 0)
		{if((rp = temptab[i].lastset) != NULL)	/* a lastset, assign to designated reg */
			{if(!rp->avail)
				fatal("CSE: rassign: lastset already assigned\n");
			 rp->avail = FALSE;
			 temptab[i].to = rp->an_id;
			}
		 else if(temptab[i].to == NULL)	/* not yet assigned */
			{if((new = getnew(i,costp)) == NULL)
				return FALSE;
			 temptab[i].to = new;
			}
		 if(get_bit(live,i) == 0)
			freereg(temptab[i].to);
		}
	}
 return TRUE;
}
	STATIC void
makeassign(lhs, rhs)		/* enter "lhs = rhs" into assigntab */
AN_Id lhs;
AN_Id rhs;
{
 AssignTab *ap;
 extern void fatal();

 if((ap = alookup(lhs)) == NULL)
	{if(atabend >= &atab[ATABSZ])
		fatal("makeassign: out of table space\n");
	 atabend->lhs = lhs;
	 atabend->rhs = rhs;
	 ++atabend;
	}
 else
	ap->rhs = rhs;
}

	STATIC AN_Id
getassign(an_id)
AN_Id an_id;
{
 AssignTab *ap;

 if((ap = alookup(an_id)) == NULL)
	return(NULL);
 return(ap->rhs);
}

	STATIC AssignTab *
alookup(lhs)
AN_Id lhs;
{
 register AssignTab *ap;

 for(ap = &atab[0]; ap < atabend; ++ap)
	if(ap->lhs == lhs)
		return ap;
 return NULL;
}
	STATIC SymTab *
symlookup(tn_id)		/* Find an entry (lhs,v) in symtab, where lhs 	*/
				/* is the dest of the instr, and v is a value	*/
				/* tuple = (op,s1,...,sn): op = opcode and 	*/
				/* si is the i-th source operand of the instr.	*/
				/* Note that we need to check operand types   	*/
				/* in order to ensure mixed type correctness.	*/
TN_Id tn_id;
{
 int i,j;
 boolean match;
 unsigned short op;
 extern struct opent optab[];
 symoper_t srcops[3];
 unsigned int src;
 register SymTab *sp;

 /* Normalize source operands */
 op = GetTxOpCodeX(tn_id);
 src = optab[op].osrcops;
 j = 0;
 for(i = 0; i < Max_Ops; ++i)
	{if(src & 1)
		{srcops[j].a = GetTxOperandAd(tn_id,i);
		 srcops[j].t = GetTxOperandType(tn_id,i);
		 ++j;
		}
	 src >>= 1;
	}

 for(sp = &stab[0]; sp < stabend; sp++)
	{if(sp->op == op && !IsDeleted(sp))
		{/* try for operand match */
		 match = TRUE;
		 for(i = 0; i < j; i++)
			if(srcops[i].a != sp->arg[i].a
					|| srcops[i].t != sp->arg[i].t)
				{match = FALSE;
				 break;
				}
		 /* one more try for commutative operators */
		 if(!match && commutative(op)
			 && (srcops[1].a == sp->arg[0].a)
			 && (srcops[0].a == sp->arg[1].a)
			 && (srcops[1].t == sp->arg[0].t)
			 && (srcops[0].t == sp->arg[1].t)
		   )
			match = TRUE;
		 if(match)
			return(sp);
		}
	}
 return(NULL);
}
	STATIC boolean
commutative(op)
unsigned op;
{
 switch(op)
	{case G_ADD3:
	 case G_AND3:
	 case G_MUL3:
	 case G_OR3:
	 case G_XOR3:
		return TRUE;
	}
 return FALSE;
}
	STATIC void
makesym(tn_id)			/* Enter an entry (lhs,v) in symtab, where lhs 	*/
				/* is the dest of the instr, and v is a value	*/
				/* tuple = (op,s1,...,sn): op = opcode and 	*/
				/* si is the i-th source operand of the instr.	*/
				/* Note that we need to enter operand types   	*/
				/* in order to ensure mixed type correctness.	*/
TN_Id tn_id;
{
 AN_Id an_id;
 int i,j;
 unsigned short op;
 OperandType type;
 extern struct opent optab[];
 unsigned int src;
 unsigned int dst;
 register SymTab *sp;
 extern void fatal();

 if((sp = stabend) >= &stab[STABSZ])
	fatal("makesym: out of symbol table space\n");
 op = GetTxOpCodeX(tn_id);
 sp->op = op;
 sp->flag = 0;
 src = optab[op].osrcops;
 dst = optab[op].odstops;
 j = 0;
 for(i = 0; i < Max_Ops; ++i)
	{an_id = GetTxOperandAd(tn_id,i);
	 type = GetTxOperandType(tn_id,i);
	 if(src & 1)
		{sp->arg[j].a = an_id;
		 sp->arg[j].t = type;
		 ++j;
		}
	 if(dst & 1)
		sp->lhs = an_id;
	 src >>= 1;
	 dst >>= 1;
	}
 sp->nargs = (unsigned char)j;
 ++stabend;
 
 if(DBdebug(4,XCSE))
	{SymTab *sq;

	 printf("%cCSE: symbol table after makesym:\n", ComChar);
	 for(sq = &stab[0]; sq < stabend; sq++)
		{if(IsDeleted(sq))
			continue;
		 printf("%c ",ComChar);
		 symprint(sq);
		 putchar(NEWLINE);
		}
	}
}
	STATIC void
flushsym(an_id)			/* Delete from symtab all entries that might */
				/* be invalidated by an assignment to an_id. */
AN_Id an_id;
{
 register SymTab *sp;
 unsigned j;

 /* assumptions:
  *  - this routine is called with an an_id that's not a temp
  *  - LHS of every entry in the symbol table is a temp
  * there are two cases:
  * 1) if the assigned-to object is aliasable, then we must remove
  *    all entries with a value tuple containing an aliasable operand.
  * 2) if the assigned-to object is not aliasable (which implies that
  *    it is not a TV since it isn't a temp), then we must remove all
  *    entries with a value tuple containing the object.
  * note that we need not remove any entries that contain a value tuple
  * which depends on a removed entry. this is because an entry in symtab
  * represents value computed into a temp and therefore not aliasable.
  */

 if(aliasable(an_id))
	 for(sp = &stab[0]; sp < stabend; ++sp)	/* for every entry in sym table */
		{if(IsDeleted(sp))
			continue;
		 for(j = 0; j < sp->nargs; ++j)	/* for every arg on RHS */
			{if(aliasable(sp->arg[j].a)
				&& !(j == 0 && sp->op == G_MOVA)) /* not src of MOVA */
				{
				 Delete(sp);
				 if(DBdebug(4,XCSE))
					{printf("%c flushed: ",ComChar);
					 symprint(sp);
					 putchar(NEWLINE);
					}
				 break;
				}
			}
		}
 else
	for(sp = &stab[0]; sp < stabend; ++sp)	/* for every entry in sym table */
		{if(IsDeleted(sp))
			continue;
		 for(j = 0; j < sp->nargs; ++j)	/* for every arg on RHS */
			{if(sp->arg[j].a == an_id)
				{Delete(sp);
				 if(DBdebug(4,XCSE))
					{printf("%c flushed: ",ComChar);
					 symprint(sp);
					 putchar(NEWLINE);
					}
				 break;
				}
			}
		}
}
	STATIC boolean
aliasable(an_id)
AN_Id an_id;
{
 switch(GetAdGnaqType(an_id))
	{case NAQ:
	 case SNAQ:
		return FALSE;
	}
 return TRUE;
}
	STATIC void
tempsets(tn_id,array,words)
TN_Id tn_id;			/* TN_Id of the instruction.	*/
unsigned long int array[];	/* Where to put results.	*/
unsigned int words;		/* Number of result words wanted.	*/

{extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
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
 if(IsOpAux(opx))				/* If auxiliary node,	*/
	return;					/* that's all there is.	*/

 op = &optab[opx];				/* Pointer to op-code entry. */
 dst = op->odstops;

 for(operand = 0; operand < Max_Ops; operand++)	/* Do each operand.	*/
	{type = GetTxOperandType(tn_id,operand);	/* Get operand's type.*/
	 if(type == Tnone)			/* If Tnone, we are done */
		break;				/* with this instruction. */
	 if(dst & 1)				/* If operand is a destination*/
		{an_id = GetTxOperandAd(tn_id,operand);	/* Get operand's Id.*/
		 if(IsAdTemp(an_id))		/* If address index, */
			{vector = GetAdTempIndex(an_id);
			 set_bit(array,vector);
			 if(type == Tdouble)
				set_bit(array,vector+1);
			}
		} 
	 dst >>= 1;
	} 
}
	STATIC void
tempuses(tn_id,array,words)
TN_Id tn_id;
unsigned long int array[];
unsigned int words;

{extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 AN_Id an_id;			/* AN_Id of an operand.	*/
 unsigned int dst;
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

 opcode = GetTxOpCodeX(tn_id);

 if(IsOpAux(opcode))				/* If auxiliary node,	*/
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
		{if(!IsAdTemp(an_id))
			{used_an_id = an_id;
			 while((used_an_id = GetAdUsedId(used_an_id,0)) != NULL) 
				{if(IsAdTemp(used_an_id))
					{vector = GetAdTempIndex(used_an_id);
					 set_bit(array,vector);
					}
				}
			}
		} 
	 if(src & 1)
		{if(IsAdTemp(an_id))
			{vector = GetAdTempIndex(an_id);
			 set_bit(array,vector);
			 if(type == Tdouble)
				set_bit(array,vector+1);
			}
		}
	 dst >>= 1;
	 src >>= 1;
	} /* END OF for(operand = 0; operand < Max_Ops; operand++) */
}
	STATIC int
ibenefit(tn_id)		/* compute the benefit of removing instruction tn_id, */
			/* in terms of cycles saved. note that we should really */
			/* take into account the address modes, but... */
			/*(the cycle counts are gleaned from the assembler manual)*/
TN_Id tn_id;
{
 unsigned short op;

 op = GetTxOpCodeX(tn_id);
 if(IsOpGeneric(op))
	switch(op)
		{case G_MOVA:
		 case G_SUB2:
		 case G_OR2:
		 case G_AND2:
		 case G_XOR2:
		 case G_BIT:
				return 2;
		 case G_ADD2:
				return 3;
		 case G_ADD3:
		 case G_SUB3:
		 case G_OR3:
		 case G_AND3:
		 case G_XOR3:
		 case G_INSF:
				return 4;
		 case G_ALS3:
		 case G_ARS3:
		 case G_LLS3:
		 case G_LRS3:
		 case G_ROT:
				return 5;
		 case G_MUL2:
		 case G_SWAPI:
				return 18;
		 case G_DIV2:
		 case G_MOD2:
				return 19;
		 case G_MUL3:
				return 20;
		 case G_DIV3:
				return 21;
		 case G_MOD3:
				return 23;
		}
 return 1;	/* minimum savings */
}
	STATIC void
get_outTV(end,outTV)		/* compute the set of TVs live on block exit */
TN_Id end;
unsigned long outTV[];
{
 unsigned int NVector;
 unsigned int i;
 extern unsigned int ld_maxword;
 unsigned long int live[NVECTORS];
 Reg *rp;

 NVector = ld_maxword;

 /* initialize outTV to mask of TVs */
 for(i = 0; i < NVector; ++i)
	outTV[i] = 0;
 for(rp = &reg[0]; rp <= tracetop; ++rp)
	set_bit(outTV,rp->ld);

 /* set what's live on block exit */
 GetTxLive(end,live,NVector);
 for(i = 0; i < NVector; ++i)
	outTV[i] &= live[i];
}
	STATIC void
fix_lastset(outTV)		/* set the lastset attribute of temps */
unsigned long outTV[];
{
 Reg *rp;

 /* all referenced TVs will have been set by some temp by now,
  * even those live on entry. (for live-on-entry ones, it is
  * safe to assume that they have been set prior to block entry.)
  * note that temps will only have a lastset attribute for a 
  * TV that's live on block exit.
  */
 for(rp = &reg[0]; rp <= tracetop; ++rp)
	{if(rp->ref && get_bit(outTV,rp->ld) != 0)
		temptab[rp->lastset].lastset = rp;
	}

 if(DBdebug(3,XCSE))
	{unsigned int i;
	 extern unsigned int praddr();

	 printf("%c temp's with lastset attributes:\n",ComChar);
	 for(i = 0; i < nexttemp; ++i)
		if(temptab[i].lastset != NULL)
			{printf("%c ",ComChar);
			 praddr(GetAdTemp(i),Tword,stdout);
			 printf(" is a last set of ");
			 praddr(temptab[i].lastset->an_id,Tword,stdout);
			 putchar(NEWLINE);
			}
	 putchar(NEWLINE);
	}
}	 
	STATIC boolean
set_constraints(begin,end,Ntemp,outTV)
				/* determine constraints for allocation */
TN_Id begin;
TN_Id end;
unsigned int Ntemp;
unsigned long outTV[];		/* set of TVs live on block exit	*/
{
 TN_Id afterend;
 unsigned int i;
 unsigned long int in[NTWORDS];	/* set of temps live on entry		*/
 unsigned long int live[NTWORDS];
 Reg *rp;
 unsigned long int set[NTWORDS];
 unsigned long int temp;
 TN_Id tn_id;
 unsigned long use[NTWORDS];	/* set of temps used by the first instr */

 /* if a TV is live on exit but never referenced in the block,
  * then we cannot allocate it to any temporary.
  */
 for(rp = &reg[0]; rp <= tracetop; ++rp)
	if(!rp->ref && get_bit(outTV,rp->ld) != 0)
		rp->avail = FALSE;

 /* we make sure here that if a temp ti is a lastset of %rn then
  * all tj's whose live range conflicts with ti's must not grab %rn.
  */
 afterend = GetTxNextNode(end);
 for(tn_id = begin; tn_id != afterend; tn_id = GetTxNextNode(tn_id))
	{if(IsTxMarked(tn_id))
		continue;
	 GetTxLive(tn_id,live,Ntemp);
	 GetSet(tn_id,set,Ntemp);
	 temp = 0;
	 for(i = 0; i < Ntemp; ++i)
		temp |= set[i];
	 if(temp == 0)		/* this instruction does not set any temps */
		continue;
	 for(i = 0; i < nexttemp; ++i)
		 if(get_bit(set,i) != 0 && temptab[i].lastset != NULL)
			{/* exclude TV assigned to i from the */
			 /* allowable set of other temps */
			 exclude(i,live);
			}
	}

 /* find all temporaries live on entry, i.e. those live after
  * the first instr (but not set by it) AND used by the first instr.
  */
 tempuses(begin,use,Ntemp);
 GetSet(begin,set,Ntemp);
 GetTxLive(begin,in,Ntemp);
 for(i = 0; i < Ntemp; ++i)
	{in[i] &= ~set[i];
	 in[i] |= use[i];
	}

 /* set input constraints; actually do the assignment here 
  */
 for(i = 0; i < nexttemp; ++i)
	if(get_bit(in,i) != 0)
		{rp = findreg(temptab[i].from);
		 if(regallow(rp,i))
			{rp->avail = FALSE;
			 temptab[i].to = temptab[i].from;
			}
		 else	/* conflicting in/out constraints */
			/* can probably generate a move */
			/* for this one. */
			return FALSE;
		}

 if(DBdebug(3,XCSE))
	{extern unsigned int praddr();

	 for(rp = &reg[0]; rp <= regtop; ++rp)
		if(!rp->avail)
			{printf("%c ",ComChar);
			 praddr(rp->an_id,Tword,stdout);
			 printf(" not available for use\n");
			}
	 for(i = 0; i < nexttemp; ++i)
		if(get_bit(in,i) != 0)
			{printf("%c ",ComChar);
			 praddr(GetAdTemp(i),Tword,stdout);
			 if(temptab[i].to == NULL)
				printf(" is live on input but NOT assigned");
			 else
				{printf(" is live on input, assigned to ");
				 praddr(temptab[i].to,Tword,stdout);
				}
			 putchar(NEWLINE);
			}
	 for(i=0; i < nexttemp; ++i)
		if(temptab[i].nallow != 0)
			{printf("%c ",ComChar);
			 praddr(GetAdTemp(i),Tword,stdout);
			 printf(", nallow = %s\n", mtos(temptab[i].nallow));
			}
	 putchar(NEWLINE);
	}

 return(TRUE);

}
	STATIC void
exclude(tj,live)		/* Exclude TV assigned to tj from all ti's */
				/* either live when tj was set or created  */
				/* after tj.				   */

unsigned tj;			/* temp with a lastset */
unsigned long live[];		/* set of temps live when tj was set */
{
 unsigned int i;
 Reg *rp;
 unsigned int tjlastset;
 extern void fatal();

 if((rp = temptab[tj].lastset) == NULL)
	fatal("CSE: exclude called for temp without a lastset attribute\n"); 
 tjlastset = rp->mbit;
 for(i = 0; i < nexttemp; ++i)
	{if( (get_bit(live,i) != 0)	/* i live when tj is set */
		|| (i > tj)		/* or i created after tj */
	   )
		 temptab[i].nallow |= tjlastset;
	}
}
	STATIC unsigned int
ridtom(rid)			/* return a reg mask given a RegId */
RegId rid;
{
 switch(rid)
	{case CREG0:	return(L_R0);
	 case CREG1:	return(L_R1);
	 case CREG2:	return(L_R2);
	 case CREG8:	return(L_R8);
	 case CREG7:	return(L_R7);
	 case CREG6:	return(L_R6);
	 case CREG5:	return(L_R5);
	 case CREG4:	return(L_R4);
	 case CREG3:	return(L_R3);
	}
 return 0;
}
	STATIC void
mod_use_r(tn_id, op)		/* modify an operand to use a TV instead of a temp */
TN_Id tn_id;
unsigned int op;

{
 AN_Id an_id;
 char *expr;
 AN_Id new;
 AN_Id regnode;
 OperandType type;
 AN_Id u;
 extern void fatal();

 an_id = GetTxOperandAd(tn_id,op);
 if(IsAdCPUReg(an_id))
	return;
 if((u = GetAdUsedId(an_id,0)) == NULL)
	return;
 type = GetTxOperandType(tn_id,op);
 switch(GetAdMode(an_id))
	{case Disp:
		if(!IsAdTemp(u))
			return;
		regnode = temptab[GetAdTempIndex(u)].to;
		if(regnode == NULL)
			fatal("CSE: mode_use_r(D): unassigned temp\n");
		expr = GetAdExpression(type,an_id);
		new = GetAdDisp(type,expr,GetAdRegA(regnode));
		endcase;
	 case DispDef:
		u = GetAdUsedId(u,0);
		if(!IsAdTemp(u))
			return;
		regnode = temptab[GetAdTempIndex(u)].to;
		if(regnode == NULL)
			fatal("CSE: mode_use_r(DD): unassigned temp\n");
		expr = GetAdExpression(type,an_id);
		new = GetAdDispDef(expr,GetAdRegA(regnode));
		endcase;
	 default:
		return;
	}
 PutTxOperandAd(tn_id,op,new);
}
	STATIC void
save_old(tn_id, i, an_id)		/* save modifications in LIFO order.*/
TN_Id tn_id;
unsigned int i;
AN_Id an_id;
{
 ModRec *mp;
 extern char *Malloc();
 extern void fatal();

 if((mp = (ModRec *)Malloc(sizeof(ModRec))) == NULL)
	fatal("CSE: save_old: out of space\n");
 mp->next = mhead;
 mp->t = tn_id;
 mp->i = i;
 mp->a = an_id;
 mhead = mp;
}

	STATIC void
restore_old()
{
 ModRec *mp;
 ModRec *next;
 extern void Free();

 for(mp = mhead; mp != NULL; )
	{next = mp->next;
	 PutTxOperandAd(mp->t,mp->i,mp->a);
	 Free(mp);
	 mp = next;
	}
 mhead = NULL;
}

	STATIC void
free_old()
{
 ModRec *mp;
 ModRec *next;
 extern void Free();

 for(mp = mhead; mp != NULL; )
	{next = mp->next;
	 Free(mp);
	 mp = next;
	}
 mhead = NULL;
}
	STATIC AN_Id
getnew(ti,costp)		/* get an avalable and allowable */
				/* register for temp ti		 */
unsigned int ti;
int *costp;
{
 AN_Id new;

 if((new = getreg(ti)) == NULL)	/* no more available */
	{if((new = getsvreg(ti)) == NULL)	/* no more unused save */
		return NULL;
	 *costp += SR_DELTA;	/* up the cost of save/restore a save reg */
	}
 return new;
}

	STATIC AN_Id
getreg(ti)			/* get an available and allowable */
				/* register from the existing pool */
				/* for temp ti			   */
unsigned int ti;
{
 Reg *rp;

 if(temptab[ti].sz > 1)		/* can't handle doubles and triples yet */
	return NULL;
 for(rp = &reg[0]; rp <= regtop ;++rp)
	if(rp->avail)
		{if(!regallow(rp,ti))
			continue;
		  rp->avail = FALSE;
		  return(rp->an_id);
		}
 return(NULL);
}

	STATIC void
freereg(an_id)			/* free a register for reuse	*/
AN_Id an_id;
{
 Reg *rp;
 extern void fatal();

 rp = findreg(an_id);
 if(rp == NULL)
	fatal("CSE: freereg: can't free\n");
 rp->avail = TRUE;
}


	STATIC Reg *
findreg(an_id)			/* find a register and return its reg[] location */
AN_Id an_id;
{
 Reg *rp;

 for(rp = &reg[0]; rp <= regtop ;rp++)
	if(rp->an_id == an_id)
		return(rp);
 return NULL;
}


	STATIC AN_Id
getsvreg(ti)			/* get another saved register */
unsigned int ti;		/* temp to be assigned */
{
 unsigned int n;

 n = temptab[ti].sz;
 if(nsvregs+n > 6)
	return NULL;
 if(n > 1)			/* can't handle doubles and triples yet */
	return NULL;
 ++nsvregs;
 ++regtop;
 regtop->avail = FALSE;
 return(regtop->an_id);
}

	STATIC boolean
regallow(r,ti)			/* can ti grab reg r? */
Reg *r;
unsigned int ti;		/* temp in question */
{
 unsigned int m;

 m = temptab[ti].nallow;
 if(m != 0 && (r->mbit & m) != 0)
	return FALSE;
 return TRUE;
}
	STATIC AN_Id
maketemp(an_id,type)		/* maketemp creates a temp for a traced var */
AN_Id an_id;
OperandType type;
{
 AN_Id new;
 extern void fatal();

 if(nexttemp >= NTEMP)		/* out of temps ? */
	return(NULL);		/* too bad.       */
 if((new = GetAdTemp(nexttemp)) == NULL)
	fatal("maketemp: can't get temp register\n");
 set_tempdata(an_id,nexttemp);
 switch(type)
	{case Tdblext:
		if(nexttemp >= NTEMP)
			fatal("maketemp: out of temp registers for dblext\n");
		temptab[nexttemp].sz = 3;
		++nexttemp;
		an_id = GetAdCPUReg(GetNextRegId(GetAdRegA(an_id)));
		set_tempdata(an_id,nexttemp);
		/* FALLTHRU */
	 case Tdouble:
		if(nexttemp >= NTEMP)
			fatal("maketemp: out of temp registers for double\n");
		temptab[nexttemp].sz = 2;
		++nexttemp;
		an_id = GetAdCPUReg(GetNextRegId(GetAdRegA(an_id)));
		set_tempdata(an_id,nexttemp);
		endcase;
	}
 ++nexttemp;
 return new;
}

	STATIC void
set_tempdata(an_id,ti)		/* initializes temp upon creation */
AN_Id an_id;
unsigned int ti;
{
 Reg *rp;
 extern void fatal();

 rp = findreg(an_id);
 if(rp == NULL)
	fatal("CSE: set_tempdata called with nonTV\n");
 rp->lastset = ti;
 rp->ref = TRUE;
 temptab[ti].from = an_id;
 temptab[ti].to = NULL;
 temptab[ti].sz = 1;
 temptab[ti].nallow = 0;
 temptab[ti].lastset = NULL;
}
	STATIC char *
mtos(m)				/* return a string given the reg mask 	*/
				/* (used for debugging)			*/
unsigned int m;
{
 static char str[9*3];

 str[0] = '\0';
 if(m & L_R0)
	strcat(str,"R0|");
 if(m & L_R1)
	strcat(str,"R1|");
 if(m & L_R2)
	strcat(str,"R2|");
 if(m & L_R8)
	strcat(str,"R8|");
 if(m & L_R7)
	strcat(str,"R7|");
 if(m & L_R6)
	strcat(str,"R6|");
 if(m & L_R5)
	strcat(str,"R5|");
 if(m & L_R4)
	strcat(str,"R4|");
 if(m & L_R3)
	strcat(str,"R3");
 return str;
}

	STATIC void
symprint(sp)
SymTab *sp;
{
 unsigned int j;
 extern struct opent optab[];
 extern unsigned int praddr();

 printf("LHS=");
 praddr(sp->lhs,Tword,stdout);
 printf(", OP=%s, RHS=",optab[sp->op].oname);
 for(j=0; j < sp->nargs; ++j)
	{if(sp->arg[j].a == NULL)
		printf("0");
	 else
		praddr(sp->arg[j].a,sp->arg[j].t,stdout);
		putchar(SPACE);
	}
}
