/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:m32/debug.c	1.10"

#include	<stdio.h>
#include	<string.h>
#include	"defs.h"
#include	"debug.h"
#include	"ANodeTypes.h"
#include	"OpTabTypes.h"
#include	"RegId.h"
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


static long int Elevel = {0};	/* Debugging level from command line.	*/
static unsigned long int Emask = {0};	/* Debugging mask from command line.*/
static char *Ename = {""};	/* Debugging name from command line.	*/
static unsigned long int Xmask = {0};	/* Debugging mask from command line.*/
static char *Xname = {""};	/* Debugging name from command line.	*/
static unsigned long int Xpmask = {0};	/* Peephole debugging mask from command line.*/

	/* Private functions */
STATIC void windowprint();
STATIC void allbits();


	void
DBinit(mask,level,name)		/* Initialize debugging.	*/
unsigned long int mask;		/* Debugging mask: bits select function. */
long int level;			/* Debugging level: higher gives more.	*/
char *name;			/* Name of function to debug.	*/

{extern long int Elevel;	/* Debugging level from command line.	*/
 extern unsigned long int Emask;	/* Debugging mask from command line.*/
 extern char *Ename;		/* Debugging name from command line.	*/
 extern FN_Id FuncId;		/* Function being optimized.	*/

 Elevel = level;
 Emask = mask;					/* Keep these around.	*/
 Ename = name;
 FuncId = (FN_Id) NULL;			/* Don't know function yet. */

 return;
}
	unsigned long
GetPmask(pno)
int pno;
{
	switch(pno){
	case 100:	return((unsigned long)PEEP100);
	case 101:	return((unsigned long)PEEP101);
	case 102:	return((unsigned long)PEEP102);
	case 103:	return((unsigned long)PEEP103);
	case 104:	return((unsigned long)PEEP104);
	case 107:	return((unsigned long)PEEP107);
	case 109:	return((unsigned long)PEEP109);
	case 111:	return((unsigned long)PEEP111);
	case 200:	return((unsigned long)PEEP200);
	case 202:	return((unsigned long)PEEP202);
	case 203:	return((unsigned long)PEEP203);
	case 204:	return((unsigned long)PEEP204);
	case 207:	return((unsigned long)PEEP207);
	case 209:	return((unsigned long)PEEP209);
	case 210:	return((unsigned long)PEEP210);
	case 220:	return((unsigned long)PEEP220);
	case 221:	return((unsigned long)PEEP221);
	case 222:	return((unsigned long)PEEP222);
	case 227:	return((unsigned long)PEEP227);
	case 228:	return((unsigned long)PEEP228);
	case 241:	return((unsigned long)PEEP241);
	case 242:	return((unsigned long)PEEP242);
	case 244:	return((unsigned long)PEEP244);
	case 245:	return((unsigned long)PEEP245);
	case 246:	return((unsigned long)PEEP246);
	case 300:	return((unsigned long)PEEP300);
	}
	return((unsigned long)0);
}
	boolean
DBDebug(level,mask)		/* TRUE if debugging level and mask satisfied.*/
int level;			/* Level caller wants debugging at.	*/
unsigned long int mask;		/* Mask to satisfy if debugging wanted.	*/

{extern long int Elevel;	/* Debugging level program is running under. */
 extern unsigned long int Emask;	/* Mask program is running under.*/
 extern char *Ename;		/* Name of function to be debugged.	*/
 char *name;			/* Function name.	*/
 extern FN_Id FuncId;		/* FN_Id of function optimized.*/

 if(Elevel < level)				/* Satisfy level?	*/
	{if(Elevel >= 0)			/* If Level non-negative, */
		return(FALSE);			/* No.	*/
	 else if((level == 1) || (level == 2))	/* If Level negative, */
		return(FALSE);			/* skip before and after. */
	 else if(-Elevel < level)
		return(FALSE);
	}
 if((mask & Emask) == 0)			/* Satisfy mask?	*/
	return(FALSE);				/* No.	*/
 if(*Ename == EOS)				/* If no user name,	*/
	return(TRUE);				/* do all functions.	*/
 if(FuncId == (FN_Id) NULL)
	return(FALSE);				/* It should have been there. */
 name = GetAdExpression(Tbyte,GetFnName(FuncId));
 return((strcmp(Ename,name) == 0) ? TRUE : FALSE);
}
	void
