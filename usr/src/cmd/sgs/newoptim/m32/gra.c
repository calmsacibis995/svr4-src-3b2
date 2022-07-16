/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/gra.c	1.15"

/* gra.c							*/
/*								*/
/*	Global register allocation routines			*/
/*								*/

#include	<stdio.h>
#include	"defs.h"
#include	"debug.h"
#include	"ANodeTypes.h"
#include	"OpTabTypes.h"
#include	"olddefs.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"RegId.h"
#include	"RegIdDefs.h"
#include	"RoundModes.h"
#include	"ANodeDefs.h"
#include	"OpTabDefs.h"
#include	"TNodeTypes.h"
#include	"TNodeDefs.h"
#include	"ALNodeType.h"
#include	"FNodeTypes.h"
#include	"FNodeDefs.h"
#include	"optab.h"
#include	"optim.h"

/* Note: The code in this file assumes that CREG0 through CREG31 have been */
/* assigned increasing values in the RegId enumeration. 	       	   */

#define CREG_MAX	CREG31	/* Highest CPU register number. */

#define CSCR1ST_MIN	CREG0	/* Lowest 1st set CPU scratch regid. */
#define CSCR1ST_MAX	CREG2	/* Highest 1st set CPU scratch regid. */

#define	CSVD_MIN	CREG3	/* Lowest CPU saved regid. */
#define	CSVD_MAX	CREG8	/* Highest CPU saved regid. */

#define CSCR2ND_MIN	CREG16	/* Lowest 2nd set CPU scratch regid. */
#define CSCR2ND_MAX	CREG23	/* Highest 2nd set CPU scratch regid. */

#define	CSTRUCT_RET	CREG2	/* Structure return regid. */

				/* Delta in cycles to save and restore	*/
				/* one register.	*/
#define	CYCDELTA	5
#define	DESTINATION	3
#define	THIRD_SRC	2
#define	PMA_SOURCE	2

#define ld_clr_bit(array,pld)	(array[(pld)->word] &= ~((pld)->bit_mask))
#define ld_get_bit(array,pld)	(array[(pld)->word] & ((pld)->bit_mask))
#define ld_set_bit(array,pld)	(array[(pld)->word] |= ((pld)->bit_mask))

static unsigned int rvregs;	/* Number of returned registers from function.*/

	/* Private function declarations. */
STATIC boolean fromregs();
STATIC unsigned int get_NVectors();
STATIC void get_active();	/* Computes the active vector. */
STATIC void get_conflicts();	/* Computes conflicts matrix. */
STATIC unsigned int get_ldavail();
STATIC void get_liveset();	/* Computes liveset vector. */
STATIC void get_vardata();	/* Get info on vars from context. */
STATIC struct ldent *get_sub(); /* Compute subset list. */
STATIC struct ldent *get_submove(); /* Compute subset list of vars for insertmove.*/
STATIC struct ldent *get_vars(); /* Computes linked listof variables for GRA.*/
STATIC void gra_bitsprint();	/* Prints variables in a mask. */
STATIC void gra_funcprint();	/* Prints instructions with live and set info.*/
STATIC void gra_ldprint();	/* Prints GRA information. */
STATIC void init_gra();		/* Initializes data areas for GRA. */
STATIC void insertmoves();
STATIC void killmovesrc();
STATIC void naqmerge();		/* Rewrites operands to reflect assignments.*/
STATIC void seq_classes();	/* Sequences merging of classes of vars. */
STATIC struct ldent *seq_scratch(); /* Sequences regs to merge scr regs into.*/
STATIC void seq_vars(); 	/* Sequences regs to merge stk vars into.*/
STATIC boolean toregs();
STATIC void upd_data();		/* Update conflicts matrix and active vector.*/

	void
gra(ldtab)			/* Global register allocation.	*/
struct ldent ldtab[];		/* The Live-Dead Table for GRA.	*/

{extern FN_Id FuncId;		/* Id Of current function. */
 static char *Mbgra = {"gra: before global register allocation."};
 unsigned int NVectors;		/* Size to use for NVECTORS.	*/
 unsigned long int active[NVECTORS]; /* Variables that are active. */
 extern void addrprint();	/* Prints address table.	*/
 unsigned long int argstat[NVECTORS]; /* Variables that are args or loc stats. */
 unsigned long int assigned[NVECTORS]; /* Variables assigned to registers. */
 unsigned long int exts[NVECTORS]; /* Variables that are externals. */
 extern void fixnumnreg();	/* Changes number of registers saved.	*/
 extern void funcprint();	/* Print instructions with alluses & allsets.*/
 STATIC void get_active();	/* Computes the active vector. */
 STATIC void get_conflicts();	/* Computes conflicts matrix. */
 STATIC void get_liveset();	/* Computes liveset vector. */
 STATIC struct ldent *get_sub(); /* Compute subset list. */
 STATIC struct ldent *get_vars(); /* Computes linked listof variables for GRA.*/
 STATIC void gra_ldprint();	/* Prints GRA information. */
 STATIC void init_gra();	/* Initializes data areas for GRA. */
 STATIC void insertmoves();	/* Insert moves for var's assigned to regs. */
 unsigned int ldavail;		/* Lowest live-dead entry not used.	*/
 unsigned long int liveset[NVECTORS]; /* Variables that are live when set. */
 unsigned long int misoper[NVECTORS]; /* Variables that are ops of MIS instr. */
 STATIC void naqmerge();	/* Rewrites operands to reflect assignments.*/
 extern unsigned int rvregs;	/* Number of registers usedfor return value. */
 register boolean scr_conflicts; /* TRUE if scr conflicts need to be computed.*/
 unsigned int n;		/* Counter for number of saved registers. */
 unsigned int newnreg;		/* Number of registers saved. */
 unsigned long int ones[NVECTORS];	/* Bit vector of all ones. */
 register struct ldent *pa;	/* Ld entry of variable to be merged. */
 register struct ldent *pvar;	/* Pointer for scanning variable list. */
 struct ldent *regptrs[((int ) CREG_MAX) + 1]; /* Ld entries for registers. */
 STATIC void seq_classes();	/* Sequences merging of variable classes. */
 STATIC struct ldent *seq_scratch(); /* Sequences regs to merge scr regs into.*/
 struct ldent *sub_head;	/* Head pointer for subset list. */
 struct ldent *var_head;	/* Head pointer for variable list. */

 if(DBdebug(3,XGRA))				/* Print address table	*/
	addrprint(Mbgra);			/* If wanted.	*/
 if(DBdebug(1,XGRA))				/* Print function before, */
	funcprint(stdout,Mbgra,2);		/* if wanted.	*/

						/*Establish number of ret.reg.*/
 rvregs = GetFnRVRegs(FuncId);			
						/* Get lowest unused ld entry.*/
 ldavail = get_ldavail();
						/* Get no. words per vector.*/
 NVectors = get_NVectors(ldavail);
						/* Initialize data areas. */
 init_gra(ldavail,NVectors,ldtab,argstat,exts,liveset,active,misoper,
	ones,assigned,regptrs);

 if(Xskip(XGRA))                                /* Is this optimization wanted*/
        return;                                 /* Skip it.  */
 if(IsFnBlackBox(FuncId)) 			/* Check for black boxes, */
	return;					/* Don't do GRA if present. */

						/* Get liveset vector and get */
						/* sets data in text nodes.*/
 get_liveset(ldtab,ldavail,NVectors,argstat,exts,regptrs,liveset);
						/* Get variable list of  */
						/* registers and stack vars */
						/* and MIS operand vector. */
 var_head = get_vars(ldtab,ldavail,NVectors,regptrs,liveset,misoper);
						/* Anything to assign? */
 if(get_sub(var_head,VC_ISTK|
		     VC_STAT|
	   	     VC_STATEXT|
		     VC_EXTDEF,ones) == NULL	
						/* Any integer stack vars */
						/* or externals? */
 		&& (GetFnNumReg(FuncId) == 0))	/* Any saved registers used? */
	return;					/* No. return. */

 						/* Merge 1st set */
						/* CPU scratch registers. */
 scr_conflicts = TRUE;
 for(pa = var_head; pa; pa = pa->var_next)
	{if((pa->var_class & VC_CSCR1ST) == 0)	/* Only consider 1st set CPU */
		continue;			/* registers. */
	 					/* Only consider if liveset. */
	 if(ld_get_bit(liveset,pa) != 0 && scr_conflicts)	
		{				/* Subset of 1st set */
						/* scratch regs.*/
		 sub_head = get_sub(var_head,VC_CSCR1ST,ones);
		 				/* Compute conflicts. */
		 get_conflicts(sub_head,NVectors,argstat,exts,liveset);
		 scr_conflicts = FALSE;
		}
						/* Sequence merges. */
	 if(seq_scratch(pa,regptrs,liveset,assigned) != NULL)
		{				/* Subset of merged register. */
		 sub_head = pa;
		 pa->sub_next = NULL;
						/* Rewrite operands using the */
						/* scratch register. */
		 naqmerge(sub_head,NVectors,regptrs);
						/* Get liveset vector and get */
						/* sets data in text nodes. */
 		 get_liveset(ldtab,ldavail,NVectors,argstat,exts,regptrs,liveset);
		 scr_conflicts = TRUE;
		}
	}
 						/* Compute active variables. */
 get_active(var_head,NVectors,liveset,active);	
 						/* Subset of active variables.*/
 sub_head = get_sub(var_head,VC_ALL,active);

 get_conflicts(sub_head,NVectors,argstat,exts,liveset); /* Compute conflicts. */

						/* Merge active saved */
						/* registers, 2nd set */
						/* scratch registers, */
						/* and stack vars and */
						/* update data. */
 seq_classes(var_head,NVectors,active,assigned,ones);
						/* Subset of assigned */
						/* saved registers, */
						/* 2nd set scratch registers, */
						/* stack variables, and */
						/* externals. */
 sub_head = get_sub(var_head,VC_CSVD|
			     VC_CSCR2ND|
			     VC_ISTK|
			     VC_STAT|
			     VC_STATEXT|
			     VC_EXTDEF,assigned);
 if(sub_head == NULL)				/* Anything assigned? */
	return;
 naqmerge(sub_head,NVectors,regptrs);		/* Rewrite operands to */
						/* reflect the merges. */
 insertmoves(var_head,ldavail,NVectors);	/* Insert moves to load	*/
						/* and restore registers*/
 newnreg = 0;					/* Count regs to be saved.*/
 n = 0;
 for(pvar = var_head; pvar; pvar = pvar->var_next)
	{if((pvar->var_class & VC_CSVD) == 0)
		continue;
	 n++;
	 if(ld_get_bit(active,pvar))
		newnreg = n;
	}
 if(newnreg != GetFnNumReg(FuncId)) 
	{fixnumnreg(newnreg);			/* Fix no of saved regs. */
 	 PutFnNumReg(FuncId,newnreg);
	}

