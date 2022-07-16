/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)newoptim:common/optim.c	1.13"

			/*	Machine independent improvement routines */

#include	<stdio.h>
#include	<malloc.h>
#include	<memory.h>
#include	"defs.h"
#include	"debug.h"
#include	"OpTabTypes.h"
#include	"RegId.h"
#include	"ANodeTypes.h"
#include	"ANodeDefs.h"
#include	"olddefs.h"
#include	"TNodeTypes.h"
#include	"optim.h"
#include	"LoopTypes.h"
#include	"OperndType.h"
#include	"RoundModes.h"
#include	"TNodeDefs.h"
#include	"optab.h"

#define B_EXIT	2
#define DONE	1
#define LEFT	3
#define PCASES 13
#define RIGHT	2

			/* Unit of allocatable space (in char *'s) */
#ifndef NSPACE
#define NSPACE	1024
#endif

			/* Size of hash table for labels */
#ifndef LBLTBLSZ
#define LBLTBLSZ	239	/* should be prime */
#endif

				/* what to do if no input file specified */
#ifndef NOFILE
#define NOFILE()				/* By default, use stdin */
#endif

				/* What to report if file-opening fails */
#ifndef FFILER
#define FFILER(S)	"can't open %s\n"
#endif
						/* Block of text */

typedef struct block
	{struct block *next;	/* pointer to textually next block */
	 struct block *nextl;	/* pointer to next executed block if no br */
	 struct block *nextr;	/* pointer to next executed block if br */
	 struct block *ltest;	/* for loop termination tests */
	 TN_Id FirstTN;		/* First text node of block */
	 TN_Id LastTN;		/* Last text node of block */
	 short index;		/* block index for debugging purposes */
	 short length;		/* number of instructions in block */
	 short indeg;		/* number of text references */
	 short marked;		/* marker for various things */
	} BLOCK;

						/* Symbol table entry */

typedef struct lblstrct {
	AN_Id ad;		/* Address-node-identifier of label. */
	BLOCK *bl;		/* the block it is defined in */
	struct lblstrct *next;	/* hash table chain */
	} LBL;

						/* Data structures */

static REF r0;			/* header for non-text reference list */
static BLOCK b0;		/* header for block list */
static BLOCK * Lastb = NULL;	/* pointer to array of blocks previously
				** allocated for bldgr() */
static LBL *Lbltbl[LBLTBLSZ];	/* hash table of pointers to labels */
static int Numlbls;		/* count of labels in hash table */
static BLOCK *Prevb;		/* pointer to previous block during
				   traversal of list */
static int fnum = 0;		/* function counter */
static int idx;			/* block index (for debugging) */

				/* Space allocation control */

static struct space_t {	/* to manage space allocation */
	struct space_t *next;
	char *space[NSPACE - 1];
	} *s0 = NULL, **Space;
static char *Lasta, *Lastx;	/* pointers into allocatable space */
static long Maxu = 0, Maxm = 0, Maxa = 0;

static int Pcase[PCASES + 1];	/* block types during reconstruction of text */
static struct added {		/* to keep statistics on branches added */
	struct added *next;
	short fnum,
	n_added;
	} a0;

				/* Debugging flags.	*/


				/* For readability */

#define	CLEAR(s, n)	(void) memset((char *)(s), 0, (int)(n))
#define ALLB(b, s)	b = (s); b != NULL; b = b->next
#define PRCASE(N)	if(DBdebug(3,XPCI_REORD)) { prcase(N, b); Pcase[N]++; }
#define TOPOFBLOCK(p)	((p) == NULL || IsTxLabel(p))
#define TRACE(F)	if(DBdebug(3,XPCI)) printf("%cStarting %s[%d]\n", \
			    ComChar,F,fnum)
#define MPRINTF		if(DBdebug(3,XPCI_COMT)) printf
#define PRINDEX(P)	(b->P == NULL ? 0 : b->P->index)
#define PSTAT(S, N)	if ((N) > 0) FPRINTF(stderr, (S), (N))
#define ISUNCBL(b)	((b)->length == 1 && IsTxUncBr((b)->LastTN))
#define ISREMBL(b)	(ISUNCBL(b) && !ishb((b)->LastTN))
#define ISREMBR(p)	(IsTxBr(p) && !ishb(p))
#define RMBR(p)		(ndisc++, nsave++, DelTxNode(p))
/* TARGET follows branches until a non-branch is reached.  However,
** there is the danger that we will loop on ourselves if we encounter
** an infinite loop.  Solve the problem partially by preventing
** self-loops. */
#define TARGET(b)	while (b->nextl != NULL && ISUNCBL(b->nextl) &&\
				b->nextl != b) b = b->nextl
#define NEWBLK(n, type)	((type *) xalloc((n) * sizeof(type)))

			/* Declarations used in macros */

extern unsigned int ndisc;	/* Number of instructions discarded.	*/
extern int nsave;		/* Branches saved.	*/
STATIC void prcase();
/*extern char *memset();	** Sets memory; in C(3) library.	*/
extern char *xalloc();		/* Allocates memory.	*/

			/* Private function declarations */

STATIC void addlbl();		/* Add label in label table.	*/
STATIC boolean chktail();	/* Merge common tails.		*/
STATIC void clrltbl();		/* Clears label table.		*/
STATIC BLOCK *findlbl();	/* Finds a label in label table & */
				/* returns block it's in.	*/
STATIC void findlt();		/* Find rotatable loops.	*/
STATIC void indegree();		/* Compute the number of input branches.	*/
STATIC AN_Id label_left();	/* Get label of left target.	*/
STATIC void mkltbl();		/* Initializes the label table.	*/
STATIC void modrefs();		/* Modify refs of a block.	*/
STATIC BLOCK *nextbr();		/* Select next block to process.	*/
STATIC int outdeg();		/* Compute the number of output branches.	*/
STATIC void putbr();		/* Puts branch at end of block. */
STATIC void reach();		/* Mark all reachable blocks.	*/
STATIC BLOCK *reord1();		/* Reorders blocks.		*/
STATIC void rmtail();		/* Removes tails.		*/

	void
main(argc, argv) /* initialize, process parameters,  control processing, etc. */
int argc;			/* Argument count.	*/
register char *argv[];		/* Argument pointers.	*/

{extern void AN_INIT();		/* Initialize address node package.	*/
 extern void DBinit();		/* Initialize debugging.	*/
 extern void ParamInit();	/* Initialize misc parameters.	*/
 extern void ParseCmdLine();	/* Parses command line options. */
 extern void TN_INIT();		/* Initialize text node package.	*/
 extern void dstats();		/* Defined in local.c. */
 extern void exit();		/* System get-off; in C(3) library.	*/
 extern void fatal();		/* Handles fatal errors; in common. */
 int i;
 extern void init();
 extern REF *lastref;
 struct ldent ldtab[LIVEDEAD];	/* The Live-Dead Table for GRA.	*/
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 extern unsigned int ninst;	/* Number of instructions seen.	*/
 extern int nmerge;		/* redundant instructions */
 extern int nrot;		/* rotated loops. */
 extern int nsave;		/* Branches saved.	*/
 extern int nunr;		/* unreachable instructions */
 extern boolean sflag;		/* Want statistics on stderr if TRUE.	*/
 extern void yyinit();		/* Initialize; in Machine Dependent part. */
 extern void do_optim();	/* ...; in Machine Dependent part.	*/
					
 DBinit(0,0,"");			/* Initialize debugging. */
 ParamInit();				/* Initialize various local parameters. */
 ParseCmdLine(argc,argv);		/* Process parameters on command line */

 AN_INIT();				/* Initialize address node package. */
 TN_INIT();				/* Initialize text node package. */
 lastref = &r0;				/* Initialize reference list.	*/

 NOFILE();				/* If no input file specified, */

 init();
 yyinit();

				/* Transfer to the machine dependent part */

 do_optim(ldtab);
 if(sflag)				/* Print statistics if asked for. */
	{register struct added *a;
	 struct mallinfo info;		/* Space for malloc data.	*/

	 for(a = a0.next; a != NULL; a = a->next)
		FPRINTF(stderr, "%d branch(es) added to function %d\n",
			a->n_added, a->fnum);
	 dstats();			/* Machine dependent statistics */
	 PSTAT("%d unreachable instruction(s) deleted\n", nunr);
	 PSTAT("%d branch(es) saved\n", nsave);
	 PSTAT("%d instruction(s) merged\n", nmerge);
	 FPRINTF(stderr, "%d of %d total instructions discarded\n",
		ndisc, ninst);
	 PSTAT("%d loop(s) rotated\n", nrot);
	 FPRINTF(stderr,
		"%ld bytes used, %ld allocated\n%d function(s)\n",
		    Maxm, Maxa, fnum - 1);
	 if(DBdebug(3,XPCI_REORD))
		for (FPRINTF(stderr, "case\tnumber\n"),
				i = 0; i <= PCASES; i++)
			FPRINTF(stderr, "%2d\t%3d\n", i, Pcase[i]);
	 info = mallinfo();			/* Get malloc info. */
	 (void) fprintf(stderr,"mallinfo:\tarena:%d\n",info.arena);
	 (void) fprintf(stderr,"\t\tordblks:%d\tsmblks:%d\n",info.ordblks,
		info.smblks);
	 (void) fprintf(stderr,"\t\thblkhd:%d,\thblks:%d\n",info.hblkhd,
		info.hblks);
	 (void) fprintf(stderr,"\t\tusmblks:%d,\tfsmblks:%d\n",
		info.usmblks,info.fsmblks);
	 (void) fprintf(stderr,"\t\tuordblks:%d,\tfordblks:%d\n",
		info.uordblks,info.fordblks);
	} /* END OF if(sflag) */

#ifdef DEBUG
 DelTxNodes((TN_Id) NULL,(TN_Id) NULL);	/* See if 'free()'s match 'malloc()'s.*/
 DelAdNodes();
#endif /*DEBUG*/

if(ferror(stdin))
	{perror("optim");
	 fatal("main: I/O error in stdin 0x%8.8x\n", stdin);
	}
if(ferror(stdout))
	{perror("optim");
	 fatal("main: I/O error in stdout 0x%8.8x\n", stdout);
	}
exit(0);					/* Normal exit.	*/
 /*NOTREACHED*/
}


	void