fnprint(title)			/* Prints function node list. */
char *title;			/* title indicating where called from */

{FN_Id fn_id;			/* function node id */
 extern void funcaudit();	/* Audits function nodes */

 funcaudit(title);			/* Audit function nodes. */
				/* print title */
 printf("\n%c	************ %s ************\n",ComChar,title);
 printf("%c Function	calls def'd instr regs  local ",ComChar);
 printf("lib  sjmp blkb cand conv ndbl PA\n%c\n",ComChar);
 				/* print functions */
 for(fn_id = GetFnNextNode((FN_Id) NULL); fn_id != (FN_Id) NULL;
		fn_id = GetFnNextNode(fn_id))
 	{printf("%c %-14s",ComChar,GetAdExpression(Tbyte,GetFnName(fn_id)));
	 printf("%-6d",(int)GetFnCalls(fn_id));
	 printf("%c     ",IsFnDefined(fn_id) ? 'T' : 'F');
	 if(!IsFnDefined(fn_id)) 
		{printf("\n");
		 continue;
		}
	 printf("%-6d",(int)GetFnInstructions(fn_id));
	 printf("%-6d",GetFnNumReg(fn_id));
	 printf("%-6d",(int)GetFnLocSz(fn_id));
	 printf("%c    ",IsFnLibrary(fn_id) ? 'T' : 'F');
	 printf("%c    ",IsFnSetjmp(fn_id) ? 'T' : 'F');
	 printf("%c    ",IsFnBlackBox(fn_id) ? 'T' : 'F');
	 printf("%c    ",IsFnCandidate(fn_id) ? 'T' : 'F');
	 printf("%c    ",IsFnMISconvert(fn_id) ? 'T' : 'F');
	 printf("%c    ",IsFnNODBL(fn_id) ? 'T' : 'F');
	 printf("%c\n",IsFnPAlias(fn_id) ? 'T' : 'F');
	}
 printf("\n");
 printf("%c Function	cloc  limit expan\n",ComChar);
 printf("%c\n",ComChar);
 				/* print functions */
 for(fn_id = GetFnNextNode((FN_Id) NULL); fn_id != (FN_Id) NULL;
		fn_id = GetFnNextNode(fn_id))
 	{if(!IsFnCandidate(fn_id))
		{printf("\n");
		 continue;
		}
	 printf("# %-14s",GetAdExpression(Tbyte,GetFnName(fn_id)));
	 printf("%-6d",(int)GetFnCandLocSz(fn_id));
	 printf("%-6d",(int)GetFnExpansionLimit(fn_id));
	 printf("%-6d",(int)GetFnExpansions(fn_id));
	 printf("\n");
	}
 printf("\n");
}

	void
ldbits(outfp,words,number)	/* Print live-dead bits.	*/
FILE *outfp;			/* Stream on which to write output.	*/
unsigned long int words[];	/* Words containing the live-dead bits. */
unsigned int number;		/* Number of bits of live-dead data.	*/