 if(DBdebug(3,XGRA))
	gra_ldprint("after global register allocation",ldavail,NVectors,var_head,
			misoper,liveset,active,assigned);
 if(DBdebug(2,XGRA))				/* Print function after, */
	funcprint(stdout,"gra: after global register allocation",2);
 return;
}
	STATIC unsigned int
get_ldavail()			/* Get ldavail, lowest unused ldtab entry. */

{AN_Id an_id;			/* AN_Id of last GNAQ list entry. */
 extern void fatal();		/* Handles fatal errors. */
 unsigned int ldavail;		/* Lowest unused live-dead table entry. */

 an_id = GetAdPrevGNode((AN_Id) NULL);		/* Find highest address index.*/
 if(an_id == (AN_Id) NULL)			/* If none, register entries */
						/* are missing. */
	fatal("get_ldavail: No register entries in live/dead table.\n");
 ldavail = GetAdAddrIndex(an_id) + 1;		/* Otherwise, get its index. */
 ldavail = (ldavail > (NVECTORS * sizeof(unsigned int) * B_P_BYTE)) ?
	(NVECTORS * sizeof(unsigned int) * B_P_BYTE) : ldavail;

  if(DBdebug(3,XGRA))
	(void) printf("\n%cgra: get_ldavail: ldavail=%d\n",
		ComChar,ldavail);
						/* Return the value. */
 return(ldavail);
}

	STATIC unsigned int
get_NVectors(ldavail)		/* Get number of used vectors in NVECTORS. */
unsigned int ldavail;		/* Lowest unused live-dead table entry. */

{unsigned int NVectors;		/* Number of words per vector. */

						/* Compute number of vectors. */
 NVectors = (ldavail + (sizeof(unsigned int) * B_P_BYTE) - 1) /
	(sizeof(unsigned int) * B_P_BYTE);

  if(DBdebug(3,XGRA))
	(void) printf("\n%cgra: get_NVectors: NVectors=%d\n",
		ComChar,NVectors);
						/* Return the value. */
 return(NVectors);
}
	STATIC void
init_gra(ldavail,NVectors,ldtab,argstat,exts,liveset,active,misoper,
	ones,assigned,regptrs)
				/* Initialize var and data areas for GRA.*/
unsigned int ldavail;		/* Lowest l-d entry not used. */
unsigned int NVectors;		/* Size to use for NVECTORS. */
struct ldent ldtab[];		/* Live-dead table for GRA. */
unsigned long int argstat[];	/* Ld entries that are arguments or loc stats. */
unsigned long int exts[];	/* Ld entries that are externals. */
unsigned long int liveset[];	/* Ld entries for vars that are live when set.*/
unsigned long int active[];	/* Ld entries for vars that are active. */
unsigned long int misoper[];	/* Ld entries for vars that are MIS operands. */
unsigned long int ones[];	/* Vector of all ones. */
unsigned long int assigned[];	/* Ld entries for vars that are assigned. */
struct ldent *regptrs[];	/* Ld entries for registers. */

{extern OperandType NAQ_type();	/* Returns type of NAQ for printing. */
 register AN_Id an_id;		/* AN_Id for l-d table entry. */
 register unsigned int ld;	/* Index for scanning l-d table. */
 struct ldent *pld;		/* Pointer to ld table entry. */
 extern unsigned int praddr();	/* Prints addresses. */
 RegId regid;			/* RegId for scanning registers. */
 register int vector;		/* Index for scanning words of bit vectors. */

						/* Initialize data areas. */
 for(vector = 0; vector < NVectors; vector++)
	{argstat[vector] = 0;
	 exts[vector] = 0;
	 liveset[vector] = 0;
	 active[vector] = 0;
	 misoper[vector] = 0;
	 ones[vector] = (unsigned long int)~0;
	 assigned[vector] = 0;
	 for(ld = 0; ld < ldavail; ld++)
		ldtab[ld].conflicts[vector] = 0;
	}

 for(ld = 0; ld < LIVEDEAD; ld++)		/* Initialize live-dead table.*/
	{pld = &ldtab[ld];			/* Get pointer to entry. */
	 pld->index = ld;			/* Store index. */
	 pld->passign = NULL;		/* Clear register assignments.*/
	 pld->var_class = VC_NONE;		/* Clear variable class. */
	}

 for(an_id = GetAdNextGNode((AN_Id) NULL);	/* Scan portion to be used. */
		an_id != NULL;
		an_id = GetAdNextGNode(an_id))
 	{ld = GetAdAddrIndex(an_id);
	 if(ld >= ldavail) 
		break;
	 pld = &ldtab[ld];
	 pld->laddr = an_id;			/* Set an_id. */
	 pld->word = ld / WDSIZE;		/* Set word in vectors. */
	 pld->bit_mask = 1 << (ld % WDSIZE);	/* Set bit mask in vectors. */
	 if(IsAdArg(an_id) || IsAdSNAQ(an_id))	/* If an arg, set bit. */
		ld_set_bit(argstat,pld);
						/* If external, set bit. */
	 if(GetAdGnaqType(an_id) == ENAQ || GetAdGnaqType(an_id) == SENAQ)
		ld_set_bit(exts,pld);
	}
						/* Get register pointers. */
 for(regid = CREG0; regid != REG_NONE; regid = GetNextRegId(regid))
	{an_id = GetAdCPUReg(regid);
	 if(IsAdAddrIndex(an_id))
		{ld = GetAdAddrIndex(an_id);
	 	 if(ld < ldavail)
						/* Store in array. */
	 		regptrs[(int) regid] = &ldtab[ld];
		}
	}

  if(DBdebug(4,XGRA))
	{(void) printf("\n%cgra: init_gra: after initialization:\n",
			ComChar);
	 (void) printf("%cgra: init_gra: live/dead table:\n",
			ComChar);
	 for(ld = 0; ld < ldavail; ld++)
		{(void) printf("%c\t",ComChar);
		 (void) printf("ld=%-2d ",ld);
		 (void) printf("word=%d ",ldtab[ld].word);
		 (void) printf("bit_mask=0x%8.8x ",(int)(ldtab[ld].bit_mask));
		 (void) praddr(ldtab[ld].laddr,NAQ_type(ldtab[ld].laddr),
			stdout);
		 putchar(NEWLINE);
		}
	 (void) printf("%cgra: init_gra: argstat:\n",ComChar);
	 for(ld = 0; ld < ldavail; ld++)
		{(void) printf("%c\t",ComChar);
		 (void) printf("ld=%-2d ",ld);
		 if(ld_get_bit(argstat,&ldtab[ld]) != 0)
			if(IsAdArg(ldtab[ld].laddr))
				(void) printf("        arg ");
		 	else
				(void) printf("       stat ");
		 else
				(void) printf(" notargstat ");
		 (void) praddr(ldtab[ld].laddr,NAQ_type(ldtab[ld].laddr),
		 	stdout);
		 putchar(NEWLINE);
		}
	 (void) printf("%cgra: init_gra: exts:\n",ComChar);
	 for(ld = 0; ld < ldavail; ld++)
		{(void) printf("%c\t",ComChar);
		 (void) printf("ld=%-2d ",ld);
		 if(ld_get_bit(exts,&ldtab[ld]) != 0)
			(void) printf("   ext ");
		 else
			(void) printf("notext ");
		 (void) praddr(ldtab[ld].laddr,NAQ_type(ldtab[ld].laddr),
		 	stdout);
		 putchar(NEWLINE);
		}
	 (void) printf("%cgra: init_gra: register pointers:\n",
			ComChar);
	 for(regid = CREG0; regid != REG_NONE; regid = GetNextRegId(regid))
		{(void) printf("%c\t",ComChar);
		 (void) printf("regid=%-2d ",(int) regid);
		 (void) printf("ld=%-2d ",regptrs[(int) regid]->index);
		 (void) praddr(regptrs[(int) regid]->laddr,
		 	NAQ_type(regptrs[(int) regid]->laddr),stdout);
		 putchar(NEWLINE);
		}
	}
 return;
}
	STATIC void
get_liveset(ldtab,ldavail,NVectors,argstat,exts,regptrs,liveset) 
				/* Get the sets vectors for all instructions, */
				/* and compute the liveset vector. */
struct ldent ldtab[];		/* Live-dead table for GRA. */
unsigned int ldavail;		/* Lowest unused entry in live-dead table. */
unsigned int NVectors;		/* Number of ld words to use.	*/
unsigned long int argstat[];	/* Variables that are args or loc stats. */
unsigned long int exts[];	/* Variables that are externals. */
struct ldent *regptrs[];	/* Array of pointers to ld entries for regs. */
unsigned long int liveset[];	/* Variables live where set. */

/* liveset - bit vector indicating those live/dead variables that are live at */
/* 	some point where they are set  (This is an indication that the */
/*	variable is actually doing something useful for the program) */