init()			/* Reset pointers, counters, etc for next function */

{register struct space_t *p, *pp = NULL;
 long maxa = LBLTBLSZ * sizeof(LBL);
 extern NODE n0;		/* Node before the first node.	*/
 extern void xfree();		/* Releases memory obtained from xalloc(). */

 if(Lastb != NULL)				/* Free bldgr's storage */
	xfree((char *) Lastb);
 Lastb = NULL;					/* No memory now allocated */

 for (p = s0; p != NULL; p = pp)
	{maxa += sizeof(struct space_t);
	 pp = p->next;
	 xfree((char *) p);
	}
 if ((Maxu += Numlbls * sizeof(LBL)) > Maxm)
	Maxm = Maxu;
 if (maxa > Maxa)
	Maxa = maxa;
 Maxu = 0;
 Space = &s0;
 s0 = NULL;
 Lasta = Lastx = NULL;
 b0.next = NULL;				/* Initialize the block list. */
 b0.FirstTN = b0.LastTN = &n0;			/* Initialize block 0.	*/
 fnum++;

 return;
}
	void
rmlbls(first)			/* Remove unreferenced labels.	*/
int first;

{
 extern AN_Id GetP();		/* Gets AN_Id of jump destination.	*/
 extern unsigned int LBLSrem;	/* Number of labels removed.	*/
 extern void addref();		/* Add text reference to reference list. */
 STATIC void clrltbl();		/* Clears label table; in this file.	*/
 STATIC BLOCK *findlbl();	/* Finds a label in label table & */
				/* returns block it's in */
 STATIC void addlbl();		/* Add label in label table.	*/
 extern void funcprint();
 extern struct opent optab[];	/* See optab.h */
 REF *r;
 register TN_Id next_tn_id;
 register TN_Id tn_id;
 register AN_Id an_id;

 if(Xskip(XPCI_RL)) return;

 if(DBdebug(1,XPCI_RL))
	funcprint(stdout,"rmlbls: before",0);
 TRACE("rmlbls");

 clrltbl();

					/* Add references from data section. */

 for(r = r0.nextref; r != NULL; r = r->nextref)
	addlbl(r->lab,&b0);

			/* Add references from branches in text section. */

 for(ALLN(tn_id))
	{if(IsTxBr(tn_id) && (an_id = GetP(tn_id)) != NULL)
		{addlbl(an_id,&b0);
		 continue;
		}
#ifdef ISLABREF
	 if(first && ISLABREF(tn_id) && (an_id = GetP(tn_id)) != NULL)
		{addlbl(an_id,&b0);
		 addref(an_id);
		}
#endif
	} /* END OF for(ALLN(tn_id) */

	/* Delete non-hard labels that are not now in the label table. */

 tn_id = GetTxNextNode((TN_Id) NULL);		/* Get id of first node. */
 while(tn_id)					/* Scan all text nodes.	*/
					/* We shouldn't use for(ALLN(... */
					/* because ... is undefined after */
					/* DelTxNode(...). (Although it */
					/* works with this implementation. */
	{next_tn_id = GetTxNextNode(tn_id);
	 if(IsTxLabel(tn_id) &&
			!IsTxHL(tn_id) &&
			findlbl(GetTxOperandAd(tn_id,0)) == NULL)
		{
		if(DBdebug(3,XPCI_RL))
			{printf("%clabel %s removed\n",ComChar,
				GetAdExpression(Tbyte,GetTxOperandAd(tn_id,0)));
			}
		 DelTxNode(tn_id);
		 LBLSrem += 1;			/* Count of labels removed. */
		}

	 tn_id = next_tn_id;
	}
 return;
}
/* This routine attempts to economize on space be allocating a hunk
** of storage big enough for all program blocks.  It deallocates that
** hunk on the next call in hopes it can be reused.
*/
/*ARGSUSED*/
	void
bldgr(opt)	 		/* Build flow graph of procedure */
boolean opt;

{extern AN_Id GetP();		/* Gets AN_Id of jump destination.	*/
 AN_Id an_id;			/* Identifier of an address node.	*/
 register BLOCK *b = &b0;
 STATIC BLOCK *findlbl();	/* Finds a label in label table & */
				/* returns block it's in */
 extern void fprinst();		/* Prints an instruction on a stream.	*/
 extern void funcprint();	/* Prints a function.	*/
 register TN_Id tn_id;
 extern char *xalloc();		/* Allocates memory.	*/
 extern void xfree();		/* Frees memory allocated by xalloc(). */
 STATIC int MrgDegenCmpOk();	/* Help fn called only from bldgr -- ad
				   hoc optimization */
#ifdef BBOPTIM
 STATIC void mkltbl();		/* Initializes the label table.	*/
#else
 STATIC void addlbl();		/* Add label in label table.	*/
 STATIC void clrltbl();		/* Clears label table; in this file.	*/
#endif

 if(DBdebug(1,XPCI_BGR))
	funcprint(stdout,"bldgr: before",0);
 TRACE("bldgr");

 if(Lastb != NULL)		/* Deallocate old array, if any */
	xfree((char *) Lastb);

		/* Count number of blocks so we can allocate an array */

 idx = 0;					/* Use this to count blocks */
 tn_id = GetTxNextNode((TN_Id) NULL);		/* Start with first node. */
 while(tn_id) {   				/* Scan all the text nodes. */
    idx++;					/* Count one more block. */
    while(IsTxLabel(tn_id))		/* Skip leading labels. */
	tn_id = GetTxNextNode(tn_id);
	    
    for( ; (tn_id != NULL) && !IsTxLabel(tn_id);tn_id = GetTxNextNode(tn_id))
	if(IsTxBr(tn_id)) {
	    /* See if it's preceded by a degenerate compare */
	    switch(MrgDegenCmpOk(tn_id)) { 
	    case 0:
		break;
	    case 1: /* Branch always taken, delete compare and
		       change to unconditional jump */
		DelTxNode(tn_id->back);
		PutTxOpCodeX(tn_id,IJMP);
		break;
	    case 2: /* Branch never taken, delete compare and
		       branch. */
		DelTxNode(tn_id->back);
		DelTxNode(tn_id);
		continue; /* get the next node and keep going in
			     this block */
	    }
	    tn_id = GetTxNextNode(tn_id);
	    break;
	}
 } /* END OF while(tn_id) */

			/* idx is now the number of blocks.  Allocate array. */

 if(idx == 0)
	return;
 Lastb = (BLOCK *) xalloc(idx * sizeof(BLOCK));

						/* Now build the flow graph. */

 idx = 0;
 b = b0.next = Lastb;			/* Point at prospective first block. */
#ifndef BBOPTIM
 clrltbl();
#endif /*BBOPTIM*/
 for(tn_id = GetTxNextNode((TN_Id) NULL); tn_id != NULL; )
	{register TN_Id prevTN = GetTxPrevNode(tn_id);

	 b->next = b + 1;		/* "next" will be physically next */
	 b->index = ++idx;
	 b->length = b->indeg = 0;

				/* A block starts with 0 or more labels... */
	 while(IsTxLabel(tn_id))
		{
#ifndef BBOPTIM
		 addlbl(GetTxOperandAd(tn_id,0),b);
#endif /*BBOPTIM*/
		 tn_id = GetTxNextNode(tn_id);
		} /* END OF while(IsTxLabel(tn_id) */

				/* followed by 0 or more non-branch instructions
			   terminated with a branch or before another label */

	 for ( ; ((tn_id != NULL) && !IsTxLabel(tn_id));
			tn_id = GetTxNextNode(tn_id))
		{b->length++;
		 if(IsTxBr(tn_id))
			{tn_id = GetTxNextNode(tn_id);
			 break;
			}
		}
#ifdef BBOPTIM
	 if(opt)		/* Do dependent basic-block optimization */
		{int omit = bboptim(GetTxNextNode(prevTN),
			GetTxPrevNode(tn_id));
		 if(omit > b->length)
			omit = b->length;
		 b->length -= omit;
		 ndisc += omit;
		} /* END OF if(opt) */
#endif /*BBOPTIM*/
	 b->LastTN = GetTxPrevNode(tn_id);
						/* If non-empty block, */
	 if((b->FirstTN = GetTxNextNode(prevTN)) != tn_id)
		b++;			/* we will next do next block */
	} /* END OF for(tn_id = GetTxNextNode((TN_Id) NULL); tn_id != NULL; ) */

 b[-1].next = NULL;			/* (Assumes at least one block) next
					** pointer of last block we filled in
					** is NULL*/

#ifdef BBOPTIM
 mkltbl();			/* Make label table with only definitions */
#endif /*BBOPTIM*/

						/* Set branch pointers */

 for(ALLB(b, b0.next))
	{tn_id = b->LastTN;
	 b->nextl = b->next;
	 b->nextr = IsTxBr(tn_id) && (an_id = GetP(tn_id)) != NULL ?
		findlbl(an_id) : NULL;
	 if(IsTxUncBr(tn_id))
		{b->nextl = b->nextr;
		 b->nextr = NULL;
		}
	 if(DBdebug(3,XPCI_BGR))
		{fprintf(stdout,
		"%c\n%cblock %d (left: %d, right: %d, length: %d)\n%cfirst:\t",
			ComChar, ComChar,
			b->index, PRINDEX(nextl), PRINDEX(nextr),
			b->length, ComChar);
		 fprinst(stdout,-1,b->FirstTN);
		 fprintf(stdout,"%clast:\t",ComChar);
		 fprinst(stdout,-1,tn_id);
		} /* END OF if(DBdebug(3,PCI_BGR)) */
	} /* END OF for(ALLB(b,b0.next)) */
 return;
}