{register unsigned int address;	/* Address bit under investigation.	*/
 AN_Id an_id;			/* AN_Id of address node to print.	*/
 extern void fatal();		/* Handles fatal errors; in common.	*/
 /*extern int fprintf();	** Prints to stream; in C(3) library.	*/
 AN_Mode mode;			/* Mode of an address.	*/
 extern unsigned int praddr();	/* Prints address.	*/
 AN_GnaqType type;		/* Type of GNAQ an address node is.	*/

#ifdef DEBUG
						/* Find address node for */
						/* each live-dead address. */
 for(address = 0; address < number; address++)
	{if(!(words[address / WDSIZE]
			& (1 << (address % WDSIZE))))
		continue;			/* This one not used.	*/
	 if((an_id = GetAdId(address)) == NULL)	/* Convert index to AN_Id. */
		fatal("ldbits: couldn't find %u.\n",address);
	 putc(SPACE,outfp);
	 switch((type = GetAdGnaqType(an_id)))	/* Tell GNAQ type.	*/
		{case Other:			/* Not  a GNAQ.	*/
			fprintf(stdout,"O:");
			endcase;
		 case NAQ:
			endcase;
		 case SNAQ:
			fprintf(stdout,"S:");
			endcase;
		 case ENAQ:
			fprintf(stdout,"E:");
			endcase;
		 case SENAQ:
			fprintf(stdout,"SE:");
			endcase;
		 case SV:
			fprintf(stdout,"SV:");
			endcase;
		 default:
			fatal("ldbits: unknown GNAQ type (0x%x).\n",type);
			endcase;
		}
	 switch(mode = GetAdMode(an_id))
		{case Immediate:
			endcase;
		 case Absolute:
			endcase;
		 case AbsDef:
			endcase;
		 case CPUReg:
			endcase;
		 case DispDef:
			endcase;
		 case PreDecr:
			endcase;
		 case PreIncr:
			endcase;
		 case PostDecr:
			endcase;
		 case PostIncr:
			endcase;
		 case Disp:
		 case IndexRegDisp:
			if(IsAdAddrIndex(an_id) &&
					IsAdNumber(an_id) &&
					IsAdGNAQSize(an_id))
				{fprintf(outfp,"%d-",
						GetAdNumber(Tbyte,an_id) -
						GetAdGNAQSize(an_id) + 1);
				}
			endcase;
		 case IndexRegScaling:
			endcase;
		 case MAUReg:
			endcase;
		 case StatCont:
			endcase;
		 default:
			fatal("ldbits: unknown mode (%d).\n",mode);
			endcase;
		} /* END OF switch(mode = GetAdMode(an_id)) */
	 (void) praddr(an_id,Tbyte,outfp);
	}
#endif /*DEBUG*/
 return;
}


	void
funcprint(outfp,title,nsupple)	/* Print instr with alluses and allsets. */
FILE *outfp;			/* Output stream.	*/
char *title;			/* Pointer to title of printout.	*/
int nsupple;			/* Type of supplementary information.	*/

{
#ifdef DEBUG
 extern FN_Id FuncId;
 extern void addraudit();	/* Audits addr nodes.	*/
 extern void textaudit();	/* Audits text nodes.	*/
 char *expression;		/* Expression part of address. */

 if(FuncId != NULL)
	{expression = GetAdExpression(Tbyte,GetFnName(FuncId));
	 if((*Ename != NULL) &&  (strcmp(expression,Ename) != 0))
		return;
	}

 putc(NEWLINE,outfp);
 fprintf(outfp,"%c		***** %s *****\n",
		ComChar,title);
 addraudit(title);
 textaudit(title);
 windowprint(outfp,(TN_Id) NULL,(TN_Id) NULL,nsupple);
 putc(NEWLINE,outfp);
#endif /* DEBUG */
 return;
}
	

	STATIC void
windowprint(outfp,p1,p2,nsupple)	/* Print instr window with supple info*/
FILE *outfp;			/* Output stream.	*/
TN_Id p1;			/* TN_Id of instr before window.	*/
TN_Id p2;			/* TN_Id of instr after window.	*/
int nsupple;			/* Type of supplementary information.	*/

{
#ifdef DEBUG
 extern void instructprint();	/* Prints instruction; in this file.	*/
 TN_Id pn;

 for(pn = GetTxNextNode(p1); pn != p2; pn = GetTxNextNode(pn))
 	instructprint(outfp,pn,nsupple);
#endif /*DEBUG*/
 return;
}
	void
instructprint(outfp,pn,nsupple) /* Print instruction with supplementary info */
FILE *outfp; 		/* Output stream.	*/
TN_Id pn;			/* Instruction node.	*/
int nsupple;			/* Type of supplementary information
						== 0 none
						== 1 alluses and allsets
						== 2 live/dead */