{extern OperandType NAQ_type();	/* Returns type of NAQ for printing. */
 struct ldent *p_struct_ret;	/* Ld entry for struct ret reg.	*/
 unsigned long int live[NVECTORS]; /* Variables that are live after instr. */
 register unsigned int opcode;	/* Operation code index. */
 unsigned int ld;		/* Index for scanning live-dead table. */
 extern unsigned int praddr();	/* Prints addresses. */
 unsigned long int set[NVECTORS]; /* Variables that are set by instr. */
 extern void sets();		/* Gets sets data for a text-node.	*/
 extern TN_Id skipprof();	/* Skips profiling nodes.	*/
 boolean str_ret_flag;		/* TRUE if looking for structure return addr. */
 register TN_Id tn_id;		/* Text node id for scanning text. */
 register int vector;		/* Index for scanning words in a bit vector. */

 						/* Compute liveset for args */
						/* at entry. */
 tn_id = GetTxPrevNode(skipprof(GetTxNextNode((TN_Id) NULL)));
 GetTxLive(tn_id,live,NVectors);		/* Get live for 1st instr. */
 for(vector = 0; vector < NVectors; vector++)
	liveset[vector] |= (live[vector] & (argstat[vector] | exts[vector]));
						/* Compute liveset */
						/* for other instr. */
 p_struct_ret = regptrs[(int) CSTRUCT_RET];	/* Get structure return */
						/* reg's address index. */
 str_ret_flag = TRUE;				/* We start out looking for */
						/* a move from %r2 that is */
						/* saving the address for a  */
						/* structure return. We stop */
						/* looking at first branch or */
						/* when %r2 is set. */
						/* NOTE: Cannot stop at label */
						/* because the -g option */
						/* causes a label to be placed*/
						/* before move of %r2. */
 for(ALLNSKIP(tn_id))				/* Scan text.	*/
	{if(IsTxBr(tn_id))			/* Look for structure returns */
		str_ret_flag = FALSE;		/* until first branch. */

	 if(IsTxLabel(tn_id))			/* Skip labels because they */
		continue;			/* set nothing. */

	 sets(tn_id,set,NVectors);		/* Gets sets data.	*/
	 opcode = GetTxOpCodeX(tn_id);		/* Get opcode index. */
						/* Ignore setting by restores */
	 if((opcode == IRET) || (opcode == RESTORE)) 
		for(vector=0; vector < NVectors; vector++)
			set[vector] = 0;		
	 PutTxDead(tn_id,set,NVectors);		/* Save for conflict calc. */

	 if(ld_get_bit(set,p_struct_ret))	/* If %r2 is set, */
		str_ret_flag = FALSE;		/* stop looking. */
	 if(str_ret_flag 			/* If structure return, */
			&& opcode == G_MOV
			&& p_struct_ret->laddr == GetTxOperandAd(tn_id,2))
		ld_set_bit(liveset,p_struct_ret); /* indicate %r2 liveset. */
						/* Ignore setting of struct */
						/* return register by */
						/* CALLS for liveset. */
	 if((opcode == ICALL) || (opcode == CALL))
		ld_clr_bit(set,p_struct_ret);
	 GetTxLive(tn_id,live,NVectors);	/* Get nlive data. */
						/* Compute liveset word. */
	 for(vector = 0; vector < NVectors; vector++)
		liveset[vector] |= live[vector] & set[vector];
	} /* END OF for(ALLNSKIP(tn_id)) */

  if(DBdebug(4,XGRA))
	{(void) printf("\n%cgra: get_liveset after computing liveset:\n",
		ComChar);
	 for(ld = 0; ld < ldavail; ld++)
		{(void) printf("%c\t",ComChar);
		 (void) printf("ld=%-2d ",ld);
		 if(ld_get_bit(liveset,&ldtab[ld]) != 0)
			(void) printf("   liveset ");
		 else
			(void) printf("notliveset ");
		 (void) praddr(ldtab[ld].laddr,NAQ_type(ldtab[ld].laddr),
			stdout);
		 putchar(NEWLINE);
		}
	}

 return;
}

	STATIC struct ldent *
get_vars(ldtab,ldavail,NVectors,regptrs,liveset,misoper) 
				/* Return head pointer for a NULL 	  */
				/* terminated linked list of entries	  */
				/* for register and stack variables in the*/
				/* live-dead table in the order for GRA.  */
				/* The order is:			  */
				/*	(a) R0,R1,R2			  */
				/*	(b) R8,R7,R6,R5,R4,R3		  */
				/*	(c) (32200 only) R16,R17,R18,R19  */
				/*	    R20,R21,R22,R23		  */
				/* 	(d) stk vars and externals that   */
				/*          are live when set and are not */
				/*	    operands of MIS instructions  */
				/*	    ordered by decreasing estim	  */

struct ldent ldtab[];		/* Live-dead table for GRA. */
unsigned int ldavail;		/* First unused entry in live-dead table. */
unsigned int NVectors;		/* Number of words in vectors. */
struct ldent *regptrs[];	/* Array of pointers to ld entries for regs. */
unsigned long int liveset[];	/* Variables that are live when set. */
unsigned long int misoper[];	/* Variables that are MIS operands. */

{
 extern OperandType NAQ_type();	/* Returns the type of a NAQ for printing. */
 boolean aliased_exts;		/* TRUE if externals aliased by pointers. */
 AN_Id an_id;			/* AN_Id for stack variable. */
 STATIC void get_vardata();	/* Get info on vars from context. */
 STATIC void gra_funcprint();	/* Prints instructions with live and set info.*/
 register unsigned int ld;	/* Index into ld table. */
 struct ldent *plast;		/* Pointer to last entry in variable list. */
 register struct ldent *pv;	/* Pointer to variable entry in ld table. */
 extern unsigned int praddr();	/* Prints an address. */
 register RegId regid;		/* RegId for scanning registers. */
 struct ldent *var_head;	/* Head pointer for variable list. */

 var_head = NULL;				/* Construct register order. */
 plast = NULL;					/* Insert 1st set scr */
						/* registers in increasing */
						/* order. */
 for(regid = CSCR1ST_MIN; GetPrevRegId(regid) != CSCR1ST_MAX; 
		regid = GetNextRegId(regid))
	{pv = regptrs[(int) regid];
	 if(var_head == NULL)			/* Add to end of list.*/
		var_head = pv;
	 else
		plast->var_next = pv;
	 pv->var_next = NULL;
	 plast = pv;
	 pv->var_class = VC_CSCR1ST;		/* Set register class. */
	}
 						/* Insert saved registers in */
						/* decreasing order. */
 for(regid = CSVD_MAX; GetNextRegId(regid) != CSVD_MIN; 
		regid = GetPrevRegId(regid))
	{pv = regptrs[(int) regid];
	 pv->regpaired = 0;
	 plast->var_next = pv;
	 pv->var_next = NULL;
	 plast = pv;
	 pv->var_class = VC_CSVD;		/* Set register class. */
	}

#ifdef W32200
 						/* Insert 2nd set scr */
						/* registers in increasing */
						/* order. */
 if(cpu_chip == we32200)			/* Omit these registers */
						/* if not 32200. */
 	{for(regid = CSCR2ND_MIN; GetPrevRegId(regid) != CSCR2ND_MAX; 
			regid = GetNextRegId(regid))
		{pv = regptrs[(int) regid];
		 plast->var_next = pv;
		 pv->var_next = NULL;
		 plast = pv;
		 pv->var_class = VC_CSCR2ND;	/* Set Register class. */
		}
	}
#endif

						/* Collect data on vars */
						/* from text nodes. */
 get_vardata(NVectors,regptrs,misoper,&aliased_exts);

 						/* Insert integer stack */
						/* and external */
						/* variables in */
						/* ld table order. */
 for(ld = 0; ld < ldavail; ld++)
	{an_id = ldtab[ld].laddr;
	 if(IsAdFP(an_id))			/* Ignore if floating point. */
		continue;
	 pv = &ldtab[ld];
	 if(ld_get_bit(liveset,pv) == 0)	/*Ignore if not live when set.*/
		continue;
	 if(ld_get_bit(misoper,pv) != 0)	/* Ignore if oper of MIS.*/
		continue;
	 if(IsAdArg(an_id) || IsAdAuto(an_id))
		pv->var_class = VC_ISTK;	/* Set variable class. */
	 else if(IsAdSNAQ(an_id))
		pv->var_class = VC_STAT;
	 else if(aliased_exts)			/* globals are volatile */
		continue;
	 else if(GetAdGnaqType(an_id) == SENAQ)
		 pv->var_class = VC_STATEXT;
	 else if(GetAdGnaqType(an_id) == ENAQ)
		 pv->var_class = VC_EXTDEF;
	 else
		continue;
	 plast->var_next = pv;
	 pv->var_next = NULL;
	 plast = pv;
	}

  if(DBdebug(4,XGRA))
	{(void) printf("\n%cgra: get_vars after computing:\n",
		ComChar);
	 (void) printf("%cgra: get_vars: misoper:\n",
		ComChar);
	 for(ld = 0; ld < ldavail; ld++)
		{(void) printf("%c\t",ComChar);
		 (void) printf("ld=%-2d ",ld);
		 if(ld_get_bit(misoper,&ldtab[ld]) != 0)
			(void) printf("   misoper ");
		 else
			(void) printf("notmisoper ");
		 (void) praddr(ldtab[ld].laddr,NAQ_type(ldtab[ld].laddr),
			stdout);
		 putchar(NEWLINE);
		}
	 (void)printf("%cgra: get_vars: %s\n", ComChar,
		aliased_exts ? "   aliased_ext" : "NOTaliased_ext");
	 (void) printf("%cgra: get_vars: var list:\n",
		ComChar);
	 for(pv = var_head; pv; pv = pv->var_next)
		{(void) printf("%c\t",ComChar);
		 (void) printf("ld=%-2d ",pv->index);
		 (void) printf("class=0x%8.8x ",(int)(pv->var_class));
		 (void) praddr(pv->laddr,NAQ_type(pv->laddr),stdout);
		 putchar(NEWLINE);
		}
	 gra_funcprint("after constructing variable list",NVectors,var_head);
	}
 return(var_head);
}
	STATIC void
get_vardata(NVectors,regptrs,misoper,aliased_exts)

unsigned int NVectors;		/* Number of words in vectors. */
struct ldent *regptrs[];	/* Array of pointers to ld entries for regs. */
unsigned long int misoper[];	/* Variables that are MIS operands. */
boolean *aliased_exts;		/* Has externs been aliased? */