/* Next function, called only by bldgr(), checks if the node is a
   conditional branch preceded by a degenerate compare instruction,
   e.g., CMPW	addr,addr;	je	xxxx;
   It returns
	1 if the branch is always taken,
	2 if the branch is never taken, 
	0 otherwise.
   addr must not be volatile
*/

	STATIC int
MrgDegenCmpOk(tn_id)
TN_Id tn_id;	
{
    extern AN_Id i0;
    AN_Id addr;
    TN_Id p;
    if(IsTxCBr(tn_id) && (p=tn_id->back) && p->op == G_CMP
		      && (addr=GetTxOperandAd(p,1)) == GetTxOperandAd(p,2)
		      && GetTxOperandType(p,1) == GetTxOperandType(p,2)
		      && GetTxOperandAd(p,3) == GetTxOperandAd(p,0)
		      && GetTxOperandAd(p,3) == i0
		      && IsAdSafe(addr)
		      && !IsTxOperandVol(p,2) ) {
	/* NOW can we do it ??? */
	switch(tn_id->op) {
	case IJE:
	case IJLE:
	case IJLEU:
	case IJGE:
	case IJGEU:
	    return 1;
	case IJNE: 
	case IJG:
	case IJL:
	case IJGU:
	case IJLU:
	    return 2;
	default:
	    return 0;
	}
   }
   return 0;
}
	void
mrgbrs()	 		/* Merge branches to unconditional branches */

{extern AN_Id GetP();		/* Gets AN_Id of jump destination.	*/
 extern unsigned int Max_Ops;	/* Maximum operands in an instruction.	*/
 extern void PutP();		/*Puts AN_Id of jump destination in text node.*/
 register BLOCK *b, *bb;
 extern void funcprint();	/* Prints instructions with title.	*/
 STATIC void modrefs();		/* ...; in this file.	*/
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 extern int nsave;		/* Branches saved.	*/
 unsigned int operand;		/* Operand counter.	*/
 extern void textaudit();	/* Audits text nodes.	*/

 if(Xskip(XPCI_MBR))                            /* Should this optimization */
        return;                                 /* be done? NO. */

 if(DBdebug(1,XPCI_MBR))
	funcprint(stdout,"mrgbrs: before",0);
 TRACE("mrgbrs");

			/* Merge unconditional branches to their destinations */

 for(ALLB(b, b0.next))
	{if((bb = b->nextl) == b->next)		/* fall-through */
		{if((b = bb) == NULL)
			break;
		}
	 else
		/* CONSTCOND */
		{if((bb != NULL) && (bb != b) &&
				IsTxLabel(b->FirstTN) && ISREMBL(b))
			{ndisc++;
			 nsave++;
			 modrefs(GetTxPrevNode(b->LastTN),b,bb);
			}
		}
	} /* END OF for(ALLB(b, b0.next)) */

 if(DBdebug(4,XPCI_MBR))
	funcprint(stdout,"mrgbrs: after first loop",0);
 if(DBdebug(3,XPCI_MBR))
	textaudit("mrgbrs: after first loop");
	/*
	 * It is assumed that "ret" is an unconditional branch;
	 * that getp on a "ret" returns NULL; that this can be
	 * placed on an unconditional branch ("jbr");
	 * that fprinst() will convert "jbr NULL" back to "ret";
	 * but that the NULL
	 * cannot be placed on a conditional branch ("jne").
	 * (NULL is also returned by a multi-way branch (switch).)
	 */

 for(ALLB(b,b0.next))
	{AN_Id an_id;
	 register TN_Id tn_id = b->LastTN;

	 if(!IsTxBr(tn_id))
		continue;
	 if(IsTxUncBr(tn_id))
		/* CONSTCOND */
		{while(((bb = b->nextl) != NULL) &&
				(bb != bb->nextl) &&
				ISREMBL(bb))
			{register TN_Id pp = bb->LastTN;

			 if((an_id = GetP(pp)) != NULL)
				PutP(tn_id,an_id);
			 else			/* pp is a dead-end */
				{for(operand = 0; operand < Max_Ops; operand++)
					{PutTxOperandAd(tn_id,operand,
						GetTxOperandAd(pp,operand));
					 PutTxOperandType(tn_id,operand,
						GetTxOperandType(pp,operand));
					}
#ifdef USERTDATA
				 (void) memcpy((char *) &tn_id->userdata,
					(char *) &pp->userdata,
					sizeof(USERTTYPE));
#endif /*USERTDATA*/
				 PutTxOpCodeX(tn_id,GetTxOpCodeX(pp));
				}
			 b->nextl = bb->nextl;
			} /* END OF while(((bb = b->nextl) != NULL)  ... */
		}
	  else /* IsTxUncBr(tn_id) */
		/* CONSTCOND */
		{while(((bb = b->nextr) != NULL) &&
				(bb != bb->nextl) &&
				ISREMBL(bb) &&
				((an_id = GetP(bb->LastTN)) != NULL))
			{PutP(tn_id,an_id);
			 b->nextr = bb->nextl;
			}
		} /* END OF else(IsTxUncBr(tn_id) */
	} /* END OF for(ALLB(b,b0.next)) */

 if(DBdebug(3,XPCI_MBR))
	funcprint(stdout,"mrgbrs: after second loop",0);
 if(DBdebug(0,XPCI_MBR))
	textaudit("mrgbrs: after second loop");
 return;
}
	void
comtail()			/* Merge common tails from code blocks */