{
#ifdef DEBUG
 extern unsigned int GnaqListSize;	/* Current size of GNAQ list.	*/
 unsigned int NVectors;		/* Number of vectors to use.	*/
 extern void fatal();		/* Handles fatal errors; in common. */
 extern int fprinstx();		/* Prints instructions; in Mach. Dep.	*/
 int ntot;
 unsigned long int templd[NVECTORS];	/* Place for live-dead bits.	*/

 NVectors = (GnaqListSize > (NVECTORS * sizeof(unsigned int) * B_P_BYTE)) ?
	(NVECTORS * sizeof(unsigned int) * B_P_BYTE) : GnaqListSize;
 NVectors = (NVectors + (sizeof(unsigned int) * B_P_BYTE) - 1) /
	(sizeof(unsigned int) * B_P_BYTE);

 ntot = fprinstx(outfp,-2,pn);
 while(ntot++ < 8 + 11 + 4 * 12)
	putc(SPACE,outfp);
 if(pn->mark == 1)
	fprintf(outfp," <M> ");
 switch(nsupple)
	{case 0:
		endcase;
	 case 1:
		allbits(outfp,pn,NVectors);
		endcase;
	 case 2: 
		if((!IsTxLabel(pn)) && (GetTxOpCodeX(pn) != RET))
			{fprintf(outfp,"lv:");
			 GetTxLive(pn,templd,NVectors);	/* Get live bits. */
			 ldbits(outfp,templd,
				NVectors*sizeof(unsigned int)*B_P_BYTE);
			}
		endcase;
	 default:
		fatal("instructprint: bad nsupple (%d).\n",nsupple);
		endcase;
	}
 putc(NEWLINE,outfp);
#endif /*DEBUG*/
 return;
}
	STATIC void
allbits(outfp,pn,NVectors)	/*Print alluses and allsets bits.	*/
FILE *outfp;			/* Output stream.	*/
TN_Id pn;			/* Instruction node.	*/
unsigned int NVectors;		/* Number of vectors to use.	*/

{
#ifdef DEBUG
 extern void sets();		/* Determines what is set by instruction. */
 extern void uses();		/* Determines what is used by instruction. */
 unsigned long int bitvectors[NVECTORS];

 uses(pn,bitvectors,NVectors);			/* Get uses bits.	*/
 (void) fprintf(outfp, "au:");
 ldbits(outfp,bitvectors,sizeof(unsigned int)*B_P_BYTE*NVectors);
 sets(pn,bitvectors,NVectors);			/* Get sets bits.	*/
 (void) fprintf(outfp," as:");
 ldbits(outfp,bitvectors,sizeof(unsigned int)*B_P_BYTE*NVectors);
#endif /* DEBUG */
 return;
}
	void
peepchange(ident,peeppass,peepno) 	/* Local version of wchange.	*/
char *ident;			/* Optimization identification.	*/
unsigned int peeppass;		/* Peephole pass number.	*/
unsigned int peepno;		/* Peephole optimization number.	*/

{extern unsigned long int Emask;	/* Debug mask; from -x flag.	*/
 extern TN_Id paft;
 extern TN_Id pbef;
 TN_Id pn;

 if(!DBdebug(0,XPEEP))
	return;
 if(((peeppass == 1) && (Emask & XPEEP_1)) ||
		((peeppass == 2) && (Emask & XPEEP_2)) ||
		((peeppass == 3) && (Emask & XPEEP_3)))
	{printf("%cWINDOW after ",ComChar);
	 for(pn = pbef; pn != (TN_Id) NULL; pn = GetTxPrevNode(pn))
		{if(IsTxLabel(pn))
			{printf("%s",GetAdExpression(Tbyte,GetTxOperandAd(pn,0)));
			 break;
			}
		}
	 if(pn == (TN_Id) NULL)
		printf("beginning of function");
	 printf(": peeppass=%d peepno=%d  %s\n", 
			peeppass,peepno,ident);
	 windowprint( stdout,pbef,paft,2 );
	}
 return;
}
	void
prbefore(p1,p2,nsupple,ident)
TN_Id p1;			/* TN_Id of instr before window.	*/
TN_Id p2;			/* TN_Id of instr after window.	*/
int nsupple;			/* Type of supplementary information.	*/
char *ident;			/* String identifying the opimization.	*/