{extern boolean ExternAlias();	/* TRUE if externals are aliased.	*/
 extern boolean Zflag;		/* TRUE if globals are non-volatile. */
 AN_Id an_id;			/* AN_Id for stack variable. */
 boolean aliased;		/* TRUE if externs are possibly aliased. */
 extern boolean misinp;		/* TRUE if input contains MIS instructions. */
 RegId regid;			/* RegId for scanning registers. */
 unsigned long int set[NVECTORS]; /* Variables that are set by an instr. */
 extern TN_Id skipprof();	/* Skips profiling nodes.	*/
 register TN_Id tn_id;		/* TextNode Id for scanning text nodes. */
 OperandType ty;		/* Text node operand type. */
 unsigned long int used[NVECTORS]; /* Variables that are used by an instr. */
 extern void uses();		/* Gets uses data for a text-node.	*/
 int vector;			/* Index for scanning words in bit vectors. */

 aliased = FALSE;
 if(!Zflag)			/* If globals are potentially volatile, */
	aliased = TRUE;		/* they might as well all be aliased.	*/

 for(ALLNSKIP(tn_id))
	{if(misinp && IsTxMIS(tn_id))
		/* Note all operands of MIS instructions. */
		{
		 GetTxDead(tn_id,set,NVectors);
		 uses(tn_id,used,NVectors);	/* Get uses data.	*/
		 for(vector=0; vector < NVectors; vector++)
						/* Compute misoper word. */
			misoper[vector] |= (set[vector] | used[vector]);
		}
	if(GetTxOpCodeX(tn_id) == G_MMOV)
		/* Mark all CPU register pairs/triples in G_MMOV instructions. */
		{
		 an_id = GetTxOperandAd(tn_id,2);
		 ty = GetTxOperandType(tn_id,2);
		 if(IsAdCPUReg(an_id))
			switch(ty)
			{case (int)Tdouble:
			 case (int)Tdblext:
				regid = GetAdRegA(an_id);
				regptrs[(int)regid]->regpaired = 1;
				regid = GetNextRegId(regid);
				regptrs[(int)regid]->regpaired = 1;
				if(ty == Tdouble)
					break;
				regid = GetNextRegId(regid);
				regptrs[(int)regid]->regpaired = 1;
			}
		 an_id = GetTxOperandAd(tn_id,3);
		 ty = GetTxOperandType(tn_id,3);
		 if(IsAdCPUReg(an_id))
			switch(ty)
			{case (int)Tdouble:
			 case (int)Tdblext:
				regid = GetAdRegA(an_id);
				regptrs[(int)regid]->regpaired = 1;
				regid = GetNextRegId(regid);
				regptrs[(int)regid]->regpaired = 1;
				if(ty == Tdouble)
					break;
				regid = GetNextRegId(regid);
				regptrs[(int)regid]->regpaired = 1;
			}
		}
	if(!aliased)
		/* Check whether externs can be aliased */
		/* by pointer dereferences. */
		aliased = ExternAlias(tn_id);
	}

 *aliased_exts = aliased;
}
	STATIC void
get_conflicts(sub_head,NVectors,argstat,exts,liveset) 
				/* Compute the conflicts matrix */
				/* for the indicated subset of variables, */
				/* and clear live information in the text */
				/* for variables that are not liveset. */
struct ldent *sub_head;		/* Head pointer for subset list. */
unsigned int NVectors;		/* Number of ld words to use.	*/
unsigned long int argstat[];	/* Variables that are args or loc stats. */
unsigned long int exts[];	/* Variables that are externals. */
unsigned long int liveset[];	/* Variables that are live when set. */
	
/* ldtab[].conflicts - set of bit vectors that, together, give a matix of */
/*	the conflicts between live/dead variables where A conflicts with */
/*	B if A is set someplace where B is live. */

{extern OperandType NAQ_type();	/* Returns the type of a NAQ for printing. */
 unsigned long int live[NVECTORS]; /* Variables that are live after instr. */
 extern unsigned int praddr();	/* Prints an address. */
 register struct ldent *psub;	/* Pointer for scanning variable list. */
 struct ldent *psub2;		/* Ld pointer for scanning across conflicts.*/
 unsigned long int set[NVECTORS]; /* Variables that are set by instr. */
 extern TN_Id skipprof();	/* Skips profiling nodes.	*/
 unsigned long int temp;	/* Temp for collecting set bits of vector. */
 register TN_Id tn_id;		/* Text node id for scanning text. */
 register int vector;		/* Index for scanning words of a bit vector. */

 for(psub = sub_head; psub; psub = psub->sub_next) /* Initialize. */
 	for(vector = 0; vector < NVectors; vector++)
		psub->conflicts[vector] = 0;
						/* Compute arg conflicts. */
 tn_id = GetTxPrevNode(skipprof(GetTxNextNode((TN_Id) NULL)));
 GetTxLive(tn_id,live,NVectors);		/* Get live for 1st instr. */
 for(vector = 0; vector < NVectors; vector++)	/* Clear live bits */
	live[vector] &= liveset[vector];	/* for unused NAQs. */
 for(psub = sub_head; psub; psub = psub->sub_next)
	if(ld_get_bit(argstat,psub) | ld_get_bit(exts,psub))
		for(vector = 0; vector < NVectors; vector++)
			psub->conflicts[vector] |= live[vector];

						/* Compute rest of conflicts. */
 for(ALLNSKIP(tn_id)) 				/* Scan text.	*/
	{if(IsTxLabel(tn_id))			/* Skip labels because	*/
						/* they set nothing and have */
		continue;			/* no live data. */
	 GetTxLive(tn_id,live,NVectors);	/* Get nlive data. */
	 GetTxDead(tn_id,set,NVectors);
	 temp = 0;				/* If this instruction sets */
						/* nothing, skip conflicts  */
						/* calculation.	*/
	 for(vector = 0; vector < NVectors; vector++)
		{live[vector] &= liveset[vector];	/* Clear live bits */
						   	/* for unused NAQs */
		 temp |= set[vector];		/* (see if it sets anything) */
		}
	 PutTxLive(tn_id,live,NVectors);	/* Update live data.	*/
	 if(temp == 0)				/* Does it set anytning? */
		continue;			/* No: skip calculation. */

	 if(IsTxAnyCall(tn_id))			/* Ignore conflicts at calls */
						/* for externals because we */
						/* put protection around */
						/* calls later. */
		for(vector = 0; vector < NVectors; vector++)
			live[vector] &= ~exts[vector];

	 					/* Compute conflicts. */
	 for(psub = sub_head; psub; psub = psub->sub_next)
		 if(ld_get_bit(set,psub) != 0)
			for(vector = 0; vector < NVectors; vector++)
				psub->conflicts[vector] |= live[vector];
	} /* END OF for(ALLNSKIP(tn_id)) */

  if(DBdebug(3,XGRA))
	{(void) printf("\n%cgra: get_conflicts: conflicts:\n",
		ComChar);
	 for(psub = sub_head; psub; psub = psub->sub_next)
		{(void) printf("%c\t",ComChar);
		 (void) printf("ld=%-2d ",psub->index);
		 (void) praddr(psub->laddr,NAQ_type(psub->laddr),stdout);
		 (void) printf(": ");
	 	 for(psub2 = sub_head; psub2; psub2 = psub2->sub_next)
			if(ld_get_bit(psub->conflicts,psub2) != 0)
				{(void) praddr(psub2->laddr,
					NAQ_type(psub2->laddr),stdout);
			 	 (void)printf(" ");
				}
		 putchar(NEWLINE);
		}
	}
 return;
}
	STATIC void
get_active(var_head,NVectors,liveset,active) 
				/* Compute active vector. */
struct ldent *var_head;		/* Head of variable list. */
unsigned int NVectors;		/* Number of ld words to use.	*/
unsigned long int liveset[];	/* Variables that live and set. */
unsigned long int active[];	/* Variables that are liveset and saved. */

/* active -  bit vector same as liveset except with all the bits set for */
/* 	scratch registers and registers being saved prior to allocation. */
	
{extern FN_Id FuncId;		/* Id of current function. */
 extern OperandType NAQ_type();	/* Returns the type for printing.*/
 register unsigned int n;	/* Counter for number of saved registers. */
 extern unsigned int praddr();	/* Prints and address. */
 register struct ldent *pvar;	/* Pointer for scanning variables in the list.*/
 register int vector;		/* Index for scanning words of bit vectors. */

 for(vector = 0; vector < NVectors; vector++)	/* First get all the bits */
	active[vector] = liveset[vector];	/* in liveset. */

 for(pvar = var_head; pvar; pvar = pvar->var_next) /* Add set bits for */
	if(pvar->var_class & (VC_CSCR1ST|VC_CSCR2ND))	/* CPU scratch regs. */
		ld_set_bit(active,pvar);	/* This is needed to pick up */
						/* conflicts with sets by */
						/* calls. */

 n = GetFnNumReg(FuncId);			/* Finally set bits for */
 for(pvar = var_head; pvar; pvar = pvar->var_next) /* registers actually */
	if(pvar->var_class & VC_CSVD)		/* saved. */
		{if(n-- == 0)
			break;
		 ld_set_bit(active,pvar);
		}

 if(DBdebug(4,XGRA))
	{(void) printf("\n%cgra: get_active after computing active:\n",
		ComChar);
	 for(pvar = var_head; pvar; pvar = pvar->var_next)
		{(void) printf("%c\t",ComChar);
		 (void) printf("ld=%-2d ",pvar->index);
		 if(ld_get_bit(active,pvar) != 0)
			(void) printf("   active ");
		 else
			(void) printf("notactive ");
		 (void) praddr(pvar->laddr,NAQ_type(pvar->laddr),stdout);
		 putchar(NEWLINE);
		}
	}
 return;
}
	STATIC struct ldent *	
seq_scratch(pa,regptrs,liveset,assigned) /*Sequence scratch register merges.*/
register struct ldent *pa;	/* Ld entry of register to be merged. */
struct ldent *regptrs[];	/* Ld entries for registers. */
unsigned long int liveset[];	/* Variables that are live where set. */
unsigned long int assigned[];	/* Variables that are assigned to registers. */

	/* Merge strategy:						*/
	/*								*/
	/*	R2 liveset	# ret regs	fold A->B		*/
	/*	---------- 	----------	---------		*/
	/*	No		0,1		R1->R0			*/
	/*	No		2		-			*/
	/*	Yes		0		R0->R2,R0->R1,R1->R2	*/
	/*	Yes		1		R1->R2			*/
	/*	Yes		2		-			*/
	/*								*/