{register BLOCK *bi,*bj;
 BLOCK *bi0, *bj0;
 extern boolean cflag;
 boolean changed;
 STATIC boolean chktail();
 extern void funcprint();	/* Prints a function.	*/
 STATIC void indegree();

 if(Xskip(XPCI_COMT))                           /* Should this optimization */
        return;                                 /* be done? NO. */

 if(DBdebug(1,XPCI_COMT))
	funcprint(stdout,"comtail: before",0);
 TRACE("comtail");

 do	{changed = FALSE;

	 if(cflag)			/* For conservative analysis only */
		indegree();		/* compute indegree (0 from bldgr()) */

	 for(ALLB(bi,b0.next))		/* Compute a key for each block. */
		{bi->marked = 0;
		  if((bi->length == 1) && IsTxBr(bi->LastTN))
			continue;
		  bi0 = bi;
		  if(IsTxBr(bi->LastTN) && !IsTxUncBr(bi->LastTN))
			{
			 TARGET(bi0);
			 bi->marked += GetTxOpCodeX(bi0->LastTN);
			}
		 bi->marked += bi0->nextl - &b0;
		} /* END OF for(ALLB(bi,b0.next) */

	 for(ALLB(bi,b0.next))
		{if(!bi->marked)
			continue;
		 for(ALLB(bj,bi->next))
			{if(bi->marked != bj->marked)
				continue;	/* Quick sieve on key */
			 if(bi->nextr != bj->nextr)
				continue;
			 bi0 = bi;
			 bj0 = bj;
				/* If both blocks end in conditional branches,
				 * look ahead for left targets */
			 if(IsTxBr(bj->LastTN) && !IsTxUncBr(bj->LastTN))
				{if(GetTxOpCodeX(bi->LastTN) !=
						GetTxOpCodeX(bj->LastTN))
					continue;
				 if((bi->nextr == NULL) &&
					    !IsTxSame(bi->LastTN,bj->LastTN))
					continue;
				 TARGET(bi0);
				 TARGET(bj0);
				}
				/* Blocks must fall through to same place */
			 if(bi0->nextl != bj0->nextl)
				continue;
				/* Dead-end branches must have same text */
			 if((bi0->nextl == NULL) &&
					!IsTxSame(bi0->LastTN,bj0->LastTN))
				continue;
			 if(chktail(bi,bj,bi0->nextl) == TRUE)
				 changed = TRUE;
			} /* END OF for(ALLB(bj,bi->next) */
		} /* END OF for(ALLB(bi,b0.next) */
	} while (changed == TRUE);
 return;
}
	STATIC boolean
chktail(bi,bj,bl)		/* Merge tails of bi-> and bj-> */
register BLOCK *bi, *bj;
BLOCK *bl;

{extern boolean cflag;
 extern char *getspace();
 STATIC void modrefs();		/* ...; in this file.	*/
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 extern int nmerge;
 extern int nsave;		/* Branches saved.	*/
 STATIC void rmtail();
 register BLOCK *bn;
 TN_Id pi = bi->LastTN, pj = bj->LastTN, FirstTN, LastTN, pb = NULL;
 int length = 0, isbri = 0, isbrj = 0;

			/* pi and pj scan backwards through blocks bi and bj 
			   until difference or no more code */

 if(IsTxBr(pi))		/* Trailing branches have already been matched */
	{pb = pi;
	 pi = GetTxPrevNode(pi);
	 isbri++;
	}
 if(IsTxBr(pj))
	{pb = pj;
	 pj = GetTxPrevNode(pj);
	 isbrj++;
	}
 for(FirstTN = LastTN = pj; !TOPOFBLOCK(pi) && !TOPOFBLOCK(pj) &&
		IsTxSame(pi, pj) == TRUE; length++)
	{FirstTN = pj;
	 pi = (pi == bi->FirstTN) ? NULL : GetTxPrevNode(pi);
	 pj = (pj == bj->FirstTN) ? NULL : GetTxPrevNode(pj);
	}
 if(length == 0)
	return(FALSE);

	/* If blocks identical, change references to one to the other */

 if(TOPOFBLOCK(pi) && TOPOFBLOCK(pj))
	{isbri = 0;
	 modrefs(pj, bj, bn = bi);
	 MPRINTF("%cblock %d merged into block %d and deleted\n",
		ComChar, bj->index, bi->index);
	}

	/*
	 * Conservative common-tail merging avoids adding a branch to
	 * achieve a merge.  It merges only blocks which join with no other
	 * blocks joining at that point, so that the joining branch is merely
	 * raised above the common tail, and no new branch is added.
	 */

 else
	if(cflag && ((bl == NULL) || (bl->indeg > 2)))
		return(FALSE);			/* Conservative common tails */

		/* If one block is a tail of the other, remove the tail from the
		   larger block and make it reference the smaller */

	else
		if(TOPOFBLOCK(pi))
			{isbri = 0;
			 bj->LastTN = pj;
			 bj->length -= length + isbrj;
			 bj->nextl = bn = bi;
			 rmtail(bj);
			}
	else
		if(TOPOFBLOCK(pj))
			{isbrj = 0;
			 bi->LastTN = pi;
			 bi->length -= length + isbri;
			 bi->nextl = bn = bj;
			 rmtail(bi);
			}

	/* otherwise make a new block, remove tails from common blocks and
	   make them reference the new block */

	else
		{bi->LastTN = pi;
		 bj->LastTN = pj;
		 bi->length -= length + isbri;
		 bj->length -= length + isbrj;
		 bn = GETSTR(BLOCK);
		 *bn = *bj;
		 bn->FirstTN = FirstTN;
		 bn->LastTN = LastTN;
		 bn->length = (short)length;
		 bn->index = ++idx;
		 bn->indeg = 2;
		 bi->nextl = bj->nextl = bj->next = bn;
		 bi->nextr = bj->nextr = NULL;
		 MPRINTF("%ctails of %d and %d merged into new block %d\n",
			ComChar,bi->index,bj->index,idx);
		}
 if(pb != NULL && !IsTxBr(bn->LastTN))	/* Save final branch */
	{ndisc--;
	 nsave--;
	 bn->length++;
	 pb->back = bn->LastTN;
	 bn->LastTN = bn->LastTN->forw = pb;
	}

#ifdef IDVAL
 for(pb = bn->FirstTN; pb != NULL; pb = GetTxNextNode(pb))
	{PutTxUniqueId(pb,IDVAL);
	 if(pb == bn->LastTN)
		break;
	}
#endif /*IDVAL*/

 ndisc += length + isbri + isbrj;
 nmerge += length;
 nsave += isbri + isbrj;	/* don't blame resequence for added branch */
 MPRINTF("%c%d instruction(s) common to blocks %d and %d\n",
	ComChar, length, bi->index, bj->index);
 return(TRUE);
}


	STATIC void
rmtail(b)
register BLOCK *b;		/* remove tail of b */

{
 b->nextr = NULL;
 MPRINTF("%ctail of block %d deleted\n",ComChar, b->index);
 return;
}
	STATIC void
modrefs(pi,bi,bj) 		/* Change all refs from bi to bj.	*/
TN_Id pi;			/* TN_Id of last label to move.	*/
register BLOCK *bi, *bj;

{
 STATIC void addlbl();	/* Add label in label table.	*/
 register BLOCK *b;
 extern void fprinst();		/* Prints an instruction on a stream.	*/
 TN_Id tn_id;

 if(DBdebug(0,XPCI))
	{(void) fprintf(stdout,"%cmodrefs:\tpi is:\t",ComChar);
	 if(pi == NULL)
		(void) fprintf(stdout,"NULL");
	 else
		fprinst(stdout,-1,pi);
	 (void) fprintf(stdout,"\tbi is %d, bj is %d\n",
		bi->index,bj->index);
	}
 if(pi != NULL)			/* Transfer labels, if any, from bi to bj. */
		/* bi->FirstTN points to the first label to be transferred,
		 * pi points to the last. */
	{bj->FirstTN->back = pi;
	 pi->forw = bj->FirstTN;
	 bj->FirstTN = bi->FirstTN;
	 for(tn_id = pi; tn_id != NULL; tn_id = GetTxPrevNode(tn_id))
		{addlbl(GetTxOperandAd(tn_id,0),bj);
		 if(tn_id == bi->FirstTN)	/* Quit at start of block bi. */
			break;
		} /* END OF for(tn_id = pi; tn_id != NULL; tn_id = ... ) */
	} /* END OF if(pi != NULL) */

 for(ALLB(b, &b0))			/* Update the block structure.	*/
	{
	 if(b->next == bi)		/* Remove block bi from blocklist. */
		b->next = bi->next;
	 if(b->nextl == bi)		/* Make next-left references to bi */
		b->nextl = bj;		/* refer to bj.	*/
	 if(b->nextr == bi)		/* Make next-right references to bi */
		b->nextr = bj;		/* refer to bj.	*/
	}

 return;
}
	void
reord()				/* Reorder code.	*/

{register BLOCK *b;
 STATIC void findlt();
 extern void funcprint();	/* Prints a function.	*/
 STATIC void indegree();
 STATIC void mkltbl();		/* Initializes label table.	*/
 extern int nrot;		/* rotated loops. */
 extern NODE ntail;
 STATIC void putbr();		/* Puts branch at end of block; in this file. */
 STATIC BLOCK *reord1();
 extern void rmunrch();
 extern void textaudit();	/* Audits text nodes.	*/

 if(DBdebug(999,XPCI_REORD))
	funcprint(stdout,"reord: before",0);
 TRACE("reord");

 for(ALLB(b, b0.next))
	{b->ltest = NULL;
	 b->marked = 0; 		/* Mark all blocks as unprocessed */
	}
 indegree();				/* Compute indegree.	*/

 if(nrot >= 0)
	findlt();			/* Find rotatable loops.	*/

					/* Tie blocks back together.	*/
 if(DBdebug(3,XPCI_REORD))
	printf("%cblock\tleft\tright\tcase\tlabels\n",ComChar);
 Prevb = &b0;
 b = b0.next;
 while( b != NULL)
	b = reord1(b);
 if(Prevb->nextl != NULL)
	putbr(Prevb);
 Prevb->LastTN->forw = &ntail;		/* Tack on tail node to text list. */
 ntail.back = Prevb->LastTN;
 if(DBdebug(3,XPCI_REORD))
	textaudit("reord: after reord1 loop");

 mkltbl();			/* Make label table with only definitions */
 rmunrch(TRUE);				/* Remove unreachable code.	*/

 return;
}
	STATIC void