{
 TN_Id pn;

#ifdef DEBUG
 printf("%cWINDOW after ",ComChar);
 for(pn = p1; pn != NULL; pn = GetTxPrevNode(pn))
	{if(IsTxLabel(pn))
		{printf("%s",GetAdExpression(Tbyte,GetTxOperandAd(pn,0)));
		 break;
		}
	}
 if(pn == NULL)
	printf("beginning of function");
 printf(": %s\n", ident);
 windowprint(stdout,p1,p2,nsupple);
#endif /*DEBUG*/
 return;
}
	void
prafter(p1,p2,nsupple)
TN_Id p1;			/* TN_Id of instr before window.	*/
TN_Id p2;			/* TN_Id of instr after window.	*/
int nsupple;			/* Type of supplementary information.	*/

{

#ifdef DEBUG

 if(GetTxNextNode(p1) == p2)
	printf("%cDELETED\n",ComChar);
 else
	{printf("%cCHANGED TO:\n",ComChar);
	 windowprint( stdout, p1, p2, nsupple );
	}
#endif /*DEBUG*/
 return;
}
	void
Xinit(mask,pmask,name)		/* Initialize debugging.	*/
unsigned long int mask;		/* Debugging mask: bits select function. */
unsigned long int pmask;	/* Peephole debugging mask: bits select function. */
char *name;			/* Name of function to debug.	*/

{extern unsigned long int Xmask;	/* Debugging mask from command line.*/
 extern unsigned long int Xpmask;	/* Peephole debugging mask from command line. */
 extern char *Xname;		/* Debugging name from command line.	*/

 Xmask = mask;					/* Keep these around.	*/
 Xpmask = pmask;
 Xname = name;

 return;
}
	boolean
XSkip(mask)			/* FALSE if debugging mask satisfied.*/
unsigned long mask;		/* Mask to satisfy if debugging wanted.	*/

{extern unsigned long int Xmask;	/* Mask program is running under.*/
 extern char *Xname;		/* Name of function to be debugged.	*/
 char *name;			/* Function name.	*/
 extern FN_Id FuncId;		/* FN_Id of function optimized.*/

 if((mask & Xmask) != 0)			/* Satisfy mask?	*/
	{if(*Xname == EOS)			/* If no user name,	*/
		return(TRUE);			/* do all functions.	*/
	 if(FuncId == (FN_Id) NULL)
		return(TRUE);			/* It should have been there. */
	 name = GetAdExpression(Tbyte,GetFnName(FuncId));
	 return((strcmp(Xname,name) == 0) ? TRUE : FALSE);
	}
 return(FALSE);					/* OK to execute optimization.*/
}
	boolean
PSkip(mask)
unsigned long mask;
{extern unsigned long int Xpmask;	/* Mask program is running under.*/
 extern char *Xname;		/* Name of function to be debugged.	*/
 char *name;			/* Function name.	*/
 extern FN_Id FuncId;		/* FN_Id of function optimized.*/

 if((mask & Xpmask) != 0)			/* Satisfy mask?	*/
	{if(*Xname == EOS)			/* If no user name,	*/
		return(TRUE);			/* do all functions.	*/
	 if(FuncId == (FN_Id) NULL)
		return(TRUE);			/* It should have been there. */
	 name = GetAdExpression(Tbyte,GetFnName(FuncId));
	 return((strcmp(Xname,name) == 0) ? TRUE : FALSE);
	}
 return(FALSE);					/* OK to execute optimization.*/
}
	void
Xdebug(debugno)
int debugno;
{
 extern boolean cflag;
 unsigned long GetPmask();
 extern unsigned long Xmask;
 extern unsigned long Xpmask;

 Xmask = Xpmask = 0;
 switch(debugno){	/* for backward compatibility */
 case 10:	cflag = TRUE; endcase;
 case 20:	Xmask = XGRA; endcase;
 case 30:	Xmask = XIL; endcase;
 case 40:	Xmask = XLICM; endcase;
 case 50:	Xmask = XVTRACE; endcase;
 case 60:	Xmask = XPEEP; endcase;
 default:
	Xpmask = GetPmask(debugno);
	if(Xpmask != 0){
		Xmask = XPEEP;
		Xpmask = ~Xpmask;
	}
	endcase;
 }
 if( Xmask != 0 )
 	Xmask = ~Xmask;
}