{extern OperandType NAQ_type();	/* Returns the type of a NAQ for printing. */
 extern void fatal();		/* Handles fatal errors.	*/
 register struct ldent *pb;	/* LD entry of register to merge into. */
 extern unsigned int praddr();	/* Prints an address. */
 struct ldent *pR2;		/* Address index of CPU Register 2.	*/
 struct ldent *return_val;	/* Return value. */
 extern unsigned int rvregs;	/* Number of registers usedfor return value. */

 return_val = NULL;

 if(ld_get_bit(liveset,pa) == 0)		/* Don't merge if not live */
						/* when set. */
	goto exit_scratch;

 pR2 = regptrs[(int) CREG2];			/* Get index for R2. */

 if(pa == regptrs[(int) CREG0])			/* Is first register R0? */
	{
						/* Is %r2 liveset with no */
						/* register being used to */
						/* return a value? */
	 if((ld_get_bit(liveset,pR2) != 0) && (rvregs == 0))

						/* Try merging into R2. */
		{pb = regptrs[(int) CREG2];
						/* Check conflicts. */
 		 if((ld_get_bit(pa->conflicts,pb)
				| ld_get_bit(pb->conflicts,pa)) == 0)
 			{			/* Record re-assignment. */
			 pa->passign = pb;
			 ld_set_bit(assigned,pa);
			 return_val = pb;
			 goto exit_scratch;
			}
						/* Try merging into R1. */
		 pb = regptrs[(int) CREG1];
						/* Check conflicts. */
 		 if((ld_get_bit(pa->conflicts,pb) 
				| ld_get_bit(pb->conflicts,pa)) == 0)
 			{			/* Record re-assignment. */
			 pa->passign = pb; 
			 ld_set_bit(assigned,pa);
			 return_val = pb;
			 goto exit_scratch;
			}
		}
	}
 else if(pa == regptrs[(int) CREG1])		/* Is first register R1? */

						/* Is R2 liveset, but not */
						/* used to return a value. */
	{if((ld_get_bit(liveset,pR2) != 0) && (rvregs <= 1))
						/* Try merging into R2. */
		{pb = regptrs[(int) CREG2];
						/* Check conflicts. */
 		 if((ld_get_bit(pa->conflicts,pb) 
				| ld_get_bit(pb->conflicts,pa)) == 0)
 			{			/* Record re-assignment. */
			 pa->passign = pb; 
			 ld_set_bit(assigned,pa);
			 return_val = pb;
			 goto exit_scratch;
			}
		} 
						/* Is R2 neither liveset nor */
						/* used to return a value? */
	 if((ld_get_bit(liveset,pR2) == 0) && (rvregs <= 1))
						/* Try merging into R0. */
		{pb = regptrs[(int) CREG0];
						/* Check conflicts. */
 		 if((ld_get_bit(pa->conflicts,pb) 
				| ld_get_bit(pb->conflicts,pa)) == 0)
 			{			/* Record re-assignment. */
			 pa->passign = pb; 
			 ld_set_bit(assigned,pa);
			 return_val = pb;
			 goto exit_scratch;
			}
		}
	}
 else if(pa == regptrs[(int) CREG2])	 	/* It is register 2. */
	{goto exit_scratch;
	}
 else
	fatal("seq_scratch: unexpected register\n" );
exit_scratch:

	if(DBdebug(3,XGRA))
		{(void) printf("\n%c",ComChar);
		 (void) printf("gra: seq_scratch: ");
		 if(ld_get_bit(liveset,pa) != 0)
			(void) printf("   liveset ");
		 else
			(void) printf("notliveset ");
		 (void) praddr(pa->laddr,NAQ_type(pa->laddr),stdout);
		 if(return_val == NULL)
			{(void) printf(" cannot be merged");
			}
		 else
			{(void) printf(" can be merged into ");
		 	 (void) praddr(pb->laddr,NAQ_type(pb->laddr),stdout);
			}
		 putchar(NEWLINE);
		}

 return(return_val);
}
	STATIC void 
seq_classes(var_head,NVectors,active,assigned,ones) /* Sequence merging of */
				/* variable classes. */
struct ldent *var_head;		/* Head of variable list. */
unsigned int NVectors;		/* Number of ld words to use.	*/
unsigned long int active[];	/* Variables that are active. */
unsigned long int assigned[];	/* Variables that are assigned to regs. */
unsigned long int ones[];	/* Vector of all ones. */

{STATIC void seq_vars();	/* Sequence merging of variables. */

				/* Merge saved registers and */
				/* integer stack variables and externals into */
				/* 1st set CPU scratch registers and  */
				/* saved registers. */
 seq_vars(var_head,VC_CSVD|
		   VC_ISTK|
		   VC_STAT|
		   VC_STATEXT|
		   VC_EXTDEF, VC_CSCR1ST|
		   	      VC_CSVD,
		NVectors,active,assigned,ones);

#ifdef W32200
				/* Merge integer stack variables and */
				/* externals into 2nd set CPU scratch */
				/* registers. */
 seq_vars(var_head,VC_ISTK|
		   VC_STAT|
		   VC_STATEXT|
		   VC_EXTDEF,VC_CSCR2ND,
		NVectors,active,assigned,ones);
#endif

}
	STATIC void
seq_vars(var_head,maska,maskb,NVectors,active,assigned,ones) 
				/* Sequence merging of variables. */
struct ldent *var_head;		/* Head of variable list. */
unsigned long int maska;	/* Class mask for variables to be merged. */
unsigned long int maskb;	/* Class mask for variables to merge into. */
unsigned int NVectors;		/* Number of ld words to use.	*/
unsigned long int active[];	/* Variables that are active. */
unsigned long int assigned[];	/* Variables that are assigned to regs. */
unsigned long int ones[];	/* Vector of all ones. */

{extern FN_Id FuncId;
 extern OperandType NAQ_type();	/* Returns type for printing. */
 extern enum CC_Mode ccmode;	/* cc -X? */
 int estim;			/* Estimator of cycle payoff. */
 register struct ldent *pa;	/* Ld entry of register to be merged. */
 register struct ldent *pb;	/* Ld entry of register to merge into. */
 extern unsigned int praddr();	/* Prints address node to a string.	*/
 struct ldent *sub_head;	/* Head pointer for subset list. */

 sub_head = get_sub(var_head,maskb,ones);	/* Get subset of variables to */
						/* merge into. */
						/* Scan variables to merge. */
 for(pa = var_head; pa; pa = pa->var_next)
	{if((pa->var_class & maska) == 0)	/* Scan only specified */
		continue;			/* variables. */
	 if(pa->var_class & (VC_ISTK|VC_STAT|VC_STATEXT|VC_EXTDEF)) 		
						/* Variable checks. */
	 	{if(ccmode == Transition	/* Don't assign if */
				&& IsFnSetjmp(FuncId)) /* in transition mode &*/
						/* there is a setjmp in func. */
			continue;
 	 	 if((estim = GetAdEstim(pa->laddr)) < 0) /* Check payoff. */
			continue;		/* Not enough. */
		}
	 else if(pa->var_class == VC_CSVD && pa->regpaired == 1)
						/* Don't bother merge register */
						/* pairs used for doubles. */
		continue;
 	 if(ld_get_bit(active,pa) == 0)		/* Consider only active. */
		continue;
						/* Merge into active vars. */
	 for(pb = sub_head; pb; pb = pb->sub_next) /* Scan registers to merge */
						/* into. */
		{if(pa == pb)			/* Don't put a register into */
			break;			/* lower numbered saved */
						/* register or higher */
						/* numbered 2nd set scr. */
						/* Check active flag against */
						/* against whether variable */
						/* is active. */
		 if(ld_get_bit(active,pb) == 0) /* Is it active? */
			continue;		/* No, forget it. */
			
						/* Check conflicts. */
 	 	 if((ld_get_bit(pa->conflicts,pb) 
				| ld_get_bit(pb->conflicts,pa)) == 0)
						/* Update active vector */
						/* and conflicts matrix. */
			{upd_data(pa,pb,var_head,NVectors,active,assigned);
			 break;
			}
		} /* END OF for(pb = sub_head; pb; pb = pb->sub_next) */

 	 if(ld_get_bit(active,pa) == 0)		/* Consider only still active.*/
		continue;
						/* Merge into inactive vars. */
	 for(pb = sub_head; pb; pb = pb->sub_next) /* Scan registers to merge */
						/* into. */
		{if(pa == pb)			/* Don't put a register into */
			break;			/* lower numbered saved */
						/* register or higher */
						/* numbered 2nd set scr. */
						/* Check active flag against */
						/* against whether variable */
						/* is active. */
		 if(ld_get_bit(active,pb) != 0) /* Is it active? */
			continue;		/* Yes, forget it. */
						/* Before merging */
						/* into an inactive saved */
						/* reg, check whether can */
						/* afford to save/restore */
						/* another reg. */
		 if((pa->var_class & (VC_ISTK|VC_STAT|VC_STATEXT|VC_EXTDEF)) 
				&& (pb->var_class == VC_CSVD)
		 		&& ((estim = (estim - CYCDELTA)) < 0))
			continue;
						/* Check conflicts. */
 	 	 if((ld_get_bit(pa->conflicts,pb) 
				| ld_get_bit(pb->conflicts,pa)) == 0)
						/* Update assignment vector, */
						/* active vector, */
						/* and conflicts matrix. */
			{upd_data(pa,pb,var_head,NVectors,active,assigned);
			 break;
			}
		} /* END OF for(pb = sub_head; pb; pb = pb->sub_next) */
	} /* END OF for(pa = var_head; pa; pa = pa->var_next) */

 if(DBdebug(3,XGRA))
	{(void) printf("\n%c",ComChar);
	 (void) printf("gra: seq_vars: ");
	 if(!IsAdCPUReg(pa->laddr))
		if(IsFnSetjmp(FuncId))
	 		(void) printf("   setjmp ");
	 	else
			(void) printf("notsetjmp ");
	 putchar(NEWLINE);
	 for(pa = var_head; pa; pa = pa->var_next)
		{(void) printf("%c\t",ComChar);
		 if(pa->var_class & (VC_ISTK|VC_STATEXT|VC_EXTDEF))
			{if(ld_get_bit(active,pa) != 0)
				(void) printf("   active ");
		 	 else
				(void) printf("notactive ");
			}
		 if(pa->var_class & (VC_ISTK|VC_STATEXT|VC_EXTDEF))
			(void) printf("estim=%d ",estim);
		 (void) praddr(pa->laddr,NAQ_type(pa->laddr),stdout);
		 if(ld_get_bit(assigned,pa) == 0)
			{(void) printf(" not assigned");
			}
		 else
			{(void) printf(" assigned to ");
		 	 (void) praddr(pa->passign->laddr,
					NAQ_type(pa->passign->laddr),stdout);
			}
		 putchar(NEWLINE);
		} /* END OF for(pvar = var_head; pvar; pvar = pvar->var_next) */
	}/* END OF if(DBdebug(3,GRA) */
}
	STATIC void