indegree()			/* Compute indegree.	*/

{register BLOCK *b, *bb;

 for (ALLB(b, b0.next))
	b->indeg = 0;
 for (ALLB(b, b0.next))				/* Compute indegree.	*/
	{if((bb = b->nextl) != NULL)
		bb->indeg++;
	 if((bb = b->nextr) != NULL)
		bb->indeg++;
	}
 return;
}
	STATIC void
findlt()			/* Find rotatable loops.	*/

	/* To identify the top and termination-test of a rotatable loop:
	 * Look at the target of an unconditional backward branch.
	 * If it has only one reference, then it isn't the start of a loop.
	 * Then look at all intermediate blocks in lexical order
	 * to find a conditional jump past the backward branch.
	 * This is a very simplistic heuristic approach, because the loop
	 * test is actually never made.
	 * But it seems to give reasonable results rather rapidly.
	 * If there is more than one exit from the loop,
	 * rotate at the exit nearest to the bottom,
	 * in order to keep the elements of a compound test near each
	 * other (in case of window optimization)
	 * and near the bottom (in case of span-dependent branches).
	 */

{
 register BLOCK *b, *bl, *bb, *br;

 if(DBdebug(3,XPCI_BGR))
	printf("%cltests are:",ComChar);
 for (ALLB(b, b0.next))
	{if((b->nextr != NULL) ||
			((bl = b->nextl) == NULL) ||
			(bl->indeg < 2) ||
			(bl->index > b->index) || (bl->ltest != NULL))
		continue;
	 for (bb = bl; bb != NULL && bb->index < b->index; bb = bb->next)
		if ((br = bb->nextr) != NULL && br->index > b->index)
			bl->ltest = bb;
	 if(DBdebug(3,XPCI_BGR) && bl->ltest != NULL)
		printf(" %d/%d", bl->index, bl->ltest->index);
	}
 if(DBdebug(3,XPCI_BGR))
	putchar(NEWLINE);
 return;
}
	STATIC BLOCK *
reord1(b)
register BLOCK *b;

{extern void PutP();		/* Defined in comutil.c. */
 register BLOCK *bl, *br, *blt;
 STATIC AN_Id label_left();	/* In this file.	*/
 STATIC BLOCK *nextbr();
 extern int nrot;		/* rotated loops. */
 STATIC void putbr();		/* Buts branch at end of block; in this file. */
 STATIC int outdeg();

					/* Top of rotatable loop.	*/
	/* Don't rotate unless there already must be a branch to the entry. */

 if((b->ltest != NULL) && (b != Prevb->nextl) &&
		    ((bl = b->ltest->nextl)->ltest == NULL) && !bl->marked)
	{b->ltest = NULL;
	 nrot++;
	 return (bl);
	}

				/* Mark block as processed and tie it in. */

 b->marked++;
 if (b != Prevb->nextl)				/* If this block is not the */
						/* next(left) block from the */
	putbr(Prevb);				/* the previous block, put a */
						/* jump to the next on the */
						/* end of the previous. If a */
						/* label is required on the */
						/* next(left) block, generate */
						/* one, too.	*/
						/* Us&We mean first node of */
						/* current block (b).	*/
						/* Previous means last	*/
						/* node of previous block. */
 Prevb->LastTN->forw = b->FirstTN;		/* Previous points to us. */
 b->FirstTN->back = Prevb->LastTN;		/* We point to previous. */
 Prevb = b;

						/* Dead-end block.	*/

 if ((bl = b->nextl) == NULL)
	{PRCASE(0);
	 return (nextbr(b));
	}

 bl->indeg--;
 if ((br = b->nextr) != NULL)
	br->indeg--;

						/* Top of rotatable loop */

 if ((blt = bl->ltest) != NULL && blt->nextl->ltest == NULL &&
		!blt->nextl->marked && !blt->nextr->marked && outdeg(bl) <= 1)
	{PRCASE(1);
	 b = blt->nextl;
	 bl->ltest = NULL;
	 nrot++;
	 return (b);
	}

 if (br == NULL)	 /* Unconditional branch or conditional to dead-end */
	{if (!bl->marked)		 /* to unprocessed block */
		{if (bl->indeg <= 0)	 /* with indeg 1 */
			{PRCASE(2);
			 return (bl);
			}

					/* Branch to block with indeg > 1
					   that originally followed this one */

		 if (bl == b->next)
			{PRCASE(3);
			 return (bl);
			}

					/* Branch to dead-end block */

		 if (bl->nextl == NULL)
			{PRCASE(4);
			 return (bl);
			}

		}

					/* All other unconditional branches */

	 PRCASE(5);
	 return(nextbr(b));
	}

				/* Conditional branch to processed block */

 if (br->marked && !bl->marked)
			/* fall through to unprocessed block with indeg = 1 */

	{if (bl->indeg <= 0)
		{PRCASE(6);
		 return (bl);
		}

			/* Fall through to unprocessed block with indeg > 1
			   that originally followed this one */

	 if (bl == b->next)
		{PRCASE(7);
		 return (bl);
		}
	}

			/* Reversible conditional branch to unprocessed block,
					fall through to processed block */

 if (bl->marked && !br->marked && IsTxRev(b->LastTN))
	{RevTxBr(b->LastTN);
	 PutP(b->LastTN, label_left(b));
	 b->nextr = b->nextl;
	 b->nextl = br;
	 PRCASE(8);
	 return (br->indeg <= 0 ? br : nextbr(b));
	}

		/* All other conditional branches that have one leg or the
		   other going to processed blocks */

 if (bl->marked || br->marked)
	{PRCASE(9);
	 return (nextbr(b));
	}

				/* Fall through to block with indeg = 1
		   but not if it is an unlabeled unconditional transfer */

 if (bl->indeg <= 0 && !(IsTxUncBr(bl->FirstTN) && IsTxRev(b->LastTN)))
	{PRCASE(10);
	 return (bl);
	}

				/* Reversible branch to block with indeg = 1 */

 if (br->indeg <= 0 && IsTxRev(b->LastTN))
	{RevTxBr(b->LastTN);
	 PutP(b->LastTN,label_left(b));
	 b->nextr = b->nextl;
	 b->nextl = br;
	 PRCASE(11);
	 return (br);
	}

				/* Fall through to block with indeg > 1 that
				   originally followed this block */

 if (bl == b->next)
	{PRCASE(12);
	 return (bl);
	}

						/* Everything else */

 PRCASE(13);
 return(nextbr(b));
}
/* Routine outdeg works in conjunction with loop rotation.  It uses a
** heuristic to determine how many of the loop exit target's remaining
** incoming arcs are due to exits from the loop that is to be rotated.
** Outdeg is called with a pointer to the top-of-loop block.  It scans
** lexically through the blocks that follow the top (much like findlt())
** until
**	1) there is no next block
**	2) the "left" path points at the loop top, indicating the block
**		is the loop end
**	3) the new block's index is at or past the loop target (since
**		findlt calls something a loop exit when the block index
**		of the "right" path is beyond the loop end)
**
** As we scan through the blocks lexically, we decrement the effective
** indegree of the loop target whenever we find a "right" path that
** goes to the target from an unmarked block.  (If the block was marked,
** its contribution to indegree has already been accounted for.)
*/

	STATIC int
outdeg(top)
BLOCK * top;				/* Pointer to top of loop */
{
 BLOCK * target = top->ltest->nextr;	/* loop exit target */
 BLOCK * bp;				/* scanning block pointer */
 int Lindegree = target->indeg;		/* in-degree of target block */

    /* As a short-circuit, discontinue searching when the new indegree
    ** is <= 1
    */

 if(Lindegree <= 1)
	return(Lindegree);
    
 for (bp = top;
		bp != NULL		/* have a block */
		&& bp->nextl != top		/* it doesn't close the loop */
		&& bp->index < target->index ; /* it isn't past the target */
		bp = bp->next)
	{if(! bp->marked		/* the block is unmarked */
		    && bp->nextr == target	/* it branches cond.to target */
		    && --Lindegree <= 1)		/* time to quit */
	    break;
	}

 return(Lindegree);			/* return effective indegree */
}
	STATIC void