upd_data(pa,pb,var_head,NVectors,active,assigned) 
				/* Update assignment vector, active vector, */
				/* and conflicts matrix to reflect assignment. */
register struct ldent *pa;	/* Ld entry of variable to merge in. */
struct ldent *pb;		/* Ld entry of variable to merge into. */
struct ldent *var_head;		/* Head of variable list. */
unsigned int NVectors;		/* Number of vectors to use.	*/
unsigned long int active[];	/* Variables that are active. */
unsigned long int assigned[];	/* Variables that are assigned to regs. */

{extern OperandType NAQ_type();	/* Returns the type for printing. */
 extern unsigned int praddr();	/* Prints an address. */
 register struct ldent *pvar;	/* Pointer for scanning variable list. */
 struct ldent *pvar2;		/* Pointer for scanning variable list. */
 register unsigned int vector;	/* Index for scanning words of bit vectors. */

 pa->passign = pb; 				/* Record assignment. */
 ld_set_bit(assigned,pa);

 if(ld_get_bit(active,pa) != 0)			/* Merge active bits. */
	{ld_set_bit(active,pb);
	 ld_clr_bit(active,pa);
	}

 for(vector = 0; vector < NVectors; vector++)
	{					/* Merge Conflicts. */
	 pb->conflicts[vector] |= pa->conflicts[vector];
	 pa->conflicts[vector] = 0;
	}
 for(pvar = var_head; pvar; pvar = pvar->var_next)
	if(ld_get_bit(pvar->conflicts,pa))
		{ld_set_bit(pvar->conflicts,pb);
		 ld_clr_bit(pvar->conflicts,pa);
		}

 if(DBdebug(4,XGRA))
	{(void) printf("\n%cgra: upd_data after the update:\n",
		ComChar);
	 (void) printf("%cgra: upd_data: active:\n",
		ComChar);
	 for(pvar = var_head; pvar; pvar = pvar->var_next)
		{(void) printf("%c\t",ComChar);
		 (void) printf("ld=%-2d ",pvar->index);
		 if(ld_get_bit(active,pvar) != 0)
			(void) printf("   active ");
		 else
			(void) printf("notactive ");
		 (void) praddr(pvar->laddr,NAQ_type(pvar->laddr),stdout);
		 putchar(NEWLINE);
		}
	 (void) printf("%cgra: upd_data: conflicts:\n",
		ComChar);
 	 for(pvar = var_head; pvar; pvar = pvar->var_next)
		{(void) printf("%c\t",ComChar);
		 (void) printf("ld=%-2d ",pvar->index);
		 (void) praddr(pvar->laddr,NAQ_type(pvar->laddr),stdout);
		 (void) printf(": ");
	 	 for(pvar2 = var_head; pvar2; pvar2 = pvar2->var_next)
			if(ld_get_bit(pvar->conflicts,pvar2) != 0)
				{(void) praddr(pvar2->laddr,
					NAQ_type(pvar2->laddr),stdout);
			 	 (void)printf(" ");
				}
		 putchar(NEWLINE);
		}
	}
 return;
}
	STATIC void
naqmerge(sub_head,NVectors,regptrs) /*Merge NAQs into registers.*/
struct ldent *sub_head;		/* Head of list of subset to be considered. */
unsigned int NVectors;		/* Number of vectors to use.	*/
struct ldent *regptrs[];	/* Ld entries for registers. */

{extern unsigned int Max_Ops;	/* Maximum operands for an instruction. */
 AN_Id ada;			/* AN_Id of variable to be merged. */
 AN_Id adb;			/* AN_Id of register to merge into. */
 AN_Id ad_used;			/* First address used by an address. */
 AN_Id ad_used_used;		/* First address used by addr used by an addr.*/
 AN_Id an_id;			/* AN_Id for holding rewritten address. */
 extern void fatal();		/* Handles fatal errors.	*/
 boolean flag;			/* TRUE if MOVA/PUSHA src is in subset. */
 unsigned long int live[NVECTORS]; /* Live info from text node. */
 AN_Mode mode;			/* Mode of adddress node. */
 AN_Id newan_id;		/* AN_Id for scanning operands.  */
 unsigned short int opcode;	/* Opcode of instruction. */
 register unsigned int operand;	/* Operand counter.	*/
 register struct ldent *pa;	/* Ld entry for variable to be merged. */
 extern void prafter();		/* Prints instructions after a change.	*/
 extern void prbefore();	/* Prints instructions before a change.	*/
 extern TN_Id skipprof();	/* Skips profiling nodes.	*/
 register TN_Id tn_id;		/* TN_Id for scanning text. */
 OperandType ty;		/* Operand type. */

 for(tn_id = GetTxPrevNode(skipprof(GetTxNextNode((TN_Id) NULL)));
		tn_id != (TN_Id) NULL;
		tn_id = GetTxNextNode(tn_id))
	{if(IsTxLabel(tn_id))			/* Don't bother with labels. */
		continue;

	 GetTxLive(tn_id,live,NVectors);	/* Get live data.	*/

						/* Modify live vector to  */
						/* reflect assignments. */
	 for(pa = sub_head; pa; pa = pa->sub_next)
		{if(ld_get_bit(live,pa))
			{ld_set_bit(live,pa->passign);
						/* If pa not scr reg, */
						/* can clear previous var. */
			 if(pa != regptrs[(int) CREG0] 
					&& pa != regptrs[(int) CREG1] 
					&& pa != regptrs[(int) CREG2])
				ld_clr_bit(live,pa);
			}
		}
	 PutTxLive(tn_id,live,NVectors);	/* Put live data back into */
						/* text node. */

	 opcode = GetTxOpCodeX(tn_id);		/* Get instruction opcode. */
	 if(opcode == SAVE || opcode == ISAVE)
		continue;			/* Skip CPU save.	*/
	 if(IsOpAux(opcode))	
		continue;			/* Skip auxiliary nodes. */
	 for(ALLOP(tn_id,operand))		/* Scan operands.	*/
		{ty = GetTxOperandType(tn_id,operand);
		 if(ty == Tnone)
			break;			/* No more operands.	*/

						/* Get old AN_Id. */
		 an_id = GetTxOperandAd(tn_id,operand);
		 newan_id = NULL;
						/* Source of a PUSHA or MOVA*/
						/* cannot be a reference to */
						/* a NAQ because a NAQ cannot */
						/* have its address taken. */
		 flag = FALSE;
		 if((operand == PMA_SOURCE)
		 		&& ((opcode == G_MOVA) || (opcode == G_PUSHA)))
			for(pa = sub_head; pa; pa = pa->sub_next)
				if(an_id == pa->laddr)
					flag = TRUE;
		 if(flag)
			continue; 

						/* Rewrite operands. */
		 switch(mode = GetAdMode(an_id))	
			{case Immediate:
			 case MAUReg:
			 case StatCont:
			 case Raw:
				continue;
			 case CPUReg:
			 case Absolute:
				for(pa = sub_head; pa; pa = pa->sub_next)
					{ada = pa->laddr;
					 if(ada == an_id)
					 	{adb = pa->passign->laddr;
					 	 newan_id = adb;
					 	 break;
						}
					}
				endcase;
			 case Disp:
				ad_used = GetAdUsedId(an_id,0);
				for(pa = sub_head; pa; pa = pa->sub_next)
					{ada = pa->laddr;
					 if(ada == an_id)
						{adb = pa->passign->laddr;
						 newan_id = adb;
						 break;
						}
					 else if(ada == ad_used)
		 			 	{adb = pa->passign->laddr;
						 newan_id = GetAdChgRegAInc(ty,
							an_id,GetAdRegA(adb),0);
						 break;
						}
					}
				endcase;
			 case DispDef:
			 case AbsDef:
				ad_used = GetAdUsedId(an_id,0);
				ad_used_used = GetAdUsedId(ad_used,0);
				for(pa = sub_head; pa; pa = pa->sub_next)
					{ada = pa->laddr;
					 if(ada == ad_used)
						{adb = pa->passign->laddr;
						 newan_id = GetAdDisp(ty,"",
							GetAdRegA(adb));
						 break;
						}
					 else if(ada == ad_used_used)
						{adb = pa->passign->laddr;
		 			 	 newan_id = GetAdChgRegAInc(ty,
							an_id,GetAdRegA(adb),0);
						 break;
						}
					}
				endcase;

#ifdef W32200
			 case PreDecr:
			 case PreIncr:
			 case PostDecr:
			 case PostIncr:
				ad_used = GetAdUsedId(an_id,0);
				for(pa = sub_head; pa; pa = pa->sub_next)
					{ada = pa->laddr;
					 if(ada == ad_used)
					 	{adb = pa->passign->laddr;
					 	 newan_id = 
							GetAdChgRegAInc(ty,
							an_id,GetAdRegA(adb),0);
					 	 break;
						}
					}
				endcase;
			 case IndexRegDisp:
			 case IndexRegScaling:
				ad_used = GetAdUsedId(an_id,1);
				for(pa = sub_head; pa; pa = pa->sub_next)
					{ada = pa->laddr;
					 if(ada == ad_used)
					 	{adb = pa->passign->laddr;
					 	 newan_id = 
							GetAdChgRegBInc(ty,
							an_id,GetAdRegA(adb),0);
					 	 break;
						}
					}
				endcase;
#endif

			 default:
				fatal("naqmerge: unknown mode (%u).\n",mode);
				endcase;
			} /* END OF switch(mode = GetAdMode(an_id)) */

		 if(newan_id == NULL)
			continue;

		 if(DBdebug(0,XGRA))
			prbefore(GetTxPrevNode(tn_id),GetTxNextNode(tn_id),
				0,"gra: naqmerge");

		 PutTxOperandAd(tn_id,operand,newan_id); /* Use new address. */

		 if(DBdebug(0,XGRA))
			prafter(GetTxPrevNode(tn_id),GetTxNextNode(tn_id),0);

		} /* END OF for(ALLOP(operand)) */
	} /* END OF for(tn_id=GetTxPrevNode(skipprof(GetTxNextNode((TN_Id) ...*/

 return;
}
	STATIC void
insertmoves(var_head, ldavail, NVectors)
struct ldent *var_head;		/* Set of variables. */
unsigned int ldavail;
unsigned int NVectors;

{
 TN_Id after;
 AN_Id an_id;			/* Identifier of an address node.	*/
 TN_Id before;
 boolean changed;
 STATIC boolean fromregs();
 STATIC struct ldent *get_submove();
 unsigned int gmask;
 unsigned long int lastlive[NVECTORS];	/* Last live data.	*/
 STATIC void killmovesrc();
 unsigned int op_code;
 extern void prafter();		/* Prints nodes after a change.	*/
 extern void prbefore();	/* Prints nodes before a change.	*/
 extern TN_Id skipprof();	/* Skips profiling code, if any.	*/
 struct ldent *sub;
 STATIC boolean toregs();
 register TN_Id tn_id;
 extern void updlive();		/* Updates live information; in optutil.c. */

 sub = get_submove(var_head, &gmask);
 if( sub == NULL )		/* Nothing to move */
	return;			/* we're done. */

 tn_id = skipprof(GetTxNextNode((TN_Id) NULL));	/* Function entry.	*/
 an_id = GetTxOperandAd(tn_id,DESTINATION);	/* If a stack increment, */
 if((GetTxOpCodeX(tn_id) == G_ADD3) && 
		IsAdCPUReg(an_id) &&
		(GetAdRegA(an_id) == CSP))
	tn_id = GetTxNextNode(tn_id);		/* skip to next instruction. */

 tn_id = GetTxPrevNode(tn_id);
 before = GetTxPrevNode(tn_id);
 after = GetTxNextNode(tn_id);
						/* Move args, statics, and */
						/* globals to registers. */
 if(toregs(tn_id,sub,gmask))
 	{GetTxLive(tn_id,lastlive,NVectors);		/* Get live data.	*/
	 updlive(before,after,lastlive,NVectors);	/* Propagate it. */
	 if(DBdebug(0,XGRA))
		{prbefore(GetTxPrevNode(tn_id),
			GetTxNextNode(tn_id),0,
			"insertmoves: on function entry");
		 prafter(before,after,0);
		}
	}

						/* If no STATLOC or EXT assigned,*/
						/* we're done.	*/
 if((gmask & ((unsigned)SNAQ | (unsigned)ENAQ | (unsigned)SENAQ)) == 0)
	return;

 for(ALLNSKIP(tn_id))				/* Scan the function.	*/
	{if(IsTxAnyCall(tn_id))			/* If a call, and we  have	*/
						/* allocated globals to regs	*/
		{before = GetTxPrevNode(tn_id);
		 after = GetTxNextNode(tn_id);
						/* Unload registers for globals. */
		 changed = fromregs(tn_id,sub,
				(unsigned)ENAQ | (unsigned)SENAQ | (unsigned)SNAQ);
						/* Load globals back to registers */
		 if(	toregs(tn_id,sub,
					(unsigned)ENAQ | 
					(unsigned)SENAQ | 
					(unsigned)SNAQ
			)
			|| changed
		   )
			{GetTxLive(tn_id,lastlive,NVectors);
			 updlive(before,after,lastlive,NVectors);
			 if(DBdebug(0,XGRA))
				{prbefore(GetTxPrevNode(tn_id),
					GetTxNextNode(tn_id),0,
					"insertmoves: for calls");
				 prafter(before,after,0);
				}
			}
		} /* END OF if(IsTxAnyCall(tn_id)) */
						/* If return,	*/
	 else if(	((op_code = GetTxOpCodeX(tn_id)) == IRET)
		    ||	(op_code == RESTORE))
		{before = GetTxPrevNode(tn_id);
		 after = GetTxNextNode(tn_id);
						/* restore  statics & globals. */
		 if(	fromregs(tn_id,sub,
					(unsigned)SNAQ |
					(unsigned)ENAQ | 
					(unsigned)SENAQ
			)
		   ) 
			{GetTxLive(tn_id,lastlive,NVectors);
			 updlive(before,after,lastlive,NVectors);
			 killmovesrc(before,after,ldavail,NVectors);
			 if(DBdebug(0,XGRA))
				{prbefore(GetTxPrevNode(tn_id),
					GetTxNextNode(tn_id),0,
					"insertmoves: for restore");
				 prafter(before,after,0);
				}
			}
		} /* END OF else if((op_code = GetTxOpCodeX(tn_id)) == ... */
	} /* END OF for(ALLNSKIP(tn_id)) */

 return;
}
	STATIC struct ldent *		/* Get the list of variables	*/
get_submove(var_head,gmp)		/* assigned to registers that	*/
					/* needs explicit moves inserted*/
struct ldent *var_head;			/* var list head	*/
unsigned *gmp;				/* mask of all gnaqtypes in list*/

{
 unsigned mask;				/* gnaqtype accumulator	*/
 register struct ldent *pa;		/* points to one var assigned to reg*/
 struct ldent *head;			/* head of list.	*/
 struct ldent *tail;			/* tail of list.	*/
 AN_Id var;				/* Variable assigned to reg */

 mask = 0;
 head = tail = NULL;
 for(pa = var_head; pa != NULL; pa = pa->var_next)
	{if(pa->passign == NULL)	/* Is it assigned?	*/
		continue;		/* No.	*/
	 if((pa->var_class & (VC_ISTK|VC_STAT|VC_STATEXT|VC_EXTDEF)) == 0)
		continue;		/* Not a var of interest */
	 var = pa->laddr;		/* If NAQ, is it */
	 if(IsAdNAQ(var) && !IsAdArg(var)) /* an arg? */
		continue;		/* No. */
					/* Otherwise, append to list.	*/
	 if(head == NULL)
		head = pa;
	 else
		tail->sub_next = pa;
	 pa->sub_next = NULL;
	 tail = pa;
					/* Note this gnaqtype.	*/
	 mask |= (unsigned)GetAdGnaqType(pa->laddr);
	}
 *gmp = mask;
 return( head );
}
	STATIC boolean
toregs(tn_id,sub,gtype)	/* Move GNAQs to Registers.	*/
TN_Id tn_id;			/* Put G_MOV's after this one.	*/
struct ldent *sub;		/* The set of allocated vars.	*/
unsigned int gtype;		/* Type of GNAQ to move. */

{AN_Id var;			/* Name of GNAQ-variable.	*/
 extern boolean IsDeadAd();	/* TRUE if address dead.	*/
 extern OperandType NAQ_type();	/* Guesses type of NAQ offered; in gra.c. */
 AN_Id Register;		/* Register variable goes in.	*/
 boolean generated;		/* TRUE if we generated any instructions. */
 unsigned short movop;		/* MOV or MMOV	*/
 register TN_Id new_tn_id;	/* Identifier of new text node.	*/
 register struct ldent *pa;	/* Scans sub_head list.	*/
 OperandType type;		/* Type of an operand.	*/

 generated = FALSE;
 for(pa=sub; pa != NULL; pa = pa->sub_next)	/* Scan live-dead table for */
						/* vars assigned to regs. */
	{var = pa->laddr;			/* Var assigned to register.*/
	 if((((unsigned)GetAdGnaqType(var)) & gtype) == 0)
						/* If not the right type, */
		continue;			/* forget it.	*/
	 Register = pa->passign->laddr;		/* Register assigned.	*/
	 if(IsDeadAd(Register,tn_id))		/* If register is dead,	*/
		continue;			/* don't bother to load it. */
	 type = NAQ_type(var);			/* Guess type of the var. */
	 movop = IsAdFP(var) ? G_MMOV : G_MOV;
	 new_tn_id = MakeTxNodeAfter(tn_id,movop);
	 PutTxOperandAd(new_tn_id,THIRD_SRC,var);
	 PutTxOperandType(new_tn_id,THIRD_SRC,type);
	 PutTxOperandAd(new_tn_id,DESTINATION,Register);
	 PutTxOperandType(new_tn_id,DESTINATION,type);
	 generated = TRUE;
	} 

 return(generated);
}
	STATIC boolean
fromregs(tn_id,sub,gtype)	/* Move Registers to memory.	*/
TN_Id tn_id;			/* Put G_MOV's before this one.	*/
struct ldent *sub;		/* The set of vars to consider.	*/
unsigned int gtype;		/* Type of GNAQ to unload.	*/

{AN_Id var;			/* Name of variable.	*/
 extern OperandType NAQ_type();	/* Guesses type of NAQ offered; in gra.c. */
 AN_Id Register;		/* Register variable goes in.	*/
 boolean generated;		/* TRUE if we generated any instructions. */
 unsigned short movop;		/* MOV or MMOV	*/
 register TN_Id new_tn_id;	/* Identifier of new text node.	*/
 register struct ldent *pa;
 OperandType type;		/* Type of an operand.	*/

 generated = FALSE;
 for(pa = sub; pa != NULL; pa = pa->sub_next)	/* Scans ldtab for */
						/* variables assigned to regs. */
	{var= pa->laddr;			/* Variable assigned to register.*/
	 if((((unsigned)GetAdGnaqType(var)) & gtype) == 0)
						/* If not right type, */
		continue;			/* forget it.	*/
	 Register = pa->passign->laddr;		/* Register assigned.	*/
	 type = NAQ_type(var);			/* Guess the type of GNAQ. */
	 movop = IsAdFP(var) ? G_MMOV : G_MOV;
	 new_tn_id = MakeTxNodeBefore(tn_id,movop);	/*Need G_MMOV.*/
	 PutTxOperandAd(new_tn_id,DESTINATION,var);
	 PutTxOperandType(new_tn_id,DESTINATION,type);
	 PutTxOperandAd(new_tn_id,THIRD_SRC,Register);
	 PutTxOperandType(new_tn_id,THIRD_SRC,type);
	 generated = TRUE;
	} 

 return(generated);
}
	STATIC void