prcase(n, b)
int n;
register BLOCK *b;	 	/* Print information during reord */

{register TN_Id p;

 printf("%c%d\t%d\t%d\t%d",ComChar, b->index,
	PRINDEX(nextl), PRINDEX(nextr), n);
 for(p = b->FirstTN; IsTxLabel(p); p = GetTxNextNode(p))
	{printf("\t%s(%s)",optab[GetTxOpCodeX(p)].oname,
		GetAdExpression(GetTxOperandType(p,0),GetTxOperandAd(p,0)));
	}
 putchar(NEWLINE);
 return;
}
	STATIC BLOCK *
nextbr(b)
register BLOCK *b;	/* select next block to process */

{register BLOCK *bb;

	/* first look for orphan blocks (no more references) from the top */

 for (ALLB(bb, b0.next))
	if (!bb->marked && bb->indeg <= 0)
		return (bb);

	/* now look for unmarked block with live consequent (circularly) */

 for (bb = b->next; bb != b; bb = bb->next)
	if (bb == NULL) /* circular scan for next block */
		bb = &b0;
	else
		if (!bb->marked &&
				bb->nextl != NULL && !bb->nextl->marked)
			return (bb);

	/* now look for any unmarked block (circularly) */

 for (bb = b->next; bb != b; bb = bb->next)
	if (bb == NULL) /* circular scan for next block */
		bb = &b0;
	else
		if (!bb->marked)
			return (bb);

 return (NULL);					/* no more blocks to process */
}
	STATIC void
putbr(b)			 /* Append a branch to b->nextl onto b */
register BLOCK *b;

{extern void PutP();		/* Inserts jump destination. */
 AN_Id an_id;
 TN_Id back;
 STATIC AN_Id label_left();	/* Defined in this file.	*/
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 extern int nsave;		/* Branches saved.	*/
 TN_Id pl;
 TN_Id tn_id;

 pl = b->LastTN;
 if((b == &b0) || IsTxUncBr(pl))
	return;
 ndisc--;
 nsave--;
 b->length++;
						/* Make IJMP node after */
						/* the one specified by	*/
						/* b->LastTN.	*/
						/* While it must be after */
						/* this one, it is not	*/
						/* necessarily before the */
						/* next one.	*/
 tn_id = GetTxNextNode(pl);			/* Save previous back pointer.*/
 back = tn_id->back;
 b->LastTN = MakeTxNodeAfter(pl,IJMP);
 tn_id->back = back;				/* Restore prev. back pointer.*/
 an_id = label_left(b);				/* Get destination label. */
 PutP(b->LastTN,an_id);				/* Put in jump destination. */
 return;
}
	STATIC AN_Id
label_left(b)
register BLOCK *b;		/* Get label of b->nextl.	*/

{extern AN_Id GetP();		/* Gets AN_Id of jump destination.	*/
 AN_Id an_id;			/* AN_Id of new label node.	*/
 register BLOCK *bl;
 extern void fatal();		/* Handles fatal errors.	*/
 TN_Id forw;
 extern void newlab();		/* Returns a new label.	*/
 char newlabel[16+1];		/* Place for a new label.	*/
 register TN_Id p;
 register TN_Id pf;
 TN_Id tn_id;

 if((bl = b->nextl) == NULL)
	fatal("label_left: label of nonexistent block requested\n");
 for( ; ISUNCBL(bl) && (bl->nextl != bl); bl = bl->nextl)
	{if(bl->nextl == NULL)
		{an_id = GetP(bl->LastTN);
		 if (an_id == NULL)		/* No target.	*/
			break;
		 b->nextl = NULL;		/* Dead-end */
		 return(an_id);
		} /* END OF if(bl->nextl == NULL) */
	} /* END OF for( ; (ISUNCBL(bl) && (bl->next != bl)); bl = bl->next) */
 b->nextl = bl;				/* re-aim b at final target */
 pf = bl->FirstTN;
 if(IsTxLabel(pf) && !IsTxHL(pf))		/* If there is a soft label, */
	return(GetTxOperandAd(pf,0));		/* use it.	*/

 bl->length++;					/* Stick new label at beg. */
 newlab(newlabel,".O",sizeof(newlabel));	/* Get a new label.	*/
 an_id = GetAdAbsolute(Tbyte,newlabel);		/* Make a new label node. */
 PutAdLabel(an_id,TRUE);			/* Mark it a label node. */
 tn_id = GetTxPrevNode(pf);			/* Save previous forw ptr. */
 forw = GetTxNextNode(tn_id);
 p = MakeTxNodeBefore(pf,LABEL);		/*Make label node ahead of it.*/
 PutTxOperandAd(p,0,an_id);			/* Insert label and	*/
 PutTxOperandType(p,0,Tubyte);			/* its type.	*/
 if(!bl->marked)		/* This block not yet processed by reord so */
 	tn_id->forw = forw;		/*point to prev first textnode.*/
 bl->FirstTN = p;				/*Include label node in block.*/
 return(GetTxOperandAd(p,0));
}
	void
rmunrch(preserve)		/* Remove unreachable code.	*/
boolean preserve;

{extern boolean IsHLP();	/* TRUE if Hard Label Present.	*/
 extern void funcprint();	/* Prints a function.	*/
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 extern int nunr;
 STATIC BLOCK *findlbl();	/* Finds a label in label table & */
				/* returns block it's in */
 extern unsigned int ndisc;	/* Number of instructions discarded.	*/
 TN_Id next;
 extern int nsave;		/* Branches saved.	*/
 STATIC void reach();
 register REF *r;
 register BLOCK *b, *prevb;

 if(DBdebug(1,XPCI_RUNR))
	funcprint(stdout,"rmunrch: before",0);
 TRACE("rmunrch");

 if(b0.next == NULL)				/* If no blocks,	*/
	return;					/* forget it.	*/

 for(ALLB(b, b0.next))				/*Mark all blocks unreachable */
	b->marked = 0;				/* to initialize.	*/

 reach(b0.next);				/* Mark all blocks that	*/
						/* are reachable from initial */
						/* block.	*/
			/* Mark all blocks reachable from hard-label blocks */

 for(ALLB(b, b0.next))				/* Scan the blocks.	*/
	if(!b->marked && IsHLP(b->FirstTN))
		reach(b);

			/* Mark all blocks reachable from non-text references */

 for (r = r0.nextref; r != NULL; r = r->nextref)
	if ((b = findlbl(r->lab)) != NULL && !b->marked)
		reach(b);

 for(ALLB(b, b0.next))				/* Remove unmarked blocks. */
	{if(b->marked)				/* Is this block marked? */
		prevb = b;			/* Yes.	*/
	 else
		{ndisc += b->length;		/* Block is not marked.	*/
		 if(ISUNCBL(b) && IsTxLabel(b->FirstTN))
			nsave++;
		 else
			{if(DBdebug(3,XPCI_RUNR))
				printf("%cunreachable block %d removed\n",
					ComChar, b->index);
			 nunr += b->length;
			}
		 if(preserve)		/* Node sequence must be preserved */
			{for(next=NULL ; b->FirstTN != NULL; b->FirstTN = next)
				{next = GetTxNextNode(b->FirstTN);
				 DelTxNode(b->FirstTN);
				 b->length -= 1;
				 if(b->FirstTN == b->LastTN)
					break;
				}
			} /* END OF if(preserve) */
		 prevb->next = b->next;
		}
	} /* END OF for(ALLB(b,b0.next)) */
 return;
}
	STATIC void
reach(bcur)
register BLOCK * bcur;

{register BLOCK *bback, *tmp;

 bback = bcur;
 tmp = 0;

 while(bcur->marked != DONE)
	{bcur->marked = LEFT;
		/*
	 	* Link around the second of successive removable branches 
		* with same op-codes; multi-way branches (switches) must 
		* be identical in text.
	 	*/
	
	 while((tmp != bcur->nextl) &&
			((tmp = bcur->nextl) != NULL) && 
			!tmp->marked &&
			/* CONSTCOND */
			(tmp->length == 1) &&
	    		(tmp->LastTN->op == bcur->LastTN->op) && 
				ISREMBR(tmp->LastTN))

		{if(!(IsTxUncBr(tmp->LastTN) || 
				tmp->nextr != NULL ||
				IsTxSame(tmp->LastTN, bcur->LastTN)))
			break;
		 bcur->nextl = tmp->nextl;
		}
	
	 if((tmp != NULL) && !tmp->marked)
		{bcur->nextl = bback;
		 bback = bcur;
		 bcur = tmp;
		 continue;
		}

	 if(((tmp = bcur->nextr) != NULL) && !tmp->marked)
		{bcur->nextr = bback;
		 bback = bcur;
		 bcur->marked = RIGHT;
		 bcur = tmp;
		 continue;
		}

	 bcur->marked = DONE;
	 tmp = bcur;
	 bcur = bback;

	 switch(bcur->marked)
		{case LEFT:
			bback = bcur->nextl;
			bcur->nextl = tmp;
			break;
		 case RIGHT:
			bback = bcur->nextr;
			bcur->nextr = tmp;
			endcase;
		} /* END OF switch(bcur->marked) */
	} /* END OF while(bcur->marked != DONE) */
 return;
}
					

	void
rmbrs()				/* Remove redundant branches */

{register BLOCK *b, *bl;
 extern void funcprint();	/* Prints a function.	*/
 register TN_Id p;

 if(Xskip(XPCI_RMBR))                           /* Should this optimization */
        return;                                 /* be done? NO. */

 if(DBdebug(1,XPCI_RMBR))
	funcprint(stdout,"rmbrs: before",0);
 TRACE("rmbrs");

 for(ALLB(b, b0.next))
	/* CONSTCOND */
 	{if((bl = b->nextl) != NULL &&
			(p = b->LastTN)->forw == bl->FirstTN && ISREMBR(p))

			/* delete unconditional branch ahead of target */

		{if(IsTxUncBr(p))
			{RMBR(p);
			 continue;
			}

			/* delete conditional branch ahead of target
			   or ahead of unconditional branch to same target */

		 do	{if (b->nextr == bl || b->nextr == NULL &&
					bl->nextl == NULL && ISUNCBL(bl) &&
					IsTxSameOps(p,bl->LastTN))
				{RMBR(p);
				 break;
				}
			} while (ISUNCBL(bl)
				 &&  bl != bl->nextl	/* avoid self-loop */
				 && (bl = bl->nextl) != NULL);
		}
	}
 return;
}
/* ldanal -- perform live/dead analysis over flow graph
**
** This routine calculates live/dead register information for the
** entire flow graph in these steps:
**
**  1.	Allocate temporary array to hold block-level live/dead info.
**	Initialize it.
**  2.	On a block-wise basis, determine registers set and used by
**	each instruction.  Determine registers used and set by the
**	block.
**  3.	Propagate register use/set information throughout the flow
**	graph blocks.
**  4.	Propagate final information back through each block to
**	reflect correct live/dead information for each instruction.
**
**  We use the live/dead algorithm described in the Aho and Ullman
**  "dragon book".
*/

#ifdef LIVEDEAD
	void
ldanal()		 /* Perform live-dead register analysis.	*/


{
 typedef unsigned long int LDREG;
 extern unsigned int GnaqListSize;	/* Current GNAQ list size.	*/
 unsigned int NVectors;			/* Number of vectors to use.	*/
 extern int REGS[];		/* Set according to the number of live/dead */
				/* variables.	*/
 extern int RETREG[];		/* Set according to comments on return */
				/* instructions.	*/
 register BLOCK * b;		/* Pointer to current block.	*/
 extern void bldgr();		/* Builds flowgraph; in this file.	*/
 boolean changed;
 unsigned long int Dead[NVECTORS];	/* Place for dead information.	*/
 extern void funcprint();	/* Prints a function.	*/
 struct ldinfo			/* temporary block-level structure */
	{LDREG buses[NVECTORS];	/* registers used by block */
	 LDREG bdef[NVECTORS];	/* registers defined (set) by block */
	 LDREG bin[NVECTORS];	/* registers live coming into block */
	 LDREG bout[NVECTORS];	/* registers live exiting block */
	};
 struct ldinfo * lddata;	/* array of data for each block */
 register struct ldinfo * ldptr;	/* pointer to one of the above */
 unsigned long int Live[NVECTORS];	/* Place for live information.	*/
 TN_Id p;			/* TN_Id of current inst. node.	*/
 extern void sets();		/* Defined in comutil.c. */
 extern void uses();		/* Defined in comutil.c. */
 register unsigned int vector;
 extern void xfree();		/* Frees memory allocated by xalloc(). */

 NVectors = (GnaqListSize > (NVECTORS * sizeof(unsigned int) * B_P_BYTE)) ?
	(NVECTORS * sizeof(unsigned int) * B_P_BYTE) : GnaqListSize;
 NVectors = (NVectors + (sizeof(unsigned int) * B_P_BYTE) - 1) /
	(sizeof(unsigned int) * B_P_BYTE);

 if(DBdebug(1,XPCI_LDA))
	funcprint(stdout,"ldanal: before",0);
 TRACE("ldanal");

 bldgr(FALSE);		/* update block structure but don't call bboptim */
 lddata = NEWBLK(idx + 1, struct ldinfo);
/* Initialize:  set the recently allocated array to zero.  The idea, here,
** is that each entry in the array corresponds to one block in the flow
** graph.  We assume that blocks have sequential index numbers and that
** idx is the last index number.
*/

 CLEAR(lddata, (idx + 1) * sizeof(struct ldinfo));

/* Step 2.  Calculate uses/def for each node and for the containing block. */

 for(ALLB(b,b0.next))
	{ldptr = lddata + b->index;
	 for(p = b->LastTN; !IsTxLabel(p); p = GetTxPrevNode(p))
		{uses(p,Live,NVectors);	/* What's used here,	*/
#if LIVEREGS
		 Live[0] |= LIVEREGS;	/* + always live.	*/
#endif
		 sets(p,Dead,NVectors);		/* What's set, but not */
						/* used here.	*/
		 for(vector = 0; vector < NVectors; vector++)
			{Dead[vector] &= ~Live[vector];
			 ldptr->buses[vector] =
				(Live[vector]
				| (ldptr->buses[vector]
				& ~Dead[vector])) & REGS[vector];
					/* current live registers */
			 ldptr->bdef[vector] =
				(Dead[vector]
				| (ldptr->bdef[vector]
				& ~Live[vector])) & REGS[vector];
					/* current registers killed by block */

			} /* END OF for(vector=0; vector<NVectors; vector++) */
		 PutTxDead(p,Dead,NVectors);	/* Put live and dead info. */
		 PutTxLive(p,Live,NVectors);	/* into text node.	*/
		 if (p == b->FirstTN)		/* Stop if reached first node */
			break;
		} /* END OF for(p = b->LastTN; !IsTxLabel(p); ... */
	} /* END OF for(ALLB(b,b0.next) */
/* Propagate live/dead data throughout the flow graph, using Aho and
** Ullman algorithm.
*/

 do	{changed = FALSE;		/* will continue until no changes */
	 for(ALLB(b,b0.next))
		{LDREG in[NVECTORS];
		 LDREG out[NVECTORS];

		 for(vector = 0; vector < NVectors; vector++)
			{if((b->nextr == NULL)
					&& ((b->nextl == NULL)
					    || (IsTxBr(b->LastTN)
						&& !IsTxUncBr(b->LastTN))))
				{
	    /* This case represents a return, or an unconditional indexed
	    ** jump, or a switch.  If we had better connectivity in the
	    ** flow graph, we could trace all successors correctly.  As
	    ** things are, we have to assume the worst about what registers
	    ** are live going into the next block.  For a return, this means
	    ** those registers that can be used to return a value.  For
	    ** others, we mark all registers live.
	    */
				 out[vector] = IsTxRet(b->LastTN) ?
					RETREG[vector] : REGS[vector];
				}
			 else		/* OUT = union (of successors) IN */
				{out[vector] = 0;	/* registers out of*/
							/* current block. */
				 if(b->nextr != NULL)
				 out[vector] |=
					lddata[b->nextr->index].bin[vector];
				 if(b->nextl != NULL)
				 out[vector] |=
					lddata[b->nextl->index].bin[vector];
				}

			 ldptr = lddata + b->index;	/* point at data for*/
							/* current block */
						/* IN = OUT - DEF u USE */
			 in[vector] = (out[vector] & ~ldptr->bdef[vector])
					| ldptr->buses[vector];
			} /* END OF for(vector = 0; vector < NVectors; ...) */

						/* See what changed.	 */

		 for(vector = 0; vector < NVectors; vector++)
			 if(in[vector] != ldptr->bin[vector]
					|| out[vector] != ldptr->bout[vector])
				changed = TRUE;

						/* Set changed values. */
		 if(changed)
			{for(vector = 0; vector < NVectors; vector++)
				{ldptr->bin[vector] = in[vector];
				 ldptr->bout[vector] = out[vector];
				} /* End of for(vector = 0; ... */
			} /* END OF if(changed) */
		} /* END OF for(ALLB ... */
	} while (changed);


/* Now set the final live/dead (really, just live) information in
** each node of each block.
*/

 for(ALLB(b,b0.next))
	{LDREG live[NVECTORS];

	 for(vector = 0; vector < NVectors; vector++)
		live[vector] = lddata[b->index].bout[vector];
				/* go backward again through each block */
				/* initial live is outgoing regs of block */

	 for(p = b->LastTN; !IsTxLabel(p); p = GetTxPrevNode(p))
		{LDREG newlive[NVECTORS];

		 for(vector = 0; vector < NVectors; vector++)
			newlive[vector] = (p->nlive[vector]
				| (live[vector]
					& ~p->ndead[vector])) & REGS[vector];
		 PutTxLive(p,live,NVectors);	/* Put live bits in node. */
						/* live for this node is what */
						/* was live going into	*/
						/* successor.	*/
		 for(vector = 0; vector < NVectors; vector++)
			 live[vector] = newlive[vector];	/* live[vector]
						** for next node is whatever
						** else we used, but didn't kill
						*/
		 if (p == b->FirstTN)
			break;		/* Quit if first node in block. */
		} /* End of for(p = b->LastTN; ... */
	} /* End of for(ALLB ... */

 xfree((char *) lddata);			/* Free up temp. storage. */
 return;
}
#endif /* def LIVEDEAD */
	STATIC void