killmovesrc(before,after,ldavail,NVectors)
					/* kill off the src of moves. */
					/* this routine should be called */
					/* after inserting moves to restore */
					/* externs from registers at exit */
					/* points. */
TN_Id before;				/* text node before the moves. */
TN_Id after;				/* text node after "ret" or "RESTORE". */
unsigned ldavail;			/* max GNAQ bit within tracking range. */
unsigned NVectors;			/* max GNAQ word within tracking range. */

{
 unsigned long dead[NVECTORS];		/* killed off registers */
 int i;
 unsigned ld;				/* GNAQ index. */
 unsigned long live[NVECTORS];		/* live vector for instruction. */
 AN_Id src;				/* Source operand of a G_MOV. */
 unsigned short op;			/* Opcode of instruction. */
 TN_Id p;				/* Scans text nodes. */

					/* Initialize dead vector. */
 for(i = 0; i < NVectors; ++i)
	dead[i] = 0;
					/* Traverse between before & after. */
 for(p = GetTxNextNode(before); p != after; p = GetTxNextNode(p))
	{
	 GetTxLive(p,live,NVectors);		/* get old live vector. */
	 op = GetTxOpCodeX(p);
	 src = GetTxOperandAd(p,THIRD_SRC);
	 if(IsOpGMove(op) && IsAdCPUReg(src))	/* current constraint */
		{
		 ld = GetAdAddrIndex(src);
		 if(ld <= ldavail)		/* update dead vector */
			 dead[ld/(sizeof(unsigned long)*B_P_BYTE)]
				|= (1 << (ld % (sizeof(unsigned long)*B_P_BYTE)));
		}
	 for(i = 0; i < NVectors; ++i)		/* kill off dead ones. */
		live[i] &= ~dead[i];
	 PutTxLive(p,live,NVectors);		/* update liveness */
	}
}
	STATIC struct ldent *
get_sub(var_head,classes,mask)	/* Return the head pointer to */
				/* a NULL terminated linked list of a subset */
				/* of the var_head list bounded by pfirst and */
				/* plast and with one's in the mask. */
struct ldent *var_head;		/* Head of variable list. */
unsigned long int classes;	/* OR of variable classes. */
unsigned long int mask[];	/* Bit vector with one for each candidate */
				/* to be accepted. */

{register struct ldent *pvar;	/* Pointer for scanning variable list. */
 struct ldent *plast;		/* Pointer to last entry in subset list. */
 struct ldent *sub_head;	/* Head pointer for subset list. */
 
 sub_head = NULL;
 plast = NULL;
						/* Scan candidates of */
						/* variable list.*/
 for(pvar = var_head; pvar; pvar = pvar->var_next)
						/* Skip if not in class. */
	{if((pvar->var_class & classes) == 0)
		continue;
						/* Skip if mask bit not set. */
	 if(ld_get_bit(mask,pvar) == 0)
		continue;
	 if(sub_head == NULL)			/* Add to end of list.*/
		sub_head = pvar;
	 else
		plast->sub_next = pvar;
	 pvar->sub_next = NULL;
	 plast = pvar;
	}
 return(sub_head);
}
	STATIC void
gra_funcprint(title,NVectors,var_head) /* Print instr with live and sets. */
char *title;			/* Pointer to title of printout.	*/
unsigned int NVectors;		/* Number of words in vecctors. */
struct ldent *var_head;		/* Head pointer for variables list. */

{
 extern int fprinstx();		/* Prints instructions; in Mach. Dep.	*/
 STATIC void gra_bitsprint();	/* Prints variables in a mask. */
 unsigned long int live[NVECTORS]; /* Quantities live at an instruction. */
 int ntot;			/* Characters printed. */
 unsigned long int set[NVECTORS]; /* Quantities set by an instruction. */
 extern void sets();		/* Gets sets data for a text-node.	*/
 extern void textaudit();	/* Audits text nodes.	*/
 TN_Id tn_id;			/* TN_Id for scanning text. */

#ifdef DEBUG

 printf("\n%c		***** %s *****\n",
		ComChar,title);
 textaudit(title);

 for(tn_id = GetTxNextNode((TN_Id) NULL); tn_id; tn_id = GetTxNextNode(tn_id))
	{ntot = fprinstx(stdout,-2,tn_id);
	 while(ntot++ < 8 + 11 + 4 * 12)
		putchar(SPACE);
	 if((!IsTxLabel(tn_id)))
		{printf("vl:");
		 GetTxLive(tn_id,live,NVectors);	/* Get live bits. */
		 gra_bitsprint(var_head,live);		/* Print bits.*/
	
	 	 (void) printf(" vs:");
	 	 sets(tn_id,set,NVectors);		/* Get sets bits. */
		 gra_bitsprint(var_head,set);		/* Print bits. */
		}

 	 putchar(NEWLINE);
	}
#endif /* DEBUG */
 return;
}
	STATIC void
gra_ldprint(title,ldavail,NVectors,var_head,misoper,liveset,active,assigned)
				/* Print GRA information. */
char *title;			/* Title giving location of call. */
unsigned int ldavail;		/* Lowest unused ldtab entry. */
unsigned int NVectors;		/* Number of words in vectors. */
struct ldent *var_head;		/* Head pointer for variable list. */
unsigned long int misoper[];	/* Variables that are operands of MIS instr. */
unsigned long int liveset[];	/* Variables that are live when set. */
unsigned long int active[];	/* Vars that are live when set plus svd regs. */
unsigned long int assigned[];	/* Vars that are assigned to registers. */
	
{extern OperandType NAQ_type();	/* Returns the type of a NAQ for printing. */
 extern void fatal();		/* Handles fatal errors. */
 extern unsigned int praddr();	/* Prints an address. */
 struct ldent *pvar;		/* Pointer for scanning variable list. */
 struct ldent *pvar2;		/* Pointer for scanning conflict vectors. */

 (void) printf("\n%cgra: gra_ldprint: %s\n",
	ComChar,title);
 (void) printf("%cgra: gra_ldprint: ldavail=%d\n",
	ComChar,ldavail);
 (void) printf("%cgra: gra_ldprint: NVectors=%d\n",
	ComChar,NVectors);
 (void) printf("%cgra: gra_ldprint: data for variables list\n",
	ComChar);
 for(pvar = var_head; pvar; pvar = pvar->var_next)
	{(void) printf("%c\t", ComChar);
	 (void) printf("ld=%-2d ",pvar->index);
	 if(ld_get_bit(misoper,pvar) != 0)
		(void) printf("   misoper ");
	 else
		(void) printf("notmisoper ");
	 if(ld_get_bit(liveset,pvar) != 0)
		(void) printf("   liveset ");
	 else
		(void) printf("notliveset ");
	 if(ld_get_bit(active,pvar) != 0)
		(void) printf("   active ");
	 else
		(void) printf("notactive ");
	 (void) praddr(pvar->laddr,NAQ_type(pvar->laddr),stdout);
	 (void) printf(": ");
	 if(ld_get_bit(assigned,pvar) != 0)
		{if(pvar->passign == NULL)
			fatal("gra_ldprint: inconsistent assignment data\n");
		 (void) printf("assignment=");
		 (void) praddr(pvar->passign->laddr,
			NAQ_type(pvar->passign->laddr),stdout);
		}
	 else
		{if(pvar->passign != NULL)
			fatal("gra_ldprint: inconsistent assignment data\n");
		 (void) printf("conflicts=");
	 	 for(pvar2 = var_head; pvar2; pvar2 = pvar2->var_next)
			if(ld_get_bit(pvar->conflicts,pvar2) != 0)
				{(void) praddr(pvar2->laddr,
					NAQ_type(pvar2->laddr),stdout);
			 	 (void)printf(" ");
				}
		}
	 putchar(NEWLINE);
	}
 return;
}


	STATIC void
gra_bitsprint(var_head,mask)
struct ldent *var_head;		/* Head pointer of variable list. */
unsigned long int mask[];	/* Mask indicating bits to print. */

{extern OperandType NAQ_type();	/* Returns the type of a NAQ for printing. */
 extern unsigned int praddr();	/* Prints addresses. */
 struct ldent *pvar;		/* Pointer for scanning variable list. */
 
 for(pvar = var_head; pvar; pvar = pvar->var_next)
	{if(ld_get_bit(mask,pvar) == 0)
		continue;
	 (void) praddr(pvar->laddr,NAQ_type(pvar->laddr),stdout);
	 (void) printf(" ");
	}
}
	OperandType
NAQ_type(an_id)			/* Returns the type of a NAQ for printing. */
				/* The type returned by this function is */
				/* only guaranteed to be sufficiently correct */
				/* for printing.  There is insufficient data */
				/* available to provide a more accurate type. */

AN_Id an_id;			/* AN_Id of the NAQ. */

{
 extern int TySize();		/* Returns size in bytes of a type */
 extern void fatal();		/* Handles fatal errors.	*/
 unsigned short int length;	/* Length, in bytes, of the NAQ. */

 if(!IsAdAddrIndex(an_id))
	fatal("NAQ_Type: address is not a GNAQ.\n");
 if(IsAdImmediate(an_id))
	return(Tword);		/* Type doesn't matter for printing immed. */
 if(IsAdStatCont(an_id))
	return(Tnone);		/* Status bits have no type. */
 if(IsAdCPUReg(an_id))
	return(Tword);		/* Type doesn't matter for printing regs. */
 if(IsAdMAUReg(an_id))
	return(Tdblext);	/* Type doesn't matter for printing regs. */
 length = GetAdGNAQSize(an_id);
 if(IsAdFP(an_id))
	{if(length == TySize(Tsingle))
		return(Tsingle);
	 else if(length == TySize(Tdouble))
		return(Tdouble);
	 else if(length == TySize(Tdblext))
		return(Tdblext);
	 else
		fatal("NAQ_type: unrecognized FP length (%d).\n",length);
	 /*NOTREACHED*/
	}
 else
	{if(length == TySize(Tbyte))
		return(Tbyte);
	 else if(length == TySize(Thalf))
		return(Thalf);
	 else if(length == TySize(Tword))
		return(Tword);
	 else
		fatal("NAQ_type: unrecognized integer length (%d).\n",length);
	 /*NOTREACHED*/
	}
 /*NOTREACHED*/
 return(Tnone);
}