mkltbl()		/* make label table with only definitions */

{register BLOCK *b;
 register TN_Id p;
 STATIC void addlbl();	/* Add label in label table.	*/
 STATIC void clrltbl();	/* Clears label table.	*/

 clrltbl();

			/* Add definitions from labels in text section */

 for (ALLB(b, b0.next))
	for (p = b->FirstTN; IsTxLabel(p); p = GetTxNextNode(p))
		addlbl(GetTxOperandAd(p,0), b);
 return;
}


	STATIC void
clrltbl() 		 /* Clear label table.	*/
{extern int Numlbls;	/* Number of labels encountered */
 extern LBL *Lbltbl[];
 register LBL **p, *q, *r = NULL;
 for (p = &Lbltbl[0]; p <= &Lbltbl[LBLTBLSZ-1]; p++)
	for (q = *p; q != NULL; q = r)
		{
		 r = q->next;
		 Free(q);
		}

 CLEAR(&Lbltbl[0], LBLTBLSZ * sizeof(LBL *));
 Numlbls = 0;
 return;
}

#define	HASH(a)	(&Lbltbl[(unsigned int) (a) % LBLTBLSZ])

	STATIC void
addlbl(an_id,b)			/* Add label in label table.	*/
register AN_Id an_id;		/* AN_Id of label.	*/
BLOCK *b;			/* Block this label is in.	*/

{extern int Numlbls;		/* Number of labels encountered.	*/
 extern int errno;
 extern void fatal();
 register LBL *p,**hp;

 hp = HASH(an_id);
 for(p = *hp; p != NULL; p = p->next)
	if(p->ad == an_id)	/* if we found it */
		{p->bl = b;	/* install the new block it's in */
		 return;
		}
 /* didn't find it */
 if((p = (LBL *)Malloc(sizeof(LBL))) == NULL)
	fatal("addlbl: Malloc failed (%d).\n",errno);
 p->next = *hp;			/* insert new LBL at the */
 p->ad = an_id;			/* Put in AN_Id of label. */
 *hp = p;			/* beginning of the hash chain */
 Numlbls++;			/* increment total # labels */
 p->bl = b;			/* install the new block it's in */
 return;
}
	STATIC BLOCK *
findlbl(an_id)			/* Find label in label table and */
				/* return the block that it's in. */
register AN_Id an_id;		/* AN_Id of label.	*/

{register LBL *p;
 for(p = *(HASH(an_id)); p != NULL; p = p->next)
	if(p->ad == an_id)	/* if we found it */
		return(p->bl);	/* return the block it's in. */
 return(NULL);			/* didn't find it */
}
#undef HASH

	char *
getspace(n)
register unsigned n;	/* return a pointer to "n" bytes */

{register char *p = Lasta;

			/* round up so pointers are always word-aligned */
	/* int conversions are to avoid call to software remaindering */

 n += sizeof(char *) - ((int) n % (int) sizeof(char *));
 Maxu += n;
 while ((Lasta += n) >= Lastx) {
 	*Space = NEWBLK(1, struct space_t);
 	p = Lasta = (char *) &(*Space)->space[0];
 	Lastx = (char *) &(*Space)->space[NSPACE - 1];
 	(*Space)->next = NULL;
 	Space = &(*Space)->next;
	}
 return (p);
}
/* Branch shortening
**
** This code shortens span-dependent branches with assistance from
** machine dependent routines.  The interface is as follows:
**
**	bspan(flag)	is the entry point available to machine-
**			dependent routines; the flag is TRUE to print
**			debugging information
**
**	BSHORTEN	symbol, defined in "defs"; enables all this
**
**	int instsize(node)
**			routine or macro; returns upper bound on size of
**			instruction in node in arbitrary units
**	void bshorten(node,dist)
**			routine or macro; changes op at node to be
**			shortened version of branch, based on (long)
**			distance (dist) between branch and target
**
** The algorithm proceeds in two passes over the blocks of the program.
** The first pass calculates the relative PC (program counter) value for
** the beginning of each block.  (Remember, labels are always at the
** beginning of a block.)  Since branches are always at the end of blocks,
** the assumption is that the machine's program counter register always
** points to the beginning of the block following the branch when the
** branch is executed.  (This assumption for the purpose of calculating
** distances.)  Branches are assumed to be shortenable both forward and
** backward.
**
** The PC values for the blocks are kept in a dynamically allocated
** array.  The array is size idx+2, where idx is the highest block
** number.  The +2 accounts for not using array[0] (we index into
** the array by block index numbers which are non-zero) and for one
** additional entry at the end to contain the PC just after the
** last block.
**
** Pass two calculates distances between branches and their targets
** and shortens branches which can be shortened.
*/

#ifdef	BSHORTEN

	void
bspan(flag)				/* shorten span-dependent branches */
boolean flag;				/* TRUE to print debug info. */
{
 STATIC BLOCK *findlbl();	/* Finds a label in label table & */
				/* returns block it's in */
 long * bpc;			/* point to array of PC's */
 register BLOCK * block;	/* block pointer */
 register NODE * node;		/* pointer for scanning block's nodes */
 BLOCK * target;		/* branch target block */
 char * label;			/* branch label string */
 long pc;			/* current PC */
 long pcdiff;			/* PC difference, branch to target */
 extern int instsize();		/* returns size of instruction */
 extern char *xalloc();		/* Allocates space.	*/

    bldgr(FALSE);			/* build flow graph */
/* allocate array for block start PC's */

    bpc = (long *) xalloc( sizeof(long) * (idx+2));
    pc = 0;				/* current PC */

    /* make first pass to compute PC at start of each block */

    if (flag)
	printf("%c Block starting PC:\n",ComChar);

    for ( ALLB(block, b0.next) )
    {
	if (flag)
	    printf("%c\t%d\t%d\n",ComChar, block->index, pc);

	bpc[block->index] = pc;		/* current PC is block start PC */
	for (node = block->FirstTN ; ; node = node->forw)
	{
	    pc += instsize(node);	/* increase PC by instruction size */
	    if (node == block->LastTN)
		break;			/* done this block at last inst. */
	}
    }
    bpc[idx+1] = pc;			/* set PC of non-existent next block */
    if (flag)
	printf("%c\t(last)\t%d\n",ComChar, pc);
    /* Pass 2.  Try to shorten branches. */

    for ( ALLB(block, b0.next) )
    {
	if (IsTxBr(block->LastTN) && (label = getp(block->LastTN)) != NULL)
	{
	    /* Beware of non-existent target for branch */

	    if((target = findlbl(label)) == NULL)
		pcdiff = ~((unsigned long) 0) >> 1; /* maximum offset */
	    else
		pcdiff = bpc[target->index]	/* target PC */
			- bpc[block->index + 1];
					/* branch PC (PC of next block */

	    /* shorten branch if branch-to-target distance short enough */

	    if (flag)
	    {
		printf("%c Difference:  %d -- Shorten:\t",ComChar, pcdiff);
		fprinst(stdout,-1,block->LastTN);
	    }
	    bshorten(block->LastTN,pcdiff);
	}
    }
 xfree((char *) bpc);			/* free up array */
 return;
}

#endif	/* def BSHORTEN */


#ifdef LINT
PBLIST(title)
char *title;
{register BLOCK *b;
 extern void fprinst();		/* Prints an instruction on a stream.	*/
 register TN_Id tn_id;

 printf("%c\tBlock List %s\n",ComChar,title);
 for(ALLB(b,b0.next))
	{printf("\t\tBlock %d,\tleft %d,\tright %d\n",
		b->index,PRINDEX(nextl),PRINDEX(nextr));
	 for(tn_id = b->FirstTN; tn_id != b->LastTN->forw; tn_id = tn_id->forw)
		fprinst(stdout,-1,tn_id);
	}
 return;
}
#endif /*LINT*/
